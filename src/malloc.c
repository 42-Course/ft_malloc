/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   malloc.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: malloc                                     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/01/18                               #+#    #+#             */
/*   Updated: 2025/01/18                              ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "malloc.h"
#include "zone.h"
#include "block.h"
#include "alloc_hdr.h"
#include "utils.h"
#include "fit.h"
#include "align.h"
#include <stddef.h>


#ifdef USE_MALLOC_LOCK

#include <pthread.h>
static pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;

#define MALLOC_PREACTION   pthread_mutex_lock(&g_mutex)
#define MALLOC_POSTACTION  pthread_mutex_unlock(&g_mutex)

#else

#define MALLOC_PREACTION   (0)
#define MALLOC_POSTACTION  (0)

#endif /* USE_MALLOC_LOCK */

/*
** ft_free_list_remove()
**
** Removes a block from the zone's free list.
** Helper function for malloc when allocating a block.
*/

static void	ft_free_list_remove(ft_zone_t *zone, ft_block_t *block)
{
	if (block->prev_free)
		block->prev_free->next_free = block->next_free;
	else
		zone->free_head = block->next_free;
	if (block->next_free)
		block->next_free->prev_free = block->prev_free;
	block->prev_free = NULL;
	block->next_free = NULL;
}

/*
** ft_free_list_add()
**
** Adds a block to the front of the zone's free list.
** Helper function for free() when returning a block to the free pool.
*/

static void	ft_free_list_add(ft_zone_t *zone, ft_block_t *block)
{
	block->prev_free = NULL;
	block->next_free = zone->free_head;
	if (zone->free_head)
		zone->free_head->prev_free = block;
	zone->free_head = block;
}

/*
** ft_allocate_from_block()
**
** Allocates memory from a free block.
** Splits the block if there's excess space, marks it as allocated,
** sets up the allocation header, and returns the user pointer.
*/

static void	*ft_allocate_from_block(ft_zone_t *zone, ft_block_t *block,
	size_t alloc_size, size_t user_size)
{
	ft_block_t		*remainder;
	void			*user_ptr;

	ft_free_list_remove(zone, block);
#if SHOW_MORE
	remainder = ft_block_split(block, alloc_size, user_size);
#else
	remainder = ft_block_split(block, alloc_size);
#endif
	if (remainder)
		ft_free_list_add(zone, remainder);
	block->is_free = 0;
	block->magic = FT_ALLOC_MAGIC;
	block->zone = zone;
	user_ptr = ft_block_data_ptr(block);
	zone->used_size += user_size;
	zone->block_count++;
	return (user_ptr);
}

/*
** malloc()
**
** Main allocation function.
** Algorithm:
** 1. Determine zone type based on size
** 2. Calculate total size needed (including all headers and alignment)
** 3. For TINY/SMALL: try to find existing free block, else create new zone
** 4. For LARGE: always create dedicated zone
** 5. Allocate from block and return user pointer
*/

static void	*_malloc(size_t size)
{
	uint8_t		type;
	size_t		alloc_size;
	ft_zone_t	*zone;
	ft_block_t	*block;

	if (size == 0)
		return (NULL);
	type = ft_zone_get_type(size);
	alloc_size = ft_calculate_alloc_size(size); // block header + alloc header + size
	if (type == FT_ZONE_LARGE)
	{
		zone = ft_zone_create(type, alloc_size);
		if (!zone)
			return (NULL);
		block = zone->first_block;
		return (ft_allocate_from_block(zone, block, alloc_size, size));
	}
	block = ft_first_fit(type, alloc_size, &zone);
	if (!block)
	{
		zone = ft_zone_create(type, 0);
		if (!zone)
			return (NULL);
		block = zone->first_block;
	}
	return (ft_allocate_from_block(zone, block, alloc_size, size));
}

void	*malloc(size_t size)
{
	void *user_ptr;

	if (MALLOC_PREACTION != 0) {
    return 0;
  }
  user_ptr = _malloc(size);
  if (MALLOC_POSTACTION != 0) {
  }
	return user_ptr;
}

/*
** ft_coalesce_blocks()
**
** After freeing a block, attempt to merge it with adjacent free blocks
** to reduce external fragmentation.
*/

static void	ft_coalesce_blocks(ft_zone_t *zone, ft_block_t *block)
{
	ft_block_t	*next;

	if (block->next && ft_block_can_merge(block, block->next))
	{
		next = block->next;
		ft_free_list_remove(zone, next); // must remove since it's independent
		ft_block_merge(block, next);
	}
	if (block->prev && ft_block_can_merge(block->prev, block))
	{
		ft_free_list_remove(zone, block); // must remove since it's independent
		ft_block_merge(block->prev, block);
	}
}

