//
// Created by gchiappe on 2022-02-01.
//

#include "math.h"
#include "../common.h"

/**
 * Calculates the page size of the calendar depending on the intervals. Calendar entries must be ordered ASC.
 * @param first_date Earliest date interval in calendar.
 * @param last_date Latest date interval in calendar.
 * @param entry_count Count of intervals in the calendar.
 * @return page size as integer.
 */
unsigned int calmath_calculate_page_size(int first_date, int last_date, int entry_count) {
    // TODO: Check page size calculation logic for other kind of calendars.
    return 90;
    //
    unsigned int page_size_tmp = 0;
    //
    double date_range = last_date - first_date;
    double type_guesser = date_range / (double) (entry_count - 1);
    if (type_guesser > 364 && type_guesser < 366) page_size_tmp = 384; // yearly
    if (type_guesser > 29 && type_guesser < 31) page_size_tmp = 32; // monthly
    // if (type_guesser > 14 && type_guesser < 16) page_size_tmp = 16; // TODO: quarterly, test
    // if (type_guesser < 14) page_size_tmp = 8; // TODO: weekly, test
    //
    // elog(INFO, "Page Size: %d", page_size_tmp);
    //
    return page_size_tmp;
}

/**
 * Returns the first entry of the given date that is inside the given calendar.
 * @param date_adt
 * @param calendar
 * @return
 */
int calmath_get_first_entry_index(int date_adt, Calendar calendar) {
    //
    int page_map_index = (date_adt / calendar.page_size) - calendar.first_page_offset;
    //
    if (page_map_index >= calendar.page_map_size) {
        int ret_val = -1 * ((int) calendar.dates_size) - 1;
        return ret_val;
    } else if (page_map_index < 0) {
        int ret_val = -1;
        return ret_val;
    }
    return page_map_index;
}