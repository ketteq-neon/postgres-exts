/**
 * (C) KetteQ, Inc.
 */

#ifndef KETTEQ_INMEMORYCALENDAR_EXTENSION_KQ_IMCX_H
#define KETTEQ_INMEMORYCALENDAR_EXTENSION_KQ_IMCX_H
// Project Info (Provided by CMAKE)
#ifndef CMAKE_HEADER_DEFINITIONS
#define CMAKE_VERSION "0.0.0"
#endif
// OS Includes
#include <stdio.h>
// Postgres Includes
#include <postgres.h>
#include <fmgr.h>
#include <inttypes.h>
#include <miscadmin.h>
#include "utils/date.h"
#include "utils/builtins.h"
#include "utils/memutils.h"
#include "executor/spi.h"
#include <funcapi.h>
// IMCX Includes
#include "../src/calendar.h"
#include "../src/common/util.h"
//
PG_MODULE_MAGIC;

extern bool loadingCache;

void _PG_init(void);
void _PG_fini(void);
void load_all_slices();

Datum imcx_info(PG_FUNCTION_ARGS);
Datum imcx_invalidate(PG_FUNCTION_ARGS);
Datum imcx_report(PG_FUNCTION_ARGS);
Datum imcx_add_calendar_days_by_id(PG_FUNCTION_ARGS);
Datum imcx_add_calendar_days_by_calendar_name(PG_FUNCTION_ARGS);
Datum kq_load_all_calendars(PG_FUNCTION_ARGS);

#endif //KETTEQ_INMEMORYCALENDAR_EXTENSION_KQ_IMCX_H