#ifndef KETTEQ_POSTGRESQL_EXTENSIONS_CACHE_H
#define KETTEQ_POSTGRESQL_EXTENSIONS_CACHE_H

//#include "postgres.h"
//#include "utils/memutils.h"
//#include <glib.h>
//#include "math.h"

#include "../calendar.h"
#include <glib.h>

extern struct Calendar * calcache_calendars;
extern unsigned long calcache_calendar_count;
extern _Bool calcache_filled;
extern GHashTable * calcache_calendar_names;

void calcache_init_calendars(unsigned long min_calendar_id, unsigned long max_calendar_id);
void calcache_init_calendar_entries(Calendar * calendar, unsigned long calendar_entry_count);
void calcache_report_calendar_names();
void calcache_init_add_calendar_name(Calendar calendar, char *calendar_name);
int calcache_get_calendar_by_name(char* calendar_name, Calendar * calendar);
void calcache_calculate_page_size(Calendar * calendar);
int calcache_add_calendar_days(int input_date, int interval, Calendar calendar);

int calcache_report();
int calcache_invalidate();

#endif //KETTEQ_POSTGRESQL_EXTENSIONS_CACHE_H
