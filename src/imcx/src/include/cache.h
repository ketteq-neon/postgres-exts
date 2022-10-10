/**
 * (C) KetteQ, Inc.
 */

#ifndef KETTEQ_POSTGRESQL_EXTENSIONS_CACHE_H
#define KETTEQ_POSTGRESQL_EXTENSIONS_CACHE_H

#include <sys/types.h>
#include <stdbool.h>
#include <stdio.h>
#include <glib-2.0/glib.h>
#include <stdlib.h>
#include <string.h>

#include "math.h"
#include "inttypes.h"

#include "common.h"
#include "util.h"

#include <storage/shmem.h>
#include <miscadmin.h>

int pg_cache_attach (IMCX *imcx);
int pg_cache_init (IMCX *imcx, unsigned long min_calendar_id, unsigned long max_calendar_id);
int pg_calendar_init (IMCX *imcx, int calendar_id, unsigned long entry_size);
int pg_set_calendar_name (IMCX *imcx, unsigned long calendar_index, const char *calendar_name);
int pg_get_calendar_index_by_name (IMCX *imcx, const char *calendar_name, unsigned long *calendar_index);

/**
 * Initializes main struct (IMCX), contains the calendars and control variables.
 * @param imcx Pointer to an allocated IMCX struct.
 * @param min_calendar_id First calendar id.
 * @param max_calendar_id Last calendar id.
 * @return RET_ERROR_MIN_GT_MAX = First Calendar Id is Greater than Last Calendar Id.
 *         RET_ERROR_CANNOT_ALLOCATE = Cannot Allocate Calendars.
 *         RET_SUCCESS = SUCCESS.
 */
int cache_init (IMCX *imcx, unsigned long min_calendar_id, unsigned long max_calendar_id);

/**
 * Initializes a calendar.
 * @param imcx Pointer to an allocated IMCX struct.
 * @param calendar_id Calendar to initialize (contained in the IMCX struct).
 * @param entry_size Count of entries to allocate.
 * @return RET_ERROR_INVALID_PARAM = Calendar Id is 0 (or negative?). If using Index, add 1 to the calendar_id.
 *         RET_ERROR_CANNOT_ALLOCATE = Cannot Allocate Calendar Entries.
 *         RET_ERROR_UNSUPPORTED_OP = Cache already finished, call invalidate to use init again.
 *         RET_SUCCESS = SUCCESS.
 */
int calendar_init (IMCX *imcx, unsigned long calendar_id, unsigned long entry_size);

/**
 * Sets a name for a given calendar index.
 * @param imcx Pointer to an allocated IMCX struct.
 * @param calendar_index Calendar index.
 * @param calendar_name Constant string containing the name of the calendar.
 */
void set_calendar_name (IMCX *imcx, unsigned long calendar_index, const char *calendar_name);

/**
 * Gets the Calendar Index by its given name.
 * @param imcx Pointer to an allocated IMCX struct.
 * @param calendar_name Constant string containing the name of the calendar.
 * @param calendar_index (Output) Pointer to the result Calendar Index.
 * @return RET_ERROR_NOT_FOUND = Calendar was not found using the provided name.
 *         RET_ERROR_UNSUPPORTED_OP = Calendar Id is too big or invalid.
 *         RET_SUCCESS = SUCCESS.
 */
int get_calendar_index_by_name (IMCX *imcx, const char *calendar_name, unsigned long *calendar_index);

/**
 * Calculates and sets the page size for the given calendar pointer
 * @param calendar Pointer to Calendar.
 * @return RET_ERROR_UNSUPPORTED_OP = Calculated PageSize is 0, cannot continue.
 *         RET_SUCCESS = SUCCESS.
 */
int init_page_size (Calendar *calendar);

/**
 * Finishes the creation of the cache. Must be called when the calendars are loaded.
 * @param imcx Pointer to an allocated IMCX struct.
 */
void cache_finish (IMCX *imcx);

/**
 * Calculates the next or previous date for a given number of intervals. The next or previous date will
 * be obtained from the given Calendar, if the interval is bigger and cannot be calculated, the function
 * will return INT32_MAX (Which is Infinity+ in PostgreSQL). If the interval is too small, will return
 * the first date of the given Calendar.
 * @param imcx Pointer to an allocated IMCX struct.
 * @param calendar_index Calendar index.
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
int add_calendar_days (
	const IMCX *imcx,
	unsigned long calendar_index,
	long input_date,
	long interval,
	int32 *result_date,
	unsigned long *first_date_idx,
	unsigned long *result_date_idx
);

/**
 * Clears the cache, deallocates memory and resets control variables, left the struct ready for a new allocation.
 * @param imcx Pointer to an allocated IMCX struct.
 * @return  RET_ERROR_UNSUPPORTED_OP = The cache is not filled.
 *          RET_SUCCESS = SUCCESS.
 */
int cache_invalidate (IMCX *imcx);

#endif //KETTEQ_POSTGRESQL_EXTENSIONS_CACHE_H
