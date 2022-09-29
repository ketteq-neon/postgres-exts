/**
 * (C) KetteQ, Inc.
 */

#ifndef KETTEQ_POSTGRESQL_EXTENSIONS_MATH_H
#define KETTEQ_POSTGRESQL_EXTENSIONS_MATH_H

#include "../calendar.h"

#define DATE_POS_CONTAINS 0
#define PAST DATEVAL_NOBEGIN
#define FUTURE DATEVAL_NOEND

int calmath_calculate_page_size(int first_date, int last_date, int entry_count);
int calmath_get_closest_index_from_left(int date_adt, InMemCalendar calendar);

#endif //KETTEQ_POSTGRESQL_EXTENSIONS_MATH_H
