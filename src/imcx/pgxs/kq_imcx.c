/**
 * (C) 2022 ketteQ
 * Giancarlo Chiappe
 */

#include "kq_imcx.h"

// Init of Extension

void _PG_init(void)
{
    elog(INFO, "KetteQ InMem Calendar Extension is now loaded.");
    // Fill Cache if Empty
    if (!cacheFilled) {
        load_all_slices();
    }
}

void _PG_fini(void)
{
    elog(INFO, "Unloaded KetteQ InMem Calendar Extension");
}

void load_all_slices() {
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
//    char *sql_get_min_max = "select min(ce.calendar_id), max(ce.calendar_id) from calendar_entries ce;";
    char *sql_get_min_max = "select min(s.id), max(s.id) from ketteq.slice s"; // 1 - 74079
    // Query #2, Get Calendar's Entries Count and Names
//    char *sql_get_entries_count_per_calendar_id = "select ce.calendar_id, count(*), (select c.\"name\" "
//                                                  "from calendar c where c.id = ce.calendar_id) \"name\" from "
//                                                  "calendar_entries ce group by ce.calendar_id order by ce.calendar_id "
//                                                  "asc ;";
    char *sql_get_entries_count_per_calendar_id = "select s.slice_type_id, count(*), "
                                                  "(select st.\"name\" from ketteq.slice_type st where st.id = s.slice_type_id) \"name\" "
                                                  "from ketteq.slice s "
                                                  "group by s.slice_type_id "
                                                  "order by s.slice_type_id asc;";
    // Query #3, Get Entries of Calendars
//    char *sql_get_entries = "select ce.calendar_id , ce.\"date\" from calendar_entries ce order by ce.calendar_id asc,"
//                            " ce.\"date\" asc ;";
    char *sql_get_entries = "select s.slice_type_id, s.start_on "
                            "from ketteq.slice s "
                            "order by s.slice_type_id asc, "
                            "s.start_on asc;";
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
    // elog(INFO, "Q1: Got %" PRIu64 " calendar entries.", query_exec_rowcount);
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
//    elog(INFO, "Calendar-Id: Min: %" PRId64 ", Max: %" PRId64, min_value, max_value);
    elog(INFO, "Q1: SliceType-Id: Min: %" PRId64 ", Max: %" PRId64, min_value, max_value);
    // Init the Struct's Cache
    prev_ctx = MemoryContextSwitchTo(TopMemoryContext);
    cacheInitCalendars(min_value, (long) max_value);
    MemoryContextSwitchTo(prev_ctx);
    // Init the Structs's date property with the count of the entries
    query_exec_ret = SPI_execute(sql_get_entries_count_per_calendar_id, true, 0);
    query_exec_rowcount = SPI_processed;
    // Check results
    if (query_exec_ret != SPI_OK_SELECT || query_exec_rowcount == 0) {
        SPI_finish();
        elog(ERROR, "Cannot count calendar's entries.");
    }
    // elog(INFO, "Q2: Got %" PRIu64 " calendar_ids with entries count.", query_exec_rowcount);
    elog(INFO, "Q2: Got %" PRIu64 " SliceTypes.", query_exec_rowcount);
    //
    spi_tuple_table = SPI_tuptable;
    spi_tuple_desc = spi_tuple_table->tupdesc;
    // Allocate Calendars and InMemCalendar's Names Map
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
//        elog(INFO, "Got: Name: %s, Calendar-Id: %" PRIu64 ", Entries: %" PRIu64,
        elog(INFO, "Q2 (Cursor): Got: SliceTypeName: %s, SliceType: %" PRIu64 ", Entries: %" PRIu64,
             calendar_name,
             calendar_id, calendar_entry_count);
        // Add to the cache
        cacheCalendars[calendar_id - 1].calendar_id = calendar_id;
        MemoryContext prev_memory_context = MemoryContextSwitchTo(TopMemoryContext);
        cacheInitCalendarEntries(
                &cacheCalendars[calendar_id - 1],
                (long) calendar_entry_count
        );
        MemoryContextSwitchTo(prev_memory_context);
        // Add The Calendar Name
        cacheInitAddCalendarName(cacheCalendars[calendar_id - 1], calendar_name);
    }
    // -> Exec Q3
    query_exec_ret = SPI_execute(sql_get_entries, true, 0);
    query_exec_rowcount = SPI_processed;
    // Check Results
    if (query_exec_ret != SPI_OK_SELECT || query_exec_rowcount == 0) {
        SPI_finish();
        elog(ERROR, "No calendar entries.");
    }
//    elog(INFO, "Q3: Got %" PRIu64 " calendar entries in total.", query_exec_rowcount);
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

//        elog(INFO, "DateADT Tuple: Calendar-Id: %" PRIu64 ", DateADT (Bin): %d",
//             calendar_id, calendar_entry);

        uint64 calendar_index = calendar_id - 1;

        if (prev_calendar_id != calendar_id) {
            prev_calendar_id = calendar_id;
            entry_count = 0;
        }
        cacheCalendars[calendar_index].dates[entry_count++] = calendar_entry;
        if (cacheCalendars[calendar_index].dates_size == entry_count) {
            // entry copy complete, calculate page size
            prev_ctx = MemoryContextSwitchTo(TopMemoryContext);
            cacheInitPageSize(&cacheCalendars[calendar_index]);
            MemoryContextSwitchTo(prev_ctx);
        }
    }
    elog(INFO, "Q3: Cached %" PRIu64 " slices in total.", query_exec_rowcount);
    SPI_finish();
    cacheFilled = true;
}

