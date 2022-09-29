/**
 * (C) ketteQ, Inc.
 */

#include "cache.h"
#include "../common.h"
#include "inttypes.h"
#include <stdbool.h>
#include <stdio.h>

struct InMemCalendar *cacheCalendars; // InMemCalendar store.
unsigned long cacheCalendarCount; // InMemCalendar store Size.
bool cacheFilled; // True if cache is filled.
GHashTable *cacheCalendarNameHashTable; // Contains the calendar names.

/**
 * Support function for deallocating hash map pointers (keys and values).
 * Should not be exported.
 * @param data
 */
void glib_value_free(gpointer data) {
    free(data);
}

/**
 * This INIT will allocate the calendar struct in-mem storage.
 * @param min_calendar_id
 * @param max_calendar_id
 * @return
 */
int cacheInitCalendars(long min_calendar_id, long max_calendar_id) {
    if (min_calendar_id < 1) return -1;
    if (max_calendar_id < 1) return -1;
    //
    unsigned long calendar_count = max_calendar_id - min_calendar_id + 1;
    //
    cacheCalendars = (InMemCalendar*) calloc(calendar_count, sizeof(InMemCalendar));
    if (cacheCalendars == NULL) {
        // This happens when the OS can't give us that much memory.
        return -1;
    }
    //
    cacheCalendarCount = calendar_count;
    // Allocate the HashMap that will store the calendar names (str). This is a dynamic map.
    cacheCalendarNameHashTable = g_hash_table_new_full
            (g_str_hash,
             g_str_equal,
             glib_value_free,
             glib_value_free);
    //
    return 0;
}

int cacheInitCalendarEntries(InMemCalendar *calendar, long calendar_entry_count) {
    calendar->dates = malloc(calendar_entry_count * sizeof(int));
    if (calendar->dates == NULL) {
        // elog(ERROR, "Cannot allocate memory for date entries.");
        return -1;
    }
    calendar->dates_size = calendar_entry_count;
    // elog(INFO, "%" PRIu64 " entries memory allocated for InMemCalendar-Id: %d", calendar_entry_count, calendar->calendar_id);
    return 0;
}

static void stdc_display_hash(gpointer key, gpointer value, gpointer user_data) {
    // elog(INFO, "Key: %s, Value: %s", (char*) key, (char*) value);
    printf("Key: %s, Value: %s", (char*) key, (char*) value);
}

void cacheInitAddCalendarName(InMemCalendar calendar, char *calendar_name) {
    // Convert Int to Str
    int num_len = snprintf(NULL, 0, "%d", calendar.calendar_id);
    char * id_str = malloc((num_len + 1) * sizeof(char));
    snprintf(id_str, num_len+1, "%d", calendar.calendar_id);
    //
    // TODO: Check how to save the id value as Int and not Str (char*) -> similar to IntHashMap
    coutil_str_to_lowercase(calendar_name);
    char * calendar_name_ll = strdup(calendar_name);
    g_hash_table_insert(cacheCalendarNameHashTable, calendar_name_ll, id_str);
}

/**
 * Gets the calendar by its given name.
 * @param calendar_name pointer to string containing the calendar name
 * @param calendar pointer to calendar (found calendar will be pointed here)
 * @return 0 = SUCCESS, -1 = ERROR
 */
int cacheGetCalendarByName(char* calendar_name, InMemCalendar * calendar) {
    coutil_str_to_lowercase(calendar_name);
    _Bool found = g_hash_table_contains(cacheCalendarNameHashTable, calendar_name);
    if (found) {
        char * calendar_id_str = g_hash_table_lookup(cacheCalendarNameHashTable, calendar_name);
        long calendar_id_l = strtol(calendar_id_str, NULL, 10);
        if (calendar_id_l > INT32_MAX) {
            // out of bounds.
            return -1;
        }
        * calendar = cacheCalendars[calendar_id_l - 1];
        return 0;
    }
    return -1;
}

/**
 * Calculates and sets the page size for the given calendar pointer
 * @param calendar pointer to calendar
 * @return 0 = SUCCESS, -1 = ERROR
 */
int cacheInitPageSize(InMemCalendar *calendar) {
    int last_date = calendar->dates[calendar->dates_size - 1];
    int first_date = calendar->dates[0];
    int entry_count = calendar->dates_size;
    //
    int page_size_tmp = calmath_calculate_page_size(first_date, last_date, entry_count);
    //
    if (page_size_tmp == 0) {
        // Page size cannot be 0, cannot be calculated.
        return -1;
    }
    //
    calendar->page_size = page_size_tmp;
    calendar->first_page_offset = first_date / page_size_tmp;
    // Allocation of the page map
    int page_end_index = calendar->dates[calendar->dates_size - 1] / calendar->page_size;
    calendar->page_map_size = page_end_index - calendar->first_page_offset + 1;
    // Assign memory and set the values to 0.
    calendar->page_map = calloc(calendar->page_map_size, sizeof(int));
    //
    // Calculate Page Map
    int prev_page_index = 0;
    // int prev_date = -1; // This will be required if we check that the array is ordered.
    for (int date_index = 0; date_index < calendar->dates_size; date_index++) {
        int curr_date = calendar->dates[date_index];
        // TODO: Check if the date array is ordered or not, we assume it is.
        int page_index = (curr_date / calendar->page_size) - calendar->first_page_offset;
        while (prev_page_index < page_index) {
            calendar->page_map[++prev_page_index] = date_index;
        }
    }
    return 0;
}

/**
 * Main Extension Function
 * Adds (or subtracts) intervals of the given date based on the given calendar type (monthly, weekly, etc)
 * @param input_date input date in DateADT int representation, can be negative
 * @param interval interval to calculate, can be negative
 * @param calendar pointer to calendar to use as database
 * @param first_date_idx pointer to the first date index (NULLABLE)
 * @param result_date_idx pointer to the result date index (NULLABLE)
 * @return RESULT DATE = SUCCESS, INT32_MAX = Out of bounds
 */
int cacheAddCalendarDays(
        int input_date,
        int interval,
        InMemCalendar calendar,
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
 * @return -1 if error, 0 if OK.
 */
int cacheInvalidate() {
    int cc;
    //
    if (cacheCalendarCount == 0) {
        return -1;
    }
    // Free Entries 1st
    for (cc = 0; cc < cacheCalendarCount; cc++) {
        free(cacheCalendars[cc].dates);
    }
    // Then Free Store
    free(cacheCalendars);
    // Then Destroy Hashmap
    g_hash_table_destroy(cacheCalendarNameHashTable);
    // Reset Control Vars
    cacheCalendarCount = 0;
    cacheFilled = false;
    // Done
    return 0;
}