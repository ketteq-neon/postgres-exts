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
//
PG_MODULE_MAGIC;

void _PG_init(void);
void _PG_fini(void);

#endif //KETTEQ_INMEMORYCALENDAR_EXTENSION_KQ_IMCX_H