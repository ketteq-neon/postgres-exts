//
// Created by gchiappe on 2022-02-01.
//

#include "math.h"
#include "common/util.h"

/**
 * @param first_date
 * @param last_date
 * @param entry_count
 * @return
 */
uint32 calmath_calculate_page_size(int32 first_date, int32 last_date, uint32 entry_count) {

    return 90;

    //
    uint32 page_size_tmp = 0;
    //
    double date_range = last_date - first_date;
    double type_guesser = date_range / (double) (entry_count - 1);
    if (type_guesser > 364 && type_guesser < 366) page_size_tmp = 384; // yearly
    if (type_guesser > 29 && type_guesser < 31) page_size_tmp = 32; // monthly
    // if (type_guesser > 14 && type_guesser < 16) page_size_tmp = 16; // TODO: quarterly, test
    // if (type_guesser < 14) page_size_tmp = 8; // TODO: weekly, test
    //
    elog(INFO, "Page Size: %d", page_size_tmp);
    //
    return page_size_tmp;
}

int32 calmath_get_first_entry_index(int32 date_adt, Calendar calendar) {
    //
    int32 page_map_index = (date_adt / calendar.page_size) - calendar.first_page_offset;
    //
    elog(INFO, "Page-Map-Index: %d, Page-Map-Size: %d", page_map_index, calendar.page_map_size);
    elog(INFO, "Date: %d, Offset: %d", date_adt, calendar.first_page_offset);
    if (page_map_index >= calendar.page_map_size) {
        int32 ret_val = -1 * ((int32) calendar.dates_size) - 1;
        elog(INFO, "page_map_index (%d) > page_map_size (%d); ret_val = %d",
             page_map_index,
             calendar.page_map_size,
             ret_val);
        return ret_val;
    } else if (page_map_index < 0) {
        int32 ret_val = -1;
        elog(INFO, "page_map_index (%d) < 0 (negative index); ret_val = %d",
             page_map_index,
             ret_val);
        return ret_val;
    }
    return page_map_index;

//    int32 inclusive_start_index = calendar.page_map[page_map_index];
//    int32 exclusive_end_index = page_map_index < calendar.page_map_size - 1 ? calendar.page_map[page_map_index + 1] :
//            calendar.dates_size;
////    elog(INFO, "Page-Finder: Date: %d, Page-Size: %d, FP-Offset: %d, Found In Start Index: %d",
////         date_adt, page_size, first_page_offset, page_map_index);
//
//    elog(INFO, "RecursiveBinarySearch: IncStartIdx: %d, ExcEndIdx: %d", inclusive_start_index, exclusive_end_index);
//    int32 found_index = coutil_binary_search(calendar.dates,
//                                             inclusive_start_index,
//                                             exclusive_end_index,
//                                             date_adt);
//    elog(INFO, "BinarySearchResult: %d", found_index);
//    return found_index;
}