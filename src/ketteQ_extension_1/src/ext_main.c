#include <postgres.h>
#include "utils/date.h"
#include "utils/builtins.h"
#include "utils/memutils.h"
#include "executor/spi.h"

#include <fmgr.h>
#include <inttypes.h>
#include <stdio.h>

#include "ketteQ_extension_1/src/calendar/cache.h"

PG_MODULE_MAGIC;

int pg_calcache_report() {
    if (!calcache_filled) {
        // elog(ERROR, "Cannot Report Cache Because is Empty.");
        return -1;
    }
    elog(INFO, "Calendar Count: %" PRIu64, calcache_calendar_count);
    for (uint64 i = 0; i < calcache_calendar_count; i++) {
        Calendar curr_calendar = calcache_calendars[i];
        elog(INFO, "Calendar-Id: %d, "
                   "Entries: %d, "
                   "Page Map Size: %d, "
                   "Page Size: %d",
             curr_calendar.calendar_id,
             curr_calendar.dates_size,
             curr_calendar.page_map_size,
             curr_calendar.page_size);
        for (uint32 j = 0; j < curr_calendar.dates_size; j++) {
            elog(INFO, "Entry[%d]: %d", j, curr_calendar.dates[j]);
        }
        for (uint32 j = 0; j < curr_calendar.page_map_size; j++) {
            elog(INFO, "PageMap Entry[%d]: %d", j, curr_calendar.page_map[j]);
        }
    }
    return 0;
}

PG_FUNCTION_INFO_V1(kq_invalidate_cache);
Datum kq_invalidate_cache(PG_FUNCTION_ARGS) {
    MemoryContext old_context = MemoryContextSwitchTo(TopMemoryContext);
    int ret = calcache_invalidate();
    MemoryContextSwitchTo(old_context);
    if (ret == 0) {
        PG_RETURN_TEXT_P(cstring_to_text("Cache Cleared."));
    } else {
        PG_RETURN_TEXT_P(cstring_to_text("Cache Not Exists."));
    }
}

PG_FUNCTION_INFO_V1(kq_report_cache);
Datum kq_report_cache(PG_FUNCTION_ARGS) {
    int ret = pg_calcache_report();
    if (ret == 0) {
        PG_RETURN_TEXT_P(cstring_to_text("OK."));
    } else {
        PG_RETURN_TEXT_P(cstring_to_text("Cache Not Exists or Empty."));
    }
}


PG_FUNCTION_INFO_V1(kq_add_calendar_days);
Datum
kq_add_calendar_days(PG_FUNCTION_ARGS) {
    int32 date = PG_GETARG_INT32(0);
    int32 calendar_interval = PG_GETARG_INT32(1);
    int32 calendar_id = PG_GETARG_INT32(2);
    //
    DateADT new_date = calcache_add_calendar_days(date,
                                                  calendar_interval,
                                                  calcache_calendars[calendar_id - 1]);
    PG_RETURN_DATEADT(new_date);
}

PG_FUNCTION_INFO_V1(kq_add_calendar_days_by_calendar_name);
Datum
kq_add_calendar_days_by_calendar_name(PG_FUNCTION_ARGS) {
    int32 date = PG_GETARG_INT32(0);
    int32 calendar_interval = PG_GETARG_INT32(1);
    text * calendar_name = PG_GETARG_TEXT_P(2);
    // Convert to CSTRING
    char * calendar_name_str = text_to_cstring(calendar_name);
    // Lookup for the Calendar
    Calendar cal;
    int ret = calcache_get_calendar_by_name(calendar_name_str, &cal);
    if (ret < 0) {
        elog(ERROR, "Calendar does not exists.");
    }
    // Do the Math.
    DateADT new_date = calcache_add_calendar_days(
            date,
            calendar_interval,
            cal
            );
    //
    PG_RETURN_DATEADT(new_date);
}

