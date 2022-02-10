//
// Created by gchiappe on 2022-02-01.
//

// #include <glib/gtypes.h>

#include "cache.h"
#include "inttypes.h"
#include "ketteQ_extension_1/src/common.h"

struct Calendar *calcache_calendars; // Calendar Store.
unsigned long calcache_calendar_count; // Calendar Store Size.
_Bool calcache_filled; // True if cache is filled.
GHashTable *calcache_calendar_names;

void glib_value_free(gpointer data) {
    free(data);
}

void calcache_init_calendars(unsigned long min_calendar_id, unsigned long max_calendar_id) {
    //
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
    calcache_calendar_names = g_hash_table_new_full
            (g_str_hash,
             g_str_equal,
             glib_value_free,
             glib_value_free);
    //
}

void calcache_init_calendar_entries(Calendar *calendar, unsigned long calendar_entry_count) {
    calendar->dates = malloc(calendar_entry_count * sizeof(int32));
    if (calendar->dates == NULL) {
        elog(ERROR, "Cannot allocate memory for date entries.");
    }
    calendar->dates_size = calendar_entry_count;
    elog(INFO, "%" PRIu64 " entries memory allocated for Calendar-Id: %d", calendar_entry_count, calendar->calendar_id);
}

static void displayhash(gpointer key, gpointer value, gpointer user_data) {
    elog(INFO, "Key: %s, Value: %s", (char*) key, (char*) value);
}

void calcache_report_calendar_names() {
    g_hash_table_foreach(calcache_calendar_names, displayhash, NULL);
}

void calcache_init_add_calendar_name(Calendar calendar, char *calendar_name) {
    // Convert Int to Str
    int num_len = snprintf(NULL, 0, "%d", calendar.calendar_id);
    char * id_str = malloc((num_len + 1) * sizeof(char));
    snprintf(id_str, num_len+1, "%d", calendar.calendar_id);
    //
    // TODO: Check how to save value as Int and not Str (char*)
    coutil_str_to_lowercase(calendar_name);
    char * calendar_name_ll = strdup(calendar_name);
    g_hash_table_insert(calcache_calendar_names, calendar_name_ll, id_str);
}

int calcache_get_calendar_by_name(char* calendar_name, Calendar * calendar) {
    coutil_str_to_lowercase(calendar_name);
    _Bool found = g_hash_table_contains(calcache_calendar_names, calendar_name);
    if (found) {
        char * calendar_id_str = g_hash_table_lookup(calcache_calendar_names, calendar_name);
        long calendar_id_l = strtol(calendar_id_str, NULL, 10);
        if (calendar_id_l > INT32_MAX) {
            // out of bounds.
            return -1;
        }
        * calendar = calcache_calendars[calendar_id_l - 1];
        return 0;
    }
    return -1;
}

void calcache_calculate_page_size(Calendar *calendar) {
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
    calendar->page_map = calloc(calendar->page_map_size, sizeof(int32));
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
    //MemoryContext old_context;
    // Testing MemoryContexts

    free(calcache_calendars);
    //MemoryContextSwitchTo(old_context);
    // End Free
    calcache_calendar_count = 0;
    calcache_filled = false;
    elog(INFO, "In-Mem: Cache Cleared.");
    return 0;
}