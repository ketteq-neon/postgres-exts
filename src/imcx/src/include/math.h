/**
 * (C) KetteQ, Inc.
 */


#ifndef KETTEQ_POSTGRESQL_EXTENSIONS_MATH_H
#define KETTEQ_POSTGRESQL_EXTENSIONS_MATH_H

#include <stdio.h>
#include <sys/types.h>

#include "cache.h"
#include "common.h"

/**
 * Calculates the page size of the calendar depending on the intervals. Calendar entries must be ordered ASC.
 * @param first_date Earliest date interval in calendar.
 * @param last_date Latest date interval in calendar.
 * @param entry_count Count of intervals in the calendar.
 * @return page size as integer.
 */
int32 calculate_page_size(int32 first_date, int32 last_date, int32 entry_count);
/**
 * Returns the first entry of the given date that is inside the given calendar.
 * @param date_adt
 * @param calendar
 * @return
 */
int32 get_closest_index_from_left(int32 date_adt, Calendar calendar);
/**
 * From here: https://www.geeksforgeeks.org/binary-search/
 * @param arr
 * @param left
 * @param right
 * @param value
 * @return
 */
int32 binary_search(const int32 arr[], int32 left, int32 right, int32 value);
/**
 * This a modified binary search algo. Returns the upper limit index.
 * @param arr
 * @param left
 * @param right
 * @param value
 * @return
 */
int32 left_binary_search(const int32 *arr, int32 left, int32 right, int32 value);

#endif //KETTEQ_POSTGRESQL_EXTENSIONS_MATH_H
