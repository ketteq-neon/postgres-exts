/**
 * (C) ketteQ, Inc.
 */

#ifndef KETTEQ_POSTGRESQL_EXTENSIONS_CACHE_H
#define KETTEQ_POSTGRESQL_EXTENSIONS_CACHE_H

#include <sys/types.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "math.h"
#include "inttypes.h"

#include "common.h"
#include "util.h"

#include <storage/shmem.h>
#include <miscadmin.h>

/**
 * Initializes or attach the Calendar Names hashtable.
 * @param imcx Pointer to allocated IMCX struct
 * @return RET_ERROR_CANNOT_ALLOCATE = Hashtable cannot be allocated in memory or cannot be attached.
 *         RET_SUCCESS = SUCCESS
 */
int32 pg_init_hashtable(IMCX *imcx);

/**
 * Initializes main struct (IMCX), contains the calendars and control variables.
 * @param imcx Pointer to an allocated IMCX struct.
 * @param min_calendar_id First calendar id.
 * @param max_calendar_id Last calendar id.
 * @return RET_ERROR_MIN_GT_MAX = First Calendar Id is Greater than Last Calendar Id.
 *         RET_ERROR_CANNOT_ALLOCATE = Cannot Allocate Calendars.
 *         RET_SUCCESS = SUCCESS.
 */
int32 pg_cache_init(IMCX *imcx, int32 min_calendar_id, int32 max_calendar_id);

/**
 * Gets the calendar based on its ID. Can be used before calendar initialization.
 * @param imcx Pointer to an initialized IMCX struct.
 * @param calendar_id Calendar ID
 * @return Pointer to the Calendar of the given ID
 */
Calendar *pg_get_calendar(const IMCX *imcx, int32 calendar_id);

/**
 * Gets the calendar index based on its ID. Can be used before calendar initialization.
 * @param imcx Pointer to an initialized IMCX struct.
 * @param calendar_id Calendar ID
 * @return The calendar index of the given ID
 */
int32 get_calendar_index(const IMCX *imcx, int32 calendar_id);

/**
 * Allocates and initializes a Calendar.
 * @param calendar Pointer to an allocated Calendar struct.
 * @param calendar_id Calendar ID (From Query).
 * @param entry_size Count of entries to allocate.
 * @param entry_count_ptr (Output) Adds the entry_size to it.
 * @return RET_ERROR_INVALID_PARAM = Calendar Id is 0 (or negative?). If using Index, add 1 to the calendar_id.
 *         RET_ERROR_CANNOT_ALLOCATE = Cannot Allocate Calendar Entries.
 *         RET_ERROR_UNSUPPORTED_OP = Cache already finished, call invalidate to use init again.
 *         RET_SUCCESS = SUCCESS.
 */
int32 pg_calendar_init(Calendar *calendar, int32 calendar_id, int32 entry_size, int32 *entry_count_ptr);

/**
 * Sets a name for a given calendar index.
 * @param imcx Pointer to an initialized IMCX struct.
 * @param calendar Pointer to an initialized Calendar struct.
 * @param calendar_name Constant string containing the name of the calendar.
 * @return RET_SUCCESS = SUCCESS.
 */
int32 pg_set_calendar_name(IMCX *imcx, Calendar *calendar, const char *calendar_name);

/**
 * Gets the Calendar id by its given name.
 * @param imcx Pointer to an allocated IMCX struct.
 * @param calendar_name Constant string containing the name of the calendar.
 * @param calendar_id (Output) Pointer to the result Calendar Index.
 * @return RET_ERROR_NOT_FOUND = Calendar was not found using the provided name.
 *         RET_ERROR_UNSUPPORTED_OP = Calendar Id is too big or invalid.
 *         RET_SUCCESS = SUCCESS.
 */
int32 pg_get_calendar_id_by_name(IMCX *imcx, const char *calendar_name, int32 *calendar_id);

/**
 * Calculates and sets the page size for the given calendar pointer
 * @param calendar Pointer to initialized Calendar.
 * @return RET_ERROR_UNSUPPORTED_OP = Calculated PageSize is 0, cannot continue.
 *         RET_SUCCESS = SUCCESS.
 */
int32 pg_init_page_size(Calendar *calendar);

/**
 * Calculates the next or previous date for a given number of intervals. The next or previous date will
 * be obtained from the given Calendar, if the interval is bigger and cannot be calculated, the function
 * will return INT32_MAX (Which is Infinity+ in PostgreSQL). If the interval is too small, will return
 * the first date of the given Calendar.
 * @param imcx Pointer to an initialized IMCX struct.
 * @param calendar Pointer to an initialized and filled Calendar.
 * @param input_date Input date (DateADT format).
 * @param interval Intervals, can be negative.
 * @param result_date (Output) Pointer to the result date (DateADT format).
 * @param first_date_idx (Output) (Nullable) Pointer to the first date found index.
 * @param result_date_idx (Output) (Nullable) Pointer to the result date index.
 * @return RET_ERROR_NOT_READY = Cache is not ready or the `finish()` function was not called.
 *         RET_ADD_DAYS_NEGATIVE = The calculated date is out of bounds from the left.
 *         RET_ADD_DAYS_POSITIVE = The calculated date is out of bounds from the right, result_date will contain INT32_MAX.
 *         RET_SUCCESS = SUCCESS.
 */
int32 add_calendar_days(
    const IMCX *imcx,
    const Calendar *calendar,
    int32 input_date,
    int32 interval,
    int32 *result_date,
    int32 *first_date_idx,
    int32 *result_date_idx
);

/**
 * Clears the cache, deallocates memory and resets control variables, left the struct ready for a new allocation.
 * @param imcx Pointer to an allocated IMCX struct.
 * @return  RET_ERROR_UNSUPPORTED_OP = The cache is not filled.
 *          RET_SUCCESS = SUCCESS.
 */
int32 cache_invalidate(IMCX *imcx);

#endif //KETTEQ_POSTGRESQL_EXTENSIONS_CACHE_H
