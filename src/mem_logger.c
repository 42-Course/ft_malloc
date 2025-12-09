#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>
#include <stddef.h>

#include "zone.h"
#include "block.h"
#include "alloc_hdr.h"
#include "mem_logger.h"

/*
** Make absolutely sure that inside this file,
** malloc/free/realloc refer to the REAL libc versions.
*/
#ifdef malloc
# undef malloc
#endif
#ifdef free
# undef free
#endif
#ifdef realloc
# undef realloc
#endif

static uint64_t g_snapshot_count = 0;
static uint64_t g_start_time_us = 0;
static int      g_logging_active = 0;

/* ----------------------------------------------- */
/* timestamp                                        */
/* ----------------------------------------------- */

static uint64_t get_timestamp_us(void)
{
	struct timeval tv;
	uint64_t now;

	gettimeofday(&tv, NULL);
	now = (uint64_t)tv.tv_sec * 1000000 + tv.tv_usec;

	if (g_start_time_us == 0)
		g_start_time_us = now;

	return (now - g_start_time_us);
}

/* ----------------------------------------------- */
/* log filename                                    */
/* ----------------------------------------------- */

static char g_log_filename[256] = {0};

static const char *get_log_filename(void)
{
	char *env_filename;
	struct timeval tv;
	pid_t pid;

	if (g_log_filename[0] != '\0')
		return g_log_filename;

	env_filename = getenv("MALLOC_LOG");
	if (env_filename)
	{
		snprintf(g_log_filename, sizeof(g_log_filename), "%s", env_filename);
		return g_log_filename;
	}

	pid = getpid();
	gettimeofday(&tv, NULL);

	snprintf(g_log_filename, sizeof(g_log_filename),
						"malloc_log_%d_%ld.json", (int)pid, (long)tv.tv_sec);

	return g_log_filename;
}

/* ----------------------------------------------- */
/* JSON helpers                                    */
/* ----------------------------------------------- */

static const char *zone_type_str(uint8_t type)
{
	if (type == FT_ZONE_TINY)  return "TINY";
	if (type == FT_ZONE_SMALL) return "SMALL";
	if (type == FT_ZONE_LARGE) return "LARGE";
	return "UNKNOWN";
}

static void dump_zone(FILE *f, ft_zone_t *zone, int *first_zone)
{
	ft_block_t *block;
	int first_block = 1;

	if (!*first_zone)
		fprintf(f, ",\n");
	*first_zone = 0;

	fprintf(f, "    {\n");
	fprintf(f, "      \"type\": \"%s\",\n", zone_type_str(zone->type));
	fprintf(f, "      \"address\": \"%p\",\n", (void *)zone);
	fprintf(f, "      \"total_size\": %zu,\n", zone->total_size);
	fprintf(f, "      \"used_size\": %zu,\n", zone->used_size);
	fprintf(f, "      \"block_count\": %zu,\n", zone->block_count);
	fprintf(f, "      \"allocations\": [\n");

	block = zone->first_block;
	while (block)
	{
		if (!block->is_free)
		{
			if (!first_block)
				fprintf(f, ",\n");

			fprintf(f, "        {\"address\": \"%p\", \"size\": %zu}",
							ft_block_data_ptr(block), block->size);

			first_block = 0;
		}
		block = block->next;
	}

	fprintf(f, "\n      ]\n");
	fprintf(f, "    }");
}

static void dump_zones(FILE *f)
{
	ft_zone_t *zone;
	int first = 1;

	fprintf(f, "  \"zones\": [\n");

	zone = g_zone_mgr.tiny_zones;
	while (zone) { dump_zone(f, zone, &first); zone = zone->next; }

	zone = g_zone_mgr.small_zones;
	while (zone) { dump_zone(f, zone, &first); zone = zone->next; }

	zone = g_zone_mgr.large_zones;
	while (zone) { dump_zone(f, zone, &first); zone = zone->next; }

	fprintf(f, "\n  ]\n");
}

/* ----------------------------------------------- */
/* Public function                                 */
/* ----------------------------------------------- */

void mem_logger_dump(void)
{
	FILE *f;
	const char *filename = get_log_filename();

	f = fopen(filename, "a");
	if (!f)
			return;

	fprintf(f, "{\n");
	fprintf(f, "  \"snapshot_id\": %llu,\n",
					(unsigned long long)g_snapshot_count++);
	fprintf(f, "  \"timestamp_us\": %llu,\n",
					(unsigned long long)get_timestamp_us());

	dump_zones(f);

	fprintf(f, "}\n");
	fclose(f);
}

/* ----------------------------------------------- */
/* Wrappers                                        */
/* ----------------------------------------------- */

#if defined(MALLOC_LOGGING)

void *malloc_logged(size_t size)
{
	void *ptr;

	if (g_logging_active)
		return malloc(size);

	ptr = malloc(size);
	g_logging_active = 1;
	mem_logger_dump();
	g_logging_active = 0;
	return ptr;
}

void free_logged(void *ptr)
{
	if (g_logging_active) {		
		free(ptr);
		return;
	}

	free(ptr);
	g_logging_active = 1;
	mem_logger_dump();
	g_logging_active = 0;
}

void *realloc_logged(void *ptr, size_t size)
{
	void *new_ptr;

	if (g_logging_active)
		return realloc(ptr, size);

	new_ptr = realloc(ptr, size);
	g_logging_active = 1;
	mem_logger_dump();
	g_logging_active = 0;
	return new_ptr;
}

#endif
