/**
 * (C) ketteQ, Inc.
 */

#ifndef KETTEQ_POSTGRESQL_EXTENSIONS_CACHE_H
#define KETTEQ_POSTGRESQL_EXTENSIONS_CACHE_H

#include "../calendar.h"
#include <glib.h>
#include <stdbool.h>

extern struct InMemCalendar * cacheCalendars;
extern unsigned long cacheCalendarCount;
extern bool cacheFilled;
extern GHashTable * cacheCalendarNameHashTable;
extern char * cacheCalendarFindCalendarName;

int cacheInitCalendars(long min_calendar_id, long max_calendar_id);
int cacheInitCalendarEntries(InMemCalendar * calendar, long calendar_entry_count);
void cacheInitAddCalendarName(InMemCalendar calendar, char *calendar_name);
int cacheGetCalendarByName(char* calendar_name, InMemCalendar * calendar);
int cacheInitPageSize(InMemCalendar * calendar);
int cacheAddCalendarDays(
        int input_date,
        int interval,
        InMemCalendar calendar,
        int * first_date_idx,
        int * result_date_idx
);

// int pg_calcache_report();
int cacheInvalidate();

#endif //KETTEQ_POSTGRESQL_EXTENSIONS_CACHE_H
