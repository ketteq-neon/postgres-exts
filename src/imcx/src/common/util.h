/**
 * (C) ketteQ, Inc.
 */

#ifndef KETTEQ_POSTGRESQL_EXTENSIONS_UTIL_H
#define KETTEQ_POSTGRESQL_EXTENSIONS_UTIL_H

#include <stddef.h>

// ptrdiff_t coutil_uint64_to_ptrdiff(unsigned long input); // To Be Deleted
int coutil_binary_search(int arr[], int left, int right, int value);
int coutil_left_binary_search(const int arr[], int left, int right, int value);
void coutil_str_to_lowercase(char * data);

#endif //KETTEQ_POSTGRESQL_EXTENSIONS_UTIL_H
