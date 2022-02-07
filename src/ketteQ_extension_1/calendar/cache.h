//
// Created by gchiappe on 2022-02-01.
//

#ifndef KETTEQ_POSTGRESQL_EXTENSIONS_CACHE_H
#define KETTEQ_POSTGRESQL_EXTENSIONS_CACHE_H

#include "postgres.h"
#include "utils/memutils.h"
#include <glib.h>

// TODO: Map the calendar name (str) to the calendar_idx
// calendar_name from users can be uppercase, lowercase or mixed -> lookup and get the calendar_idx

typedef struct Calendar {
    uint32 calendar_id;

    int32 * dates;
    uint32 dates_size;

    uint32 page_size;
    int32 first_page_offset;

    int32 * page_map;
    int32 page_map_size;
} Calendar;

// TODO: create a map for the calendar_names (for the lookup).


#include "math.h"

/**
 * Get the minValue and maxValue of the calendar_id and allocate the struct array. From Query.
 * If I get MaxValue = 10,000, allocate Max-Min structs.
 * Index of the array = Calendar_id - Min_Calendar_id
 *
 * -- then allocate the entries
 *
 * Go thru another query using the calendar_id and allocate the array with the size of the result, then
 * copy dates inside.
 */



extern struct Calendar * calcache_calendars;
extern uint64 calcache_calendar_count;
extern bool calcache_filled;
extern GHashTable * calcache_calendar_names;

void calcache_init_calendars(uint64 min_calendar_id, uint64 max_calendar_id);
void calcache_init_calendar_entries(Calendar * calendar, uint64 calendar_entry_count);
void calcache_report_calendar_names();
void calcache_init_add_calendar_name(Calendar calendar, char *calendar_name);
void calcache_calculate_page_size(Calendar * calendar);
int32 calcache_add_calendar_days(int32 input_date, int32 interval, Calendar calendar);

int calcache_report();
int calcache_invalidate();

#endif //KETTEQ_POSTGRESQL_EXTENSIONS_CACHE_H
