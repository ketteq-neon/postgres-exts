/**
 * (C) KetteQ, Inc.
 */

#ifndef KETTEQ_POSTGRESQL_EXTENSIONS_COMMON_H
#define KETTEQ_POSTGRESQL_EXTENSIONS_COMMON_H

#include <stddef.h>
#include <sys/types.h>

#include "postgres.h"
#include <utils/hsearch.h>
#include "storage/shmem.h"
#include "common/hashfn.h"

#define RET_SUCCESS 0 // Generic Success.
#define RET_ERROR (-1) // Generic error.
#define RET_ERROR_MIN_GT_MAX (-2) // Number is greater than the max allowed.
#define RET_ERROR_NULL_VALUE (-3) // Null value obtained or parameter is null.
#define RET_ERROR_CANNOT_ALLOCATE (-4) // Cannot allocate memory.
#define RET_ERROR_INVALID_PARAM (-5) // Parameter is not valid.
#define RET_ERROR_NOT_FOUND (-6) // Entry not found.
#define RET_ERROR_UNSUPPORTED_OP (-7) // Unsupported Operation.
#define RET_ERROR_NOT_READY (-8) // The Function requires a previous step before using.
#define RET_ADD_DAYS_NEGATIVE (-9) // Add Days: Result Date is to the left (Negative).
#define RET_ADD_DAYS_POSITIVE (-10) // Add Days: Result date is to the right (Positive).

#define CALENDAR_NAME_MAX_LEN 90 // Max Length of Calendar Name
#define DEF_DEBUG_LOG_LEVEL DEBUG1 // Default Logging Level, Messages only available in debug builds.

/**
 * Structure for Calendar name entries inside the a hashmap.
 */
typedef struct {
  char key[CALENDAR_NAME_MAX_LEN]; // Calendar Name
  int32 calendar_id; // Calendar Id
} CalendarNameEntry; // Hashtable entry that contains the calendar id, can be found using the "key" Calendar Name.

/**
 * Calendar struct, this is the main struct for calendars, inside we can find the id,
 * name, entries (dates) and more.
 */
typedef struct {
  int32 id; // Calendar ID (Same as in origin DB)
  char name[CALENDAR_NAME_MAX_LEN]; // Calendar Name
  int32 *dates; // Dates contained in the Calendar
  int32 dates_size; // Count of dates.
  int32 page_size; // Calculated Page Size
  int32 first_page_offset; // Offset of the First Page
  int32 *page_map; // Page map contained in the Calendar
  int32 page_map_size; // Count of Page map entries
} Calendar;

/**
 * The IMCX struct is the container struct for an In-Memory Calendar Extension (IMCX) instance, inside of the
 * instance, several calendars can be loaded. This instance should be allocated in 'preload' time, requiring installing
 * the extension in the 'preload_shared_libraries'..
 */
typedef struct {
  Calendar **calendars; // Calendars contained in the ICMX struct
  int32 calendar_count; // Count of Calendars
  int32 entry_count; // Count of Entries (From all Calendars)
  int32 min_calendar_id; // Min calendar ID, used as offset to set the calendar indices
  bool cache_filled; // Control variable set to TRUE when the `cache_finish()` function is called.
  HTAB *pg_calendar_name_hashtable; // Postgres' Hashtable that contains the names of the slices/calendars.
} IMCX;

#endif //KETTEQ_POSTGRESQL_EXTENSIONS_COMMON_H
