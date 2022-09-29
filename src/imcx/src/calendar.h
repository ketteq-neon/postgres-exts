/**
 * (C) KetteQ, Inc.
 */

#ifndef KETTEQ_POSTGRESQL_EXTENSIONS_CALENDAR_H
#define KETTEQ_POSTGRESQL_EXTENSIONS_CALENDAR_H

typedef struct InMemCalendar {
    unsigned int calendar_id;

    int * dates;
    long dates_size;

    int page_size;
    int first_page_offset;

    int * page_map;
    int page_map_size;
} InMemCalendar;

#include "calendar/cache.h"
#include "calendar/math.h"

#endif //KETTEQ_POSTGRESQL_EXTENSIONS_CALENDAR_H
