/**
 * (C) KetteQ, Inc.
 */

#ifndef KETTEQ_POSTGRESQL_EXTENSIONS_UTIL_H
#define KETTEQ_POSTGRESQL_EXTENSIONS_UTIL_H

#include <sys/types.h>

char * convert_double_to_str(double number, int precision);
char * convert_int_to_str(int number);
char * convert_long_to_str(long number);
char * convert_u_long_to_str(ulong number);
char * convert_u_int_to_str(uint number);
char * str_to_lowercase(const char * string);

#endif //KETTEQ_POSTGRESQL_EXTENSIONS_UTIL_H
