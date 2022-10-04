/**
 * (C) KetteQ, Inc.
 */

#ifndef KETTEQ_POSTGRESQL_EXTENSIONS_COMMON_H
#define KETTEQ_POSTGRESQL_EXTENSIONS_COMMON_H

#include <stddef.h>
#include <sys/types.h>
#include <glib.h>

#define RET_OK 0
#define RET_ERROR -1
#define RET_MIN_GT_MAX -2
#define RET_NULL_VALUE -3

typedef struct {
	unsigned long calendar_id;
	long * dates;
	ulong dates_size;
	int page_size;
	int first_page_offset;
	int * page_map;
	int page_map_size;
} Calendar;

typedef struct {
	Calendar * calendars;
	unsigned long calendar_count;
	unsigned long entry_count;
	bool cache_filled;
	GHashTable * calendar_name_hashtable;
} IMCX;

#endif //KETTEQ_POSTGRESQL_EXTENSIONS_COMMON_H
