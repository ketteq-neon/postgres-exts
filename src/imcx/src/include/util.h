/**
 * (C) KetteQ, Inc.
 */

#ifndef KETTEQ_POSTGRESQL_EXTENSIONS_UTIL_H
#define KETTEQ_POSTGRESQL_EXTENSIONS_UTIL_H

#include <sys/types.h>
#include <c.h>

char *convert_double_to_str (double number, int32 precision);
void int_to_str(char* target, int source);
char *convert_int_to_str (int32 number);
char *convert_long_to_str (long number);
char *convert_u_long_to_str (unsigned long number);
char *convert_u_int_to_str (uint32 number);

/**
 * Replaces the chars of the given pointer with their corresponding lowercase char.
 * @param string
 */
void str_to_lowercase (char *string);

#endif //KETTEQ_POSTGRESQL_EXTENSIONS_UTIL_H
