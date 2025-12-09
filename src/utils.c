#include "utils.h"
#include "align.h"
#include "zone.h"
#include "block.h"
#include "alloc_hdr.h"
#include <unistd.h>

/*
** ft_pagesize()
**
** Returns the system page size, caching the result since it never changes.
** Uses sysconf(_SC_PAGESIZE) which is POSIX-compliant and works on both
** Linux and macOS.
*/

size_t	ft_pagesize(void)
{
	static size_t	cached_pagesize = 0;

	if (cached_pagesize == 0)
	{
		cached_pagesize = (size_t)sysconf(_SC_PAGESIZE);
		if (cached_pagesize == 0)
			cached_pagesize = 4096;
	}
	return (cached_pagesize);
}

/*
** ft_calculate_zone_size()
**
** For TINY zones: FT_TINY_ZONE_PAGES * pagesize
** For SMALL zones: FT_SMALL_ZONE_PAGES * pagesize
** For LARGE zones: Round up (zone_hdr + block_hdr + alloc_hdr + size) to pagesize
*/

size_t	ft_calculate_zone_size(uint8_t type, size_t request_size)
{
	const size_t	ps = ft_pagesize();

	if (type == FT_ZONE_TINY)
		return (FT_TINY_ZONE_PAGES * ps);
	else if (type == FT_ZONE_SMALL)
		return (FT_SMALL_ZONE_PAGES * ps);
	else
	{
		const size_t	needed = FT_ZONE_HDR_SIZE + request_size;
		return (FT_ALIGN_UP(needed, ps));
	}
}

/*
** ft_calculate_alloc_size()
**
** Total size = block_header + alloc_header + user_size (all aligned)
** We align the total to ensure the next block will be properly aligned.
*/

size_t	ft_calculate_alloc_size(size_t user_size)
{
	size_t	total;

	total = FT_BLOCK_HDR_SIZE + user_size;
	return (FT_ALIGN_UP(total, FT_ALIGN_SIZE));
}


/*
** ft_memcpy()
**
** From Libft
*/

void	*ft_memcpy(void *dst, const void *src, size_t n)
{
	char		*temp;
	const char	*s;

	if (dst == 0)
		return (NULL);
	temp = dst;
	s = src;
	while (n--)
		*temp++ = *s++;
	return (dst);
}