PG_FUNCTION_INFO_V1(kq_load_all_calendars);
Datum
kq_load_all_calendars(PG_FUNCTION_ARGS) {
    if (calcache_filled) {
        PG_RETURN_DATUM(kq_report_cache(NULL));
    }
    // Vars
    int query_exec_ret;
    MemoryContext prev_ctx;
    // Row Control
    uint64 curr_row_call;
    uint64 proc;
    uint64 query_exec_rowcount;
    int columns;
    bool isNull;
    // Extra Counters
    int i;
    // Query #1, Get MIN_ID and MAX_ID of Calendars
    char *sql_get_min_max = "select min(ce.calendar_id), max(ce.calendar_id) from calendar_entries ce;";
    // Query #2, Get Calendar's Entries Count and Names
//    char *sql_get_entries_count_per_calendar_id = "select ce.calendar_id, count(*) from calendar_entries ce group by "
//                                                  "ce.calendar_id order by ce.calendar_id asc ;";
    char *sql_get_entries_count_per_calendar_id = "select ce.calendar_id, count(*), (select c.\"name\" "
                                                  "from calendar c where c.id = ce.calendar_id) \"name\" from "
                                                  "calendar_entries ce group by ce.calendar_id order by ce.calendar_id "
                                                  "asc ;";
    // Query #3, Get calendar names --> Can be done with the Q2. TODO: Optimize.
    // char* sql_get_calendar_names = "select c.id, c.\"name\" from calendar c order by c.id asc;";
    // Query #4, Get Entries of Calendars
    char *sql_get_entries = "select ce.calendar_id , ce.\"date\" from calendar_entries ce order by ce.calendar_id asc,"
                            " ce.\"date\" asc ;";
    // SPI
    SPITupleTable *spi_tuple_table;
    TupleDesc spi_tuple_desc;
    // Connect to SPI
    if ((query_exec_ret = SPI_connect()) < 0) {
        elog(ERROR, "SPI_connect returned %d", query_exec_ret);
    }
    // Execute Q1, get the min and max calendar_id's
    query_exec_ret = SPI_execute(sql_get_min_max, true, 0);
    query_exec_rowcount = SPI_processed;
    // Check results
    if (query_exec_ret != SPI_OK_SELECT || query_exec_rowcount == 0) {
        SPI_finish();
        elog(ERROR, "No calendars.");
    }
    //
    elog(INFO, "Q1: Got %" PRIu64 " query_exec_rowcount.", query_exec_rowcount);
    // Get the Data and Descriptor
    spi_tuple_table = SPI_tuptable;
    spi_tuple_desc = spi_tuple_table->tupdesc;
    HeapTuple min_max_tuple = spi_tuple_table->vals[0];
    //
    uint64 min_value = DatumGetUInt64(
            SPI_getbinval(min_max_tuple,
                          spi_tuple_desc,
                          1,
                          &isNull));
    uint64 max_value = DatumGetUInt64(
            SPI_getbinval(min_max_tuple,
                          spi_tuple_desc,
                          2,
                          &isNull));
    //
    elog(INFO, "Calendar-Id: Min: %" PRId64 ", Max: %" PRId64, min_value, max_value);
    // Init the Struct's Cache
    prev_ctx = MemoryContextSwitchTo(TopMemoryContext);
    calcache_init_calendars(min_value, max_value);
    MemoryContextSwitchTo(prev_ctx);
    // Init the Structs's date property with the count of the entries
    query_exec_ret = SPI_execute(sql_get_entries_count_per_calendar_id, true, 0);
    query_exec_rowcount = SPI_processed;
    // Check results
    if (query_exec_ret != SPI_OK_SELECT || query_exec_rowcount == 0) {
        SPI_finish();
        elog(ERROR, "Cannot count calendar's entries.");
    }
    elog(INFO, "Q2: Got %" PRIu64 " calendar_ids with entries count.", query_exec_rowcount);
    //
    spi_tuple_table = SPI_tuptable;
    spi_tuple_desc = spi_tuple_table->tupdesc;
    // Allocate Calendars and Calendar's Names Map
    for (uint64 row_counter = 0; row_counter < query_exec_rowcount; row_counter++) {
        HeapTuple cal_entries_count_tuple = spi_tuple_table->vals[row_counter];
        uint64 calendar_id = DatumGetUInt64(
                SPI_getbinval(cal_entries_count_tuple,
                              spi_tuple_desc,
                              1,
                              &isNull));
        uint64 calendar_entry_count = DatumGetUInt64(
                SPI_getbinval(cal_entries_count_tuple,
                              spi_tuple_desc,
                              2,
                              &isNull));
        char * calendar_name = DatumGetCString(
                SPI_getvalue(
                        cal_entries_count_tuple,
                        spi_tuple_desc,
                        3
                        )
                );
        elog(INFO, "Got: Name: %s, Calendar-Id: %" PRIu64 ", Entries: %" PRIu64,
             calendar_name,
             calendar_id, calendar_entry_count);

        calcache_calendars[calendar_id-1].calendar_id = calendar_id;

        MemoryContext prev_memory_context = MemoryContextSwitchTo(TopMemoryContext);
        calcache_init_calendar_entries(
                &calcache_calendars[calendar_id-1],
                calendar_entry_count
                );
        MemoryContextSwitchTo(prev_memory_context);
        // Add The Calendar Name
        calcache_init_add_calendar_name(calcache_calendars[calendar_id-1], calendar_name);
    }
    // -> Exec Q3
    query_exec_ret = SPI_execute(sql_get_entries, true, 0);
    query_exec_rowcount = SPI_processed;
    // Check Results
    if (query_exec_ret != SPI_OK_SELECT || query_exec_rowcount == 0) {
        SPI_finish();
        elog(ERROR, "No calendar entries.");
    }
    elog(INFO, "Q3: Got %" PRIu64 " calendar entries in total.", query_exec_rowcount);
    //
    spi_tuple_table = SPI_tuptable;
    spi_tuple_desc = spi_tuple_table->tupdesc;
    // Copy data
    uint64 prev_calendar_id = 0;
    uint64 entry_count = 0;
    for (uint64 row_counter = 0; row_counter < query_exec_rowcount; row_counter++) {
        HeapTuple cal_entries_tuple = spi_tuple_table->vals[row_counter];
        uint64 calendar_id = DatumGetUInt64(
                SPI_getbinval(cal_entries_tuple,
                              spi_tuple_desc,
                              1,
                              &isNull));
        int32 calendar_entry = DatumGetDateADT(
                SPI_getbinval(cal_entries_tuple,
                              spi_tuple_desc,
                              2,
                              &isNull));

        elog(INFO, "DateADT Tuple: Calendar-Id: %" PRIu64 ", DateADT (Bin): %d",
             calendar_id, calendar_entry);

        uint64 calendar_index = calendar_id - 1;

        if (prev_calendar_id != calendar_id) {
            prev_calendar_id = calendar_id;
            entry_count = 0;
        }
        calcache_calendars[calendar_index].dates[entry_count++] = calendar_entry;
        if (calcache_calendars[calendar_index].dates_size == entry_count) {
            // entry copy complete, calculate page size
            prev_ctx = MemoryContextSwitchTo(TopMemoryContext);
            calcache_calculate_page_size(&calcache_calendars[calendar_index]);
            MemoryContextSwitchTo(prev_ctx);
        }
    }
    SPI_finish();
    calcache_filled = true;
    PG_RETURN_TEXT_P("Executed OK.");
}