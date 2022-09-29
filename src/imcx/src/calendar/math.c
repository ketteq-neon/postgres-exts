/**
 * (C) ketteQ, Inc.
 */

#include <stdio.h>
#include "math.h"
#include "../common.h"

/**
 * Calculates the page size of the calendar depending on the intervals. InMemCalendar entries must be ordered ASC.
 * @param first_date Earliest date interval in calendar.
 * @param last_date Latest date interval in calendar.
 * @param entry_count Count of intervals in the calendar.
 * @return page size as integer.
 */
int calmath_calculate_page_size(int first_date, int last_date, int entry_count) {
    double date_range = last_date - first_date;
    double avg_entries_per_week_calendar = date_range / 7.0;
    double avg_entries_per_month_calendar = date_range / 30.33;
    double entry_count_d = (double) entry_count;
    //
    int page_size_tmp = 32; // monthly calendar
    //
    if (entry_count_d > avg_entries_per_week_calendar) {
        page_size_tmp = 16; // weekly calendar
    }
    //
//    printf("CPage-Size: %d, Entries: %.0f, Date-Range: %.0f, AvgWeek: %.4f, AvgMonth: %.4f\n",
//           page_size_tmp, entry_count_d, date_range, avg_entries_per_week_calendar, avg_entries_per_month_calendar);
    //
    return page_size_tmp;
}

/**
 * Returns the first entry of the given date that is inside the given calendar.
 * @param date_adt
 * @param calendar
 * @return
 */
int calmath_get_closest_index_from_left(int date_adt, InMemCalendar calendar) {
    int page_map_index = (date_adt / calendar.page_size) - calendar.first_page_offset;
    //
    if (page_map_index >= calendar.page_map_size) {
        int ret_val = -1 * ((int) calendar.dates_size) - 1;

        return ret_val;
    } else if (page_map_index < 0) {
        int ret_val = -1;
        return ret_val;
    }
    //
    int inclusive_start_index = calendar.page_map[page_map_index];
    int exclusive_end_index = page_map_index < calendar.page_map_size - 1 ? calendar.page_map[page_map_index + 1] : calendar.dates_size;
    //
    return coutil_left_binary_search(calendar.dates,
                                     inclusive_start_index,
                                     exclusive_end_index,
                                     date_adt);
}