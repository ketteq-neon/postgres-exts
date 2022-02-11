/**
 * (C) ketteQ, Inc.
 */

#ifndef KETTEQ_POSTGRESQL_EXTENSIONS_CACHE_H
#define KETTEQ_POSTGRESQL_EXTENSIONS_CACHE_H

#include "../calendar.h"
#include <glib.h>
#include <stdbool.h>

extern struct Calendar * calcache_calendars;
extern unsigned long calcache_calendar_count;
extern bool calcache_filled;
extern GHashTable * calcache_calendar_names;

int calcache_init_calendars(long min_calendar_id, long max_calendar_id);
int calcache_init_calendar_entries(Calendar * calendar, long calendar_entry_count);
void calcache_report_calendar_names_stdc();
void calcache_report_calendar_names(GHFunc display_func);
void calcache_init_add_calendar_name(Calendar calendar, char *calendar_name);
int calcache_get_calendar_by_name(char* calendar_name, Calendar * calendar);
int calcache_init_page_size(Calendar * calendar);
int calcache_add_calendar_days(
        int input_date,
        int interval,
        Calendar calendar,
        int * first_date_idx,
        int * result_date_idx
);

// int pg_calcache_report();
int calcache_invalidate();

#endif //KETTEQ_POSTGRESQL_EXTENSIONS_CACHE_H
