/**
 * (C) ketteQ, Inc.
 */

#ifndef KETTEQ_INMEMORYCALENDAR_EXTENSION_KQ_IMCX_H
#define KETTEQ_INMEMORYCALENDAR_EXTENSION_KQ_IMCX_H
// Constants
#define TRANCHE_NAME "IMCX"
#define SHMEM_REQUESTED_MEMORY (1024 * 1024 * 1024 * 1) // 1 GB

// Default Loading Queries
#define DEF_Q1_GET_CALENDAR_IDS "select min(c.id), max(c.id) from plan.calendar c"
#define DEF_Q2_GET_CAL_ENTRY_COUNT "select cd.calendar_id, count(*), (select LOWER(ct.\"name\") from plan.calendar ct where ct.id = cd.calendar_id) \"name\" from plan.calendar_date cd group by cd.calendar_id order by cd.calendar_id asc;"
#define DEF_Q3_GET_ENTRIES "select cd.calendar_id, cd.\"date\" from plan.calendar_date cd order by cd.calendar_id asc, cd.\"date\" asc;"
#define DEF_Q_VALIDATE_SCHEMA "SELECT EXISTS (SELECT FROM information_schema.tables WHERE  table_schema = 'plan' AND table_name = 'calendar') AND EXISTS (SELECT FROM information_schema.tables WHERE table_schema = 'plan' AND table_name = 'calendar_date') \"valid\";"

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
#include "utils/guc.h"
#include "executor/spi.h"
#include <funcapi.h>
#include "storage/shmem.h"
#include "storage/lwlock.h"
#include "storage/ipc.h"

// IMCX Includes
#include "imcx/src/include/common.h"
#include "imcx/src/include/util.h"
#include "imcx/src/include/cache.h"
//
PG_MODULE_MAGIC;

// Postgres Extension Lifecycle
void _PG_init(void);
void _PG_fini(void);

// Internal Functions
void init_gucs();
static void init_shared_memory();
void ensure_cache_populated();
void validate_compatible_db();

// Exported Functions
Datum calendar_info(PG_FUNCTION_ARGS);
Datum calendar_invalidate(PG_FUNCTION_ARGS);
Datum calendar_report(PG_FUNCTION_ARGS);
Datum add_calendar_days_by_id(PG_FUNCTION_ARGS);
Datum add_calendar_days_by_name(PG_FUNCTION_ARGS);

#endif //KETTEQ_INMEMORYCALENDAR_EXTENSION_KQ_IMCX_H