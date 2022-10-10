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
/**
 * Copies the string and replaces the chars of the given pointer with their corresponding lowercase char.
 * @param string
 */
char * str_to_lowercase(char * string);
/**
 * Replaces the chars of the given pointer with their corresponding lowercase char.
 * @param string
 */
void str_to_lowercase_self (char *string);

#endif //KETTEQ_POSTGRESQL_EXTENSIONS_UTIL_H
