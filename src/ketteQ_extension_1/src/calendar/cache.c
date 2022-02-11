/**
 * (C) ketteQ, Inc.
 */

#include "cache.h"
#include "../common.h"
#include "inttypes.h"
#include <stdbool.h>
#include <stdio.h>

struct Calendar *calcache_calendars; // Calendar Store.
unsigned long calcache_calendar_count; // Calendar Store Size.
bool calcache_filled; // True if cache is filled.
GHashTable *calcache_calendar_names;

void glib_value_free(gpointer data) {
    free(data);
}

int calcache_init_calendars(unsigned long min_calendar_id, long max_calendar_id) {
    //
    unsigned long calendar_count = max_calendar_id;
    //
    // elog(INFO, "Will allocate memory for %" PRIu64 " calendars.", calendar_count);
    calcache_calendars = malloc(calendar_count * sizeof(struct Calendar));
    if (calcache_calendars == NULL) {
        //elog(ERROR, "Cannot allocate memory for calendars.");
        return -1;
    }
    // elog(INFO, "Calendar memory allocated. Now, entries must be allocated.");
    calcache_calendar_count = calendar_count;
    //
    calcache_calendar_names = g_hash_table_new_full
            (g_str_hash,
             g_str_equal,
             glib_value_free,
             glib_value_free);
    //
    return 0;
}

int calcache_init_calendar_entries(Calendar *calendar, long calendar_entry_count) {
    calendar->dates = malloc(calendar_entry_count * sizeof(int));
    if (calendar->dates == NULL) {
        // elog(ERROR, "Cannot allocate memory for date entries.");
        return -1;
    }
    calendar->dates_size = calendar_entry_count;
    // elog(INFO, "%" PRIu64 " entries memory allocated for Calendar-Id: %d", calendar_entry_count, calendar->calendar_id);
    return 0;
}

static void stdc_display_hash(gpointer key, gpointer value, gpointer user_data) {
    // elog(INFO, "Key: %s, Value: %s", (char*) key, (char*) value);
    printf("Key: %s, Value: %s", (char*) key, (char*) value);
}

void calcache_report_calendar_names_stdc() {
    g_hash_table_foreach(calcache_calendar_names, stdc_display_hash, NULL);
}


void calcache_report_calendar_names(GHFunc display_func) {
    g_hash_table_foreach(calcache_calendar_names, display_func, NULL);
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

int calcache_calculate_page_size(Calendar *calendar) {
    // elog(INFO, "Calculating Page Size for Calendar-Id: %d", calendar->calendar_id);
    int last_date = calendar->dates[calendar->dates_size - 1];
    int first_date = calendar->dates[0];
    int entry_count = calendar->dates_size;
    //
     int page_size_tmp = calmath_calculate_page_size(first_date, last_date, entry_count);
    //int page_size_tmp = 90;
    //
    if (page_size_tmp == 0) {
        // elog(ERROR, "Cannot calculate page size.");
        return -1;
    }
    //
    calendar->page_size = page_size_tmp;
    calendar->first_page_offset = first_date / page_size_tmp;
    // Allocate Page Map
    int page_end_index = calendar->dates[calendar->dates_size - 1] / calendar->page_size;
    calendar->page_map_size = page_end_index - calendar->first_page_offset + 1;
    //
    calendar->page_map = calloc(calendar->page_map_size, sizeof(int));
    //
    // Calculate Page Map
    int prev_page_index = 0;
    int prev_date = -1;
    // TODO: Check Functioning
    for (int date_index = 0; date_index < calendar->dates_size; date_index++) {
        int curr_date = calendar->dates[date_index];
        // Checkings ommited for now...
        int page_index = (curr_date / calendar->page_size) - calendar->first_page_offset;
        while (prev_page_index < page_index) {
            calendar->page_map[++prev_page_index] = date_index;
        }
    }
    return 0;
}

/**
 * Adds (or subtracts) intervals of the first entry where the given date is located.
 * @param input_date
 * @param interval
 * @param calendar
 * @param first_date_idx
 * @param result_date_idx
 * @return
 */
int calcache_add_calendar_days(
        int input_date,
        int interval,
        Calendar calendar,
        // Optional, set to NULL if it will not be used.
        int * first_date_idx,
        int * result_date_idx
        ) {
    // Find the interval
    int first_date_index = calmath_get_closest_index_from_left(input_date, calendar);
    // Now try to get the corresponding date of requested interval
    int result_date_index = first_date_index + interval;
    //
    // Now check if inside boundaries.
    if (result_date_index >= 0) {
        if (first_date_index < 0) {
            return calendar.dates[0]; // Returns the first date of the calendar.
        }
        if (result_date_index >= calendar.dates_size) {
            return INT32_MAX; // Returns infinity+.
        }
    } else {
        if (result_date_index < 0) {
            return calendar.dates[0]; // Returns the first date of the calendar.
        }
        if (first_date_index >= calendar.dates_size) {
            return INT32_MAX; // Returns infinity+.
        }
    }
    // This can be useful for reporting or debugging.
    if (first_date_idx != NULL) {
        * first_date_idx = first_date_index;
    }
    if (result_date_idx != NULL) {
        * result_date_idx = result_date_index;
    }
    //
    return calendar.dates[result_date_index];
}

/**
 * Clears the Cache
 * @return -1 if error, 0 if ok.
 */
int calcache_invalidate() {
    int cc;
    //
    if (calcache_calendar_count == 0) {
        return -1;
    }
    // Free Entries 1st
    for (cc = 0; cc < calcache_calendar_count; cc++) {
        free(calcache_calendars[cc].dates);
    }
    // Then Free Store
    free(calcache_calendars);
    // Then Destroy Hashmap
    g_hash_table_destroy(calcache_calendar_names);
    // Reset Control Vars
    calcache_calendar_count = 0;
    calcache_filled = false;
    // Done
    return 0;
}