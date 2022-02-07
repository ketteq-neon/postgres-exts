//
// Created by gchiappe on 2022-01-26.
//

#include "verysimple.h"
#include <postgres.h>
#include <fmgr.h>

PG_MODULE_MAGIC;

PG_FUNCTION_INFO_V1(de2_sum);

Datum de2_sum(PG_FUNCTION_ARGS) {
    int32 arg1 = PG_GETARG_INT32(0);
    int32 arg2 = PG_GETARG_INT32(1);

    PG_RETURN_INT32(arg1 + arg2);
}

