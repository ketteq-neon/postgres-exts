//
// Created by gchiappe on 2022-02-01.
//

#include "cache.h"
#include "inttypes.h"

struct Calendar * calcache_calendars; // Calendar Store.
uint64 calcache_calendar_count; // Calendar Store Size.
bool calcache_filled; // True if cache is filled.

void calcache_init_calendars(uint64 min_calendar_id, uint64 max_calendar_id) {
    MemoryContext prev_memory_context;
    prev_memory_context = MemoryContextSwitchTo(TopMemoryContext);
    //
    //uint64 calendar_count = max_calendar_id - min_calendar_id;
    uint64 calendar_count = max_calendar_id;
    //
    elog(INFO, "Will allocate memory for %" PRIu64 " calendars.", calendar_count);
    calcache_calendars = malloc(calendar_count * sizeof(struct Calendar));
    if (calcache_calendars == NULL) {
        elog(ERROR, "Cannot allocate memory for calendars.");
    }
    elog(INFO, "Calendar memory allocated. Now, entries must be allocated.");
    calcache_calendar_count = calendar_count;
    //
    MemoryContextSwitchTo(prev_memory_context);
}

void calcache_init_calendar_entries(Calendar * calendar, uint64 calendar_entry_count) {
    // uint64 calendar_index = calendar_id - 1;
    MemoryContext prev_memory_context;
    prev_memory_context = MemoryContextSwitchTo(TopMemoryContext);
    calendar->dates = malloc(calendar_entry_count * sizeof (int32));
    if (calendar->dates == NULL) {
        elog(ERROR, "Cannot allocate memory for date entries.");
    }
    calendar->dates_size = calendar_entry_count;
    MemoryContextSwitchTo(prev_memory_context);
    elog(INFO, "%" PRIu64 " entries memory allocated for Calendar-Id: %d", calendar_entry_count, calendar->calendar_id);
}

void calcache_calculate_page_size(Calendar * calendar) {
    elog(INFO, "Calculating Page Size for Calendar-Id: %d", calendar->calendar_id);
    int32 last_date = calendar->dates[calendar->dates_size - 1];
    int32 first_date = calendar->dates[0];
    uint32 entry_count = calendar->dates_size;
    //
    uint32 page_size_tmp = calmath_calculate_page_size(first_date, last_date, entry_count);
    //
    if (page_size_tmp == 0) {
        elog(ERROR, "Cannot calculate page size.");
    }
    //
    calendar->page_size = page_size_tmp;
    calendar->first_page_offset = first_date / page_size_tmp;
    // Allocate Page Map
    int32 page_end_index = calendar->dates[calendar->dates_size - 1] / calendar->page_size;
    calendar->page_map_size = page_end_index - calendar->first_page_offset + 1;
    //
    MemoryContext prev_ctx = MemoryContextSwitchTo(TopMemoryContext);
    calendar->page_map = calloc( calendar->page_map_size , sizeof(int32));
    MemoryContextSwitchTo(prev_ctx);
    //
    // Calculate Page Map
    int32 prev_page_index = 0;
    int32 prev_date = -1;
    // TODO: Check Functioning
    for (int32 date_index = 0; date_index < calendar->dates_size; date_index++) {
        int32 curr_date = calendar->dates[date_index];
        // Checkings ommited for now...
        int32 page_index = (curr_date / calendar->page_size) - calendar->first_page_offset;
        while (prev_page_index < page_index) {
            calendar->page_map[++prev_page_index] = date_index;
        }
    }
    elog(INFO, "Page Size: %d, FP Offset: %d, Page Map Size: %d",
         calendar->page_size, calendar->first_page_offset, calendar->page_map_size);
}

int32 calcache_add_calendar_days(int32 input_date, int32 interval, Calendar calendar) {
    // Find the interval
    int32 first_date_index = calmath_get_first_entry_index(
            input_date,
            calendar
            );
    elog(INFO, "Cal-Id: %d, First Date Index: %d, Interval: %d, Input-Date: %d",
         calendar.calendar_id, first_date_index, interval, input_date);
    // Now try to get the corresponding date of requested interval
    int32 result_date_index = first_date_index + interval;
    //
    if (result_date_index >= 0) {
        if (first_date_index < 0) {
            // elog(ERROR, "Date is in the past.");
            return 0;
        }
        if (result_date_index >= calendar.dates_size) {
            // elog(ERROR, "Result-Date is in the future.");
            return INT32_MAX;
        }
    } else {
        if (result_date_index < 0) {
            // elog(ERROR, "Result-Date is in the past.");
            return 0; // TODO: Align on the first past date.
        }
        if (first_date_index >= calendar.dates_size) {
            // elog(ERROR, "Date is in the future.");
            return INT32_MAX;
        }
    }
    return calendar.dates[result_date_index];
}

/**
 * Reports the contents of the In-Mem Cache, it will display as Log Messages (INFO).
 * @return
 */
int calcache_report() {
    if (!calcache_filled) {
        elog(ERROR, "Cannot Report Cache Because is Empty.");
    }
    elog(INFO, "Calendar Count: %" PRIu64, calcache_calendar_count);
    for (uint64 i = 0; i < calcache_calendar_count; i++) {
        //elog(INFO, "In-Mem[CalId: %d] = %d", calcache_calendar_ids[i], calcache_date_store[i]);
        Calendar curr_calendar = calcache_calendars[i];
        elog(INFO, "Calendar-Id: %d, "
                   "Entries: %d, "
                   "Page Map Size: %d, "
                   "Page Size: %d",
                   curr_calendar.calendar_id,
                   curr_calendar.dates_size,
                   curr_calendar.page_map_size,
                   curr_calendar.page_size);
        for (uint32 j = 0; j < curr_calendar.dates_size; j++) {
            elog(INFO, "Entry[%d]: %d", j, curr_calendar.dates[j]);
        }
        for (uint32 j = 0; j < curr_calendar.page_map_size; j++) {
            elog(INFO, "PageMap Entry[%d]: %d", j, curr_calendar.page_map[j]);
        }
    }
    return 0;
}

/**
 * Clears the Cache
 * @return -1 if error, 0 if ok.
 */
int calcache_invalidate() {
    if (calcache_calendar_count == 0) {
        elog(INFO, "In-Mem: Cache Does Not Exists.");
        return -1;
    }
    MemoryContext old_context;
    // Testing MemoryContexts
    old_context = MemoryContextSwitchTo(TopMemoryContext);
    free(calcache_calendars);
    MemoryContextSwitchTo(old_context);
    // End Free
    calcache_calendar_count = 0;
    calcache_filled = false;
    elog(INFO, "In-Mem: Cache Cleared.");
    return 0;
}