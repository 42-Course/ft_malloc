// #ifndef MEM_LOGGER_H
// # define MEM_LOGGER_H

// # include <stddef.h>

// /*
// ** Memory State Logger
// **
// ** Dumps current memory state to JSON after each malloc/free/realloc.
// ** Uses existing zone/block structures - no additional tracking needed.
// **
// ** Usage:
// **   1. Compile with -DMALLOC_LOGGING
// **   2. Optionally set: export MALLOC_LOG=filename.json (default: malloc_log.json)
// **   3. Run program - state dumped after each operation
// **   4. Each dump appends to log file as a snapshot
// */

// /*
// ** mem_logger_dump()
// **
// ** Dumps current memory state to log file.
// ** Outputs all zones, blocks, and allocations in JSON format.
// ** Can be called manually or automatically via wrappers.
// */
// void	mem_logger_dump(void);

// /*
// ** Wrappers that call dump after each operation
// ** Enable with MALLOC_LOGGING
// */

// # ifdef MALLOC_LOGGING

// void	*malloc_logged(size_t size);
// void	free_logged(void *ptr);
// void	*realloc_logged(void *ptr, size_t size);

// #  define malloc(size) malloc_logged(size)
// #  define free(ptr) free_logged(ptr)
// #  define realloc(ptr, size) realloc_logged(ptr, size)

// # endif

// #endif

#ifndef MEM_LOGGER_H
#define MEM_LOGGER_H

#include <stddef.h>

/*
** mem_logger_dump()
**
** Dumps current state to JSON.
*/
void mem_logger_dump(void);

/*
** Wrappers (implemented in mem_logger.c)
*/
void *malloc_logged(size_t size);
void free_logged(void *ptr);
void *realloc_logged(void *ptr, size_t size);

/*
** Only override malloc/free/realloc if:
**   1) MALLOC_LOGGING is defined AND
**   2) We are NOT inside the allocator internals
*/
#if defined(MALLOC_LOGGING) && !defined(INTERNAL_ALLOCATOR_BUILD)

# define malloc(size)        malloc_logged(size)
# define free(ptr)           free_logged(ptr)
# define realloc(ptr, size)  realloc_logged(ptr, size)

#endif

#endif
