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
//#define MAX_CALENDAR_COUNT 1024 * 1024 * 1024 // 1 GB
#define MAX_CALENDAR_COUNT 50 // 1 GB
#define DEF_DEBUG_LOG_LEVEL DEBUG1
#define QUERY_GET_CAL_MIN_MAX_ID "select min(s.id), max(s.id) from ketteq.slice_type s"
#define QUERY_GET_CAL_ENTRY_COUNT "select s.slice_type_id, count(*), (select LOWER(st.\"name\") from ketteq.slice_type st where st.id = s.slice_type_id) \"name\" from ketteq.slice s group by s.slice_type_id order by s.slice_type_id asc;"
#define QUERY_GET_CAL_GET_ENTRIES "select s.slice_type_id, s.start_on from ketteq.slice s order by s.slice_type_id asc, s.start_on asc;"

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

void _PG_init (void);
void _PG_fini (void);

void init_gucs ();
static void init_shared_memory ();
void load_cache_concrete ();

Datum calendar_info (PG_FUNCTION_ARGS);
Datum calendar_invalidate (PG_FUNCTION_ARGS);
Datum calendar_report (PG_FUNCTION_ARGS);
Datum add_calendar_days_by_id (PG_FUNCTION_ARGS);
Datum add_calendar_days_by_name (PG_FUNCTION_ARGS);

#endif //KETTEQ_INMEMORYCALENDAR_EXTENSION_KQ_IMCX_H