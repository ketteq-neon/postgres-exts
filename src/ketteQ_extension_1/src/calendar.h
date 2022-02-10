//
// Created by gchiappe on 2022-02-10.
//

#ifndef KETTEQ_POSTGRESQL_EXTENSIONS_CALENDAR_H
#define KETTEQ_POSTGRESQL_EXTENSIONS_CALENDAR_H

typedef struct Calendar {
    unsigned int calendar_id;

    int * dates;
    unsigned int dates_size;

    unsigned int page_size;
    int first_page_offset;

    int * page_map;
    int page_map_size;
} Calendar;

#include "calendar/cache.h"
#include "calendar/math.h"

#endif //KETTEQ_POSTGRESQL_EXTENSIONS_CALENDAR_H
