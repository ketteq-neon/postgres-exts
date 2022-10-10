/**
 * (C) KetteQ, Inc.
 */

#ifndef KETTEQ_POSTGRESQL_EXTENSIONS_COMMON_H
#define KETTEQ_POSTGRESQL_EXTENSIONS_COMMON_H

#include <stddef.h>
#include <sys/types.h>
#include <glib-2.0/glib.h>

#include "postgres.h"
#include <utils/hsearch.h>
#include "storage/shmem.h"
#include "common/hashfn.h"

#define RET_SUCCESS 0 // Success.
#define RET_ERROR -1 // Generic error.
#define RET_ERROR_MIN_GT_MAX -2 //
#define RET_ERROR_NULL_VALUE -3
#define RET_ERROR_CANNOT_ALLOCATE -4
#define RET_ERROR_INVALID_PARAM -5
#define RET_ERROR_NOT_FOUND -6
#define RET_ERROR_UNSUPPORTED_OP -7
#define RET_ERROR_NOT_READY -8 // The Function requires a previous step before using.
#define RET_ADD_DAYS_NEGATIVE -9
#define RET_ADD_DAYS_POSITIVE -10
#define RET_ERROR_CANNOT_ATTACH_SHMEM -11
#define RET_ERROR_CANNOT_ATTACH_NO_CACHE -12
#define RET_ERROR_CANNOT_ATTACH_CAL_COUNT_TOO_BIG -13

#define CALENDAR_NAME_MAX_LEN 90
#define CALENDAR_NAMES_HASH_FLAGS (HASH_ELEM | HASH_STRINGS)
#define CALENDAR_NAMES_HASH_NAME "KQ_IMCX_CAL_NAMES_SMHTAB"

typedef struct {
	char key[CALENDAR_NAME_MAX_LEN];
	int calendar_id;
} CalendarNameEntry;

typedef struct {
	int id; // Calendar ID (Same as in origin DB)
	char name[CALENDAR_NAME_MAX_LEN];
	int32 * dates; // Dates contained in the Calendar
	unsigned long dates_size; // Count of dates.
	int page_size; // Calculated Page Size
	long first_page_offset; // Offset of the First Page
	long * page_map; // Page map contained in the Calendar
	long page_map_size; // Count of Page map entries
} Calendar;

typedef struct {
	Calendar ** calendars; // Calendars contained in the ICMX struct
	unsigned long calendar_count; // Count of Calendars
	unsigned long entry_count; // Count of Entries (From all Calendars)
	int min_calendar_id; // Min calendar ID, used as offset to set the calendar indices
	bool cache_filled; // Control variable set to TRUE when the `cache_finish()` function is called.
	HTAB *pg_calendar_name_hashtable;
} IMCX;

#endif //KETTEQ_POSTGRESQL_EXTENSIONS_COMMON_H