/*
** free()
**
** Frees previously allocated memory.
** Algorithm:
** 1. Validate pointer using allocation header magic number
** 2. Mark block as free and add to free list
** 3. Coalesce with adjacent free blocks
** 4. If zone becomes empty, unmap it (all zone types)
*/

static void	_free(void *ptr)
{
	ft_block_t		*block;
	ft_zone_t		*zone;

	if (!ptr)
		return ;
	block = ft_block_from_data_ptr(ptr);
	if (!ft_block_is_valid(block))
		return ;
	block->magic = 0;
	zone = (ft_zone_t *)block->zone;
	block->is_free = 1;
	ft_free_list_add(zone, block);
	zone->used_size -= block->size;
	zone->block_count--;
	ft_coalesce_blocks(zone, block);
	if (zone->block_count == 0 && zone->type == FT_ZONE_LARGE)
		ft_zone_remove(zone);
}

void	free(void *ptr)
{
	if (MALLOC_PREACTION != 0) {
    return;
  }
	_free(ptr);
  if (MALLOC_POSTACTION != 0) {
  }
}

/*
** ft_try_extend_in_place()
**
** Attempts to extend an allocation in place by merging with the next free block.
** This avoids expensive malloc + memcpy + free operations when growing buffers.
**
** @param block: Current block to extend
** @param zone: Zone containing the block
** @param needed_size: Total size needed for the new allocation
** @return: 1 if extension succeeded, 0 if not possible
**
** Context: Called by realloc when growing an allocation. Checks if the next
** block in address order is free and large enough to accommodate the growth.
*/
#if SHOW_MORE
static int	ft_try_extend_in_place(ft_block_t *block, ft_zone_t *zone,
	size_t needed_size, size_t user_size)
#else
static int	ft_try_extend_in_place(ft_block_t *block, ft_zone_t *zone,
	size_t needed_size)
#endif
{
	ft_block_t	*next;
	size_t		available;
	ft_block_t	*remainder;

	next = block->next;
	if (!next || !next->is_free)
		return (0);
	available = block->size + next->size;
	if (available < needed_size)
		return (0);
	ft_free_list_remove(zone, next);
	ft_block_merge(block, next);
#if SHOW_MORE
	remainder = ft_block_split(block, needed_size, user_size);
#else
	remainder = ft_block_split(block, needed_size);
#endif
	if (remainder)
		ft_free_list_add(zone, remainder);
	return (1);
}

/*
** realloc()
**
** Changes the size of an allocation.
** Algorithm:
** 1. Handle special cases (NULL ptr, size 0)
** 2. Validate existing allocation
** 3. If new size fits in current block, just update header
** 4. Try to extend in place by merging with next free block
** 5. If that fails: allocate new block, copy data, free old block
*/

static void *_realloc(void *ptr, size_t size) {
	ft_block_t 		*block;
	void			*new_ptr;
	size_t			copy_size;
	size_t			needed_alloc_size;

	if (!ptr)
		return (_malloc(size));
	if (size == 0)
	{
		_free(ptr);
		return (NULL);
	}
	block = ft_block_from_data_ptr(ptr);
	if (!ft_block_is_valid(block))
		return (NULL);
	needed_alloc_size = ft_calculate_alloc_size(size);
	if (block->size >= needed_alloc_size)
	{
		return (ptr);
	}
#if SHOW_MORE
	if (ft_try_extend_in_place(block,
		(ft_zone_t *)block->zone, ft_calculate_alloc_size(size), size))
#else
	if (ft_try_extend_in_place(block,
		(ft_zone_t *)block->zone, needed_alloc_size))
#endif
	{
		((ft_zone_t *)block->zone)->used_size += (size - block->size);
		return (ptr);
	}
	new_ptr = _malloc(size);
	if (!new_ptr)
		return (NULL);
	copy_size = block->size < size ? block->size : size;
	ft_memcpy(new_ptr, ptr, copy_size);
	_free(ptr);
	return (new_ptr);
}

void	*realloc(void *ptr, size_t size)
{
	if (MALLOC_PREACTION != 0) {
    return 0;
  }
  ptr = _realloc(ptr, size);
  if (MALLOC_POSTACTION != 0) {
  }
  return ptr;
}

