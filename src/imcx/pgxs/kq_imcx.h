/**
 * (C) KetteQ, Inc.
 */

#ifndef KETTEQ_INMEMORYCALENDAR_EXTENSION_KQ_IMCX_H
#define KETTEQ_INMEMORYCALENDAR_EXTENSION_KQ_IMCX_H
// Project Info (Provided by CMAKE)
#ifndef CMAKE_HEADER_DEFINITIONS
#define CMAKE_VERSION "0.0.0-DEV"

#endif
// Constants
#define TRANCHE_NAME "IMCX"
#define SHARED_MEMORY_DEF 1024 * 1024 * 1024
#define DEF_DEBUG_LOG_LEVEL INFO
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
#include "storage/shmem.h"
#include "storage/lwlock.h"
// IMCX Includes
#include "../src/common.h"
#include "../src/common/util.h"
#include "../src/calendar/cache.h"
//
PG_MODULE_MAGIC;

void _PG_init (void);
void _PG_fini (void);

void init_shared_memory ();
void load_cache_concrete ();

Datum imcx_info (PG_FUNCTION_ARGS);
Datum imcx_invalidate (PG_FUNCTION_ARGS);
Datum imcx_report (PG_FUNCTION_ARGS);
Datum imcx_add_calendar_days_by_id (PG_FUNCTION_ARGS);
Datum imcx_add_calendar_days_by_calendar_name (PG_FUNCTION_ARGS);

#endif //KETTEQ_INMEMORYCALENDAR_EXTENSION_KQ_IMCX_H