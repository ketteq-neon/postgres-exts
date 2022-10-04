/**
 * (C) KetteQ, Inc.
 */

#ifndef KETTEQ_POSTGRESQL_EXTENSIONS_MATH_H
#define KETTEQ_POSTGRESQL_EXTENSIONS_MATH_H

#include <stdio.h>
#include <sys/types.h>

#include "cache.h"
#include "../common.h"

#define PAST DATEVAL_NOBEGIN
#define FUTURE DATEVAL_NOEND

/**
 * Calculates the page size of the calendar depending on the intervals. Calendar entries must be ordered ASC.
 * @param first_date Earliest date interval in calendar.
 * @param last_date Latest date interval in calendar.
 * @param entry_count Count of intervals in the calendar.
 * @return page size as integer.
 */
int calculate_page_size (long first_date, long last_date, ulong entry_count);

/**
 * Returns the first entry of the given date that is inside the given calendar.
 * @param date_adt
 * @param calendar
 * @return
 */
long get_closest_index_from_left (int date_adt, Calendar calendar);

/**
 * From here: https://www.geeksforgeeks.org/binary-search/
 * @param arr
 * @param left
 * @param right
 * @param value
 * @return
 */
long binary_search (const long arr[], long left, long right, long value);

/**
 * This a modified binary search algo. that returns the upper limit index.
 * @param arr
 * @param left
 * @param right
 * @param value
 * @return
 */
long left_binary_search (const long arr[], long left, long right, long value);

#endif //KETTEQ_POSTGRESQL_EXTENSIONS_MATH_H
