//
// Created by gchiappe on 2022-02-03.
//

#ifndef KETTEQ_POSTGRESQL_EXTENSIONS_UTIL_H
#define KETTEQ_POSTGRESQL_EXTENSIONS_UTIL_H

#include "postgres.h"
ptrdiff_t coutil_uint64_to_ptrdiff(uint64 input);
int32 coutil_binary_search(int32 arr[], int32 left, int32 right, int32 value);

#endif //KETTEQ_POSTGRESQL_EXTENSIONS_UTIL_H
