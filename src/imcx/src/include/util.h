/**
 * (C) ketteQ, Inc.
 */

#ifndef KETTEQ_POSTGRESQL_EXTENSIONS_UTIL_H
#define KETTEQ_POSTGRESQL_EXTENSIONS_UTIL_H

#include <sys/types.h>
#include <c.h>

/**
 * Formats a double number into a string
 * @param target pointer to target string
 * @param number double number
 * @param precision format decimal precision
 */
void double_to_str(char *target, double number, int32 precision);
/**
 * Formats an int number into a string
 * @param target pointer to target string
 * @param source integer number
 */
void int32_to_str(char *target, int source);
/**
 * Formats a boolean into a string
 * @param target pointer to target string
 * @param source boolean variable
 */
void bool_to_str(char *target, bool source);
/**
 * Replaces the chars of the given pointer with their corresponding lowercase char.
 * @param string
 */
void str_to_lowercase(char *string);

#endif //KETTEQ_POSTGRESQL_EXTENSIONS_UTIL_H
