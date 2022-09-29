/**
 * (C) 2022 ketteQ
 * Giancarlo Chiappe
 */

#include "kq_imcx.h"
#include "src/common/util.h"


bool loadingCache = false;
uint64 calendars_entry_count = 0;

// Init of Extension

void _PG_init(void)
{
    elog(INFO, "KetteQ In-Memory Calendar Extension Loaded.");
    // Fill Cache if Empty
    if (!cacheFilled) {
        load_all_slices();
    }
}

void _PG_fini(void)
{
    elog(INFO, "Unloaded KetteQ In-Memory Calendar Extension");
}

void load_all_slices() {
    if (loadingCache) { return; }
    loadingCache = true;
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
    char *sql_get_min_max = "select min(s.id), max(s.id) from ketteq.slice_type s"; // 1 - 74079
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
    elog(DEBUG1, "Q1: Min: %" PRId64 ", Max: %" PRId64, min_value, max_value);
    // Init the Struct's Cache
    prev_ctx = MemoryContextSwitchTo(TopMemoryContext);
    cacheInitCalendars(min_value, (long) max_value);
    MemoryContextSwitchTo(prev_ctx);
    // Init the Structs date property with the count of the entries
    query_exec_ret = SPI_execute(sql_get_entries_count_per_calendar_id, true, 0);
    query_exec_rowcount = SPI_processed;
    // Check results
    if (query_exec_ret != SPI_OK_SELECT || query_exec_rowcount == 0) {
        SPI_finish();
        elog(ERROR, "Cannot count calendar's entries.");
    }
    elog(DEBUG1, "Q2: Got %" PRIu64 " SliceTypes.", query_exec_rowcount);
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
        calendars_entry_count += calendar_entry_count;
        elog(DEBUG1, "Q2 (Cursor): Got: SliceTypeName: %s, SliceType: %" PRIu64 ", Entries: %" PRIu64,
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
    elog(DEBUG1, "Q3: Cached %" PRIu64 " slices in total.", query_exec_rowcount);
    SPI_finish();
    cacheFilled = true;
    loadingCache = false;
}

void add_row_to_tuple(
        AttInMetadata *attinmeta,
        Tuplestorestate  *tuplestore,
        char * prop, char * value) {
    HeapTuple tuple;
    char * values[2];
    values[0] = prop; values[1] = value;
    tuple = BuildTupleFromCStrings(attinmeta, values);
    tuplestore_puttuple(tuplestore, tuple);
}

// TODO
PG_FUNCTION_INFO_V1(imcx_info);
Datum imcx_info(PG_FUNCTION_ARGS) {
    ReturnSetInfo *rsinfo = (ReturnSetInfo *) fcinfo->resultinfo;
    Tuplestorestate  *tuplestore;
    AttInMetadata *attinmeta;
    MemoryContext queryContext;
    MemoryContext oldContext;
    TupleDesc	tupleDesc;

    char*           values[2];
    bool            nulls[2];

    /* check to see if caller supports us returning a tuplestore */
    if (rsinfo == NULL || !IsA(rsinfo, ReturnSetInfo))
        ereport(ERROR,
                (errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
                        errmsg("set-valued function called in context that cannot accept a set")));
    if (!(rsinfo->allowedModes & SFRM_Materialize))
        ereport(ERROR,
                (errcode(ERRCODE_SYNTAX_ERROR),
                        errmsg("materialize mode required, but it is not allowed in this context")));
    /* Build tuplestore to hold the result rows */
    oldContext = MemoryContextSwitchTo(rsinfo->econtext->ecxt_per_query_memory); // Switch To Per Query Context

    tupleDesc = CreateTemplateTupleDesc(2);
    TupleDescInitEntry(tupleDesc,
                       (AttrNumber) 1, "property", TEXTOID, -1, 0);
    TupleDescInitEntry(tupleDesc,
                       (AttrNumber) 2, "value", TEXTOID, -1, 0);

    tuplestore = tuplestore_begin_heap(true, false, work_mem); // Create TupleStore
    rsinfo->returnMode = SFRM_Materialize;
    rsinfo->setResult = tuplestore;
    rsinfo->setDesc = tupleDesc;
    MemoryContextSwitchTo(oldContext); // Switch back to previous context
    attinmeta = TupleDescGetAttInMetadata(tupleDesc);

    add_row_to_tuple(attinmeta, tuplestore,
                     "Version", CMAKE_VERSION);
    add_row_to_tuple(attinmeta, tuplestore,
                     "Cache Available", cacheFilled ? "Yes" : "No");
    add_row_to_tuple(attinmeta, tuplestore,
                     "Slice Cache Size (SliceType Count)", convertUIntToStr(cacheCalendarCount));
    add_row_to_tuple(attinmeta, tuplestore,
                     "Entry Cache Size (Slices)", convertUIntToStr(calendars_entry_count));

    return (Datum) 0;
}

PG_FUNCTION_INFO_V1(imcx_invalidate);
Datum imcx_invalidate(PG_FUNCTION_ARGS) {
    MemoryContext old_context = MemoryContextSwitchTo(TopMemoryContext);
    int ret = cacheInvalidate();
    MemoryContextSwitchTo(old_context);
    if (ret == 0) {
        calendars_entry_count = 0;
        load_all_slices();
        PG_RETURN_CSTRING("Cache Invalidated.");
        //return CStringGetDatum("Cache Cleared.");
    }
    PG_RETURN_CSTRING("Error Invalidating the Cache.");
    // return CStringGetDatum("Cache Not Exists.");
}

typedef struct TupleData {
    AttInMetadata *attinmeta;
    Tuplestorestate  *tuplestore;
} TupleData;



void report_names(gpointer _key, gpointer _value, gpointer _user_data) {
    char * key = _key;
    char * value = _value;
    TupleData * tupleData =  (TupleData *) _user_data;
    add_row_to_tuple(tupleData->attinmeta, tupleData->tuplestore,
                     key,
                     value);
}

typedef struct GetNameData {
    char * searchId;
    char * name;
} GetNameData;

void get_name(gpointer _key, gpointer _value, gpointer _user_data) {
    char * key = _key;
    char * value = _value;
    GetNameData * user = _user_data;
    if (strcmp(user->searchId, value) == 0) {
        user->name = key;
    }
}

int pg_calcache_report(int showEntries, int showPageMapEntries, FunctionCallInfo fcinfo) {
    if (!cacheFilled) {
        return -1;
    }
    ReturnSetInfo *rsinfo = (ReturnSetInfo *) fcinfo->resultinfo;
    Tuplestorestate  *tuplestore;
    AttInMetadata *attinmeta;
    MemoryContext queryContext;
    MemoryContext oldContext;
    TupleDesc	tupleDesc;
    char*           values[2];
    bool            nulls[2];
    // --
    /* check to see if caller supports us returning a tuplestore */
    if (rsinfo == NULL || !IsA(rsinfo, ReturnSetInfo))
        ereport(ERROR,
                (errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
                        errmsg("set-valued function called in context that cannot accept a set")));
    if (!(rsinfo->allowedModes & SFRM_Materialize))
        ereport(ERROR,
                (errcode(ERRCODE_SYNTAX_ERROR),
                        errmsg("materialize mode required, but it is not allowed in this context")));
    /* Build tuplestore to hold the result rows */
    oldContext = MemoryContextSwitchTo(rsinfo->econtext->ecxt_per_query_memory); // Switch To Per Query Context

    tupleDesc = CreateTemplateTupleDesc(2);
    TupleDescInitEntry(tupleDesc,
                       (AttrNumber) 1, "property", TEXTOID, -1, 0);
    TupleDescInitEntry(tupleDesc,
                       (AttrNumber) 2, "value", TEXTOID, -1, 0);

    tuplestore = tuplestore_begin_heap(true, false, work_mem); // Create TupleStore
    rsinfo->returnMode = SFRM_Materialize;
    rsinfo->setResult = tuplestore;
    rsinfo->setDesc = tupleDesc;
    MemoryContextSwitchTo(oldContext); // Switch back to previous context
    attinmeta = TupleDescGetAttInMetadata(tupleDesc);
    // --
    add_row_to_tuple(attinmeta, tuplestore,
                     "Slices-Id Max", convertUIntToStr(cacheCalendarCount));
    add_row_to_tuple(attinmeta, tuplestore,
                     "Cache-Calendars Size", convertUIntToStr(sizeof(&cacheCalendars)));
    // --
    // elog(INFO, "Slices-Id Max %" PRIu64, cacheCalendarCount);
    int noCalendarIdCounter = 0;
    for (uint64 i = 0; i < cacheCalendarCount; i++) {
        InMemCalendar curr_calendar = cacheCalendars[i];
        if (curr_calendar.calendar_id != 0) {
            add_row_to_tuple(attinmeta, tuplestore,
                             "SliceType-Id", convertUIntToStr(curr_calendar.calendar_id));
            // -- Get The Name
            GetNameData getNameData; getNameData.searchId = convertUIntToStr(curr_calendar.calendar_id);
            g_hash_table_foreach(cacheCalendarNameHashTable, get_name, &getNameData);
            // --
            add_row_to_tuple(attinmeta, tuplestore,
                             "   Name", getNameData.name);
            add_row_to_tuple(attinmeta, tuplestore,
                             "   Entries", convertUIntToStr(curr_calendar.dates_size));
            add_row_to_tuple(attinmeta, tuplestore,
                             "   Page Map Size", convertUIntToStr(curr_calendar.page_map_size));
            add_row_to_tuple(attinmeta, tuplestore,
                             "   Page Size", convertUIntToStr(curr_calendar.page_size));
            if (showEntries)
                for (uint32 j = 0; j < curr_calendar.dates_size; j++) {
                    elog(INFO, "Entry[%d]: %d", j, curr_calendar.dates[j]);
                    add_row_to_tuple(attinmeta, tuplestore,
                                     "   Entry",
                                     convertUIntToStr(curr_calendar.dates[j]));
                }
            if (showPageMapEntries)
                for (uint32 j = 0; j < curr_calendar.page_map_size; j++) {
                    elog(INFO, "PageMap Entry[%d]: %d", j, curr_calendar.page_map[j]);
                    add_row_to_tuple(attinmeta, tuplestore,
                                     "   PageMap Entry",
                                     convertUIntToStr(curr_calendar.page_map[j]));
                }
        } else {
            noCalendarIdCounter++;
        }
    }
    add_row_to_tuple(attinmeta, tuplestore,
                     "Missing Slices (id==0)",
                     convertIntToStr(noCalendarIdCounter));
    return 0;
}

PG_FUNCTION_INFO_V1(imcx_report);
Datum imcx_report(PG_FUNCTION_ARGS) {
    int32 showEntries = PG_GETARG_INT32(0);
    int32 showPageMapEntries = PG_GETARG_INT32(1);
    pg_calcache_report(showEntries, showPageMapEntries, fcinfo);
    return (Datum) 0;
}


PG_FUNCTION_INFO_V1(imcx_add_calendar_days_by_id);
Datum
imcx_add_calendar_days_by_id(PG_FUNCTION_ARGS) {
    int32 date = PG_GETARG_INT32(0);
    int32 calendar_interval = PG_GETARG_INT32(1);
    int32 calendar_id = PG_GETARG_INT32(2);
    int32 calendar_index = calendar_id - 1;
    // TODO: Validate Calendar ID
    InMemCalendar cal = cacheCalendars[calendar_index];
    //
    int fd_idx, rs_idx;
    //
    DateADT new_date = cacheAddCalendarDays(date,
                                            calendar_interval,
                                            cal,
                                            &fd_idx,
                                            &rs_idx);
    //
    elog(DEBUG1, "FirstDate-Idx: %d = %d, ResultDate-Idx: %d = %d",
         fd_idx, cal.dates[fd_idx],
         rs_idx, cal.dates[rs_idx]
    );
    //
    PG_RETURN_DATEADT(new_date);
}

PG_FUNCTION_INFO_V1(imcx_add_calendar_days_by_calendar_name);
Datum
imcx_add_calendar_days_by_calendar_name(PG_FUNCTION_ARGS) {
    int32 date = PG_GETARG_INT32(0);
    int32 calendar_interval = PG_GETARG_INT32(1);
    text * calendar_name = PG_GETARG_TEXT_P(2);
    // Convert Name to CString
    char * calendar_name_str = text_to_cstring(calendar_name);
    // Lookup for the Calendar
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
    elog(DEBUG1, "FirstDate-Idx: %d = %d, ResultDate-Idx: %d = %d",
         fd_idx, cal.dates[fd_idx],
         rs_idx, cal.dates[rs_idx]
    );
    //
    PG_RETURN_DATEADT(new_date);
}