//
// Created by gchiappe on 2022-02-01.
//

#ifndef KETTEQ_POSTGRESQL_EXTENSIONS_MATH_H
#define KETTEQ_POSTGRESQL_EXTENSIONS_MATH_H

#include "postgres.h"
#include "utils/date.h"
#include "cache.h"

#define DATE_POS_CONTAINS 0
#define PAST DATEVAL_NOBEGIN
#define FUTURE DATEVAL_NOEND

uint32 calmath_calculate_page_size(int32 first_date, int32 last_date, uint32 entry_count);
int32 calmath_get_first_entry_index(int32 date_adt, Calendar calendar);

#endif //KETTEQ_POSTGRESQL_EXTENSIONS_MATH_H
