/**
 * (C) KetteQ, Inc.
 */

#ifndef KETTEQ_POSTGRESQL_EXTENSIONS_CACHE_H
#define KETTEQ_POSTGRESQL_EXTENSIONS_CACHE_H

#include <sys/types.h>
#include <stdbool.h>
#include <stdio.h>
#include <glib.h>
#include <stdlib.h>
#include <string.h>

#include "math.h"
#include "../common.h"
#include "inttypes.h"

int cache_init(IMCX * imcx, ulong min_calendar_id, ulong max_calendar_id);
int init_calendar(IMCX* imcx, ulong calendar_id, ulong entry_size);
void add_calendar_name(IMCX * imcx, ulong calendar_index, const char *calendar_name);

/**
 * Gets the calendar index by its given name.
 * @param imcx pointer to the extension main struct
 * @param calendar_name pointer to string containing the calendar name
 * @return CALENDAR_INDEX (>0) = SUCCESS, -1 = ERROR
 */
long get_calendar_index_by_name(IMCX * imcx, const char* calendar_name);

/**
 * Calculates and sets the page size for the given calendar pointer
 * @param calendar pointer to calendar
 * @return 0 = SUCCESS, -1 = ERROR
 */
int init_page_size(Calendar * calendar);
void cache_finish(IMCX * imcx);
// --
int add_calendar_days(
        const IMCX * imcx,
        ulong calendar_index,
        int input_date,
        int interval,
        int * first_date_idx,
        int * result_date_idx
);
int cache_invalidate(IMCX * imcx);

#endif //KETTEQ_POSTGRESQL_EXTENSIONS_CACHE_H