PG_FUNCTION_INFO_V1(kq_imcx_info);
Datum kq_imcx_info(PG_FUNCTION_ARGS) {
    ReturnSetInfo   *pReturnSetInfo = (ReturnSetInfo *) fcinfo->resultinfo;
    TupleDesc	    tupleDesc;
    Tuplestorestate *pTuplestorestate;
    Datum           values[2];
    bool            nulls[2] = {0};
    //
    if (get_call_result_type(fcinfo, NULL, &tupleDesc) != TYPEFUNC_COMPOSITE)
        elog(ERROR, "Invalid Return Type");
    pTuplestorestate = tuplestore_begin_heap(true, false, work_mem);
    values[0] = CStringGetTextDatum("Version");
    values[1] = CStringGetTextDatum(CMAKE_VERSION);
    pReturnSetInfo->returnMode = SFRM_Materialize;
    pReturnSetInfo->setResult = pTuplestorestate;
    pReturnSetInfo->setDesc = tupleDesc;
    tuplestore_putvalues(pTuplestorestate, tupleDesc, values, nulls);
    tuplestore_donestoring(pTuplestorestate);
    /* ... C code here ... */
    tuplestore_donestoring(pTuplestorestate);
    return (Datum) 0;
}

PG_FUNCTION_INFO_V1(kq_invalidate_cache);
Datum kq_invalidate_cache(PG_FUNCTION_ARGS) {
    MemoryContext old_context = MemoryContextSwitchTo(TopMemoryContext);
    int ret = cacheInvalidate();
    MemoryContextSwitchTo(old_context);
    if (ret == 0) {
        PG_RETURN_TEXT_P(cstring_to_text("Cache Cleared."));
    } else {
        PG_RETURN_TEXT_P(cstring_to_text("Cache Not Exists."));
    }
}

void report_names(gpointer _key, gpointer _value, gpointer _user_data) {
    char * key = _key;
    char * value = _value;
    elog(INFO, "SliceType-Name: '%s' -> SliceTypeId: '%s'", key, value);
}

int pg_calcache_report(int showEntries, int showPageMapEntries, int showNames) {
    if (!cacheFilled) {
        return -1;
    }
    elog(INFO, "Slices-Id Min-Max %" PRIu64, cacheCalendarCount);
    for (uint64 i = 0; i < cacheCalendarCount; i++) {
        InMemCalendar curr_calendar = cacheCalendars[i];
        if (curr_calendar.calendar_id != 0) {
            // char * calendar_name = cacheGetCalendarName(curr_calendar); // Requires Fixing
            elog(INFO, "SliceType-Id: %d, "
                       "Entries: %d, "
                       "Page Map Size: %d, "
                       "Page Size: %d",
                 curr_calendar.calendar_id,
                 curr_calendar.dates_size,
                 curr_calendar.page_map_size,
                 curr_calendar.page_size
                 );
            if (showEntries)
                for (uint32 j = 0; j < curr_calendar.dates_size; j++) {
                    elog(INFO, "Entry[%d]: %d", j, curr_calendar.dates[j]);
                }
            if (showPageMapEntries)
                for (uint32 j = 0; j < curr_calendar.page_map_size; j++) {
                    elog(INFO, "PageMap Entry[%d]: %d", j, curr_calendar.page_map[j]);
                }
        }
    }
    if (showNames)
        g_hash_table_foreach(cacheCalendarNameHashTable, report_names, NULL);
    return 0;
}

PG_FUNCTION_INFO_V1(kq_report_cache);
Datum kq_report_cache(PG_FUNCTION_ARGS) {
    int32 showEntries = PG_GETARG_INT32(0);
    int32 showPageMapEntries = PG_GETARG_INT32(1);
    int32 showNames = PG_GETARG_INT32(2);
    int ret = pg_calcache_report(showEntries, showPageMapEntries, showNames);
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
    DateADT new_date = cacheAddCalendarDays(date,
                                            calendar_interval,
                                            cacheCalendars[calendar_id - 1],
                                            NULL,
                                            NULL);
    //
    PG_RETURN_DATEADT(new_date);
}

PG_FUNCTION_INFO_V1(kq_add_calendar_days_by_calendar_name);
Datum
kq_add_calendar_days_by_calendar_name(PG_FUNCTION_ARGS) {
    //
    int32 date = PG_GETARG_INT32(0);
    int32 calendar_interval = PG_GETARG_INT32(1);
    text * calendar_name = PG_GETARG_TEXT_P(2);
    // Convert to CSTRING
    char * calendar_name_str = text_to_cstring(calendar_name);
    // Lookup for the InMemCalendar
    InMemCalendar cal;
    int ret = cacheGetCalendarByName(calendar_name_str, &cal);
    if (ret < 0) {
        elog(ERROR, "Calendar does not exists.");
    }
    int fd_idx, rs_idx;
    // Do the Math.
    DateADT new_date = cacheAddCalendarDays(
            date,
            calendar_interval,
            cal,
            &fd_idx,
            &rs_idx
    );
    //
    elog(INFO, "FD-Idx: %d = %d, RS-Idx: %d = %d", fd_idx, cal.dates[fd_idx],
         rs_idx, cal.dates[rs_idx]);
    //
    PG_RETURN_DATEADT(new_date);
}

PG_FUNCTION_INFO_V1(kq_load_all_calendars);
Datum
kq_load_all_calendars(PG_FUNCTION_ARGS) {
    if (cacheFilled) {
        PG_RETURN_DATUM(kq_report_cache(NULL));
    }
    load_all_slices();
    PG_RETURN_TEXT_P(cstring_to_text("Done."));
}