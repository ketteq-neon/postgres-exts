/**
 * (C) KetteQ, Inc.
 */

#include "kq_imcx.h"
#include "config.h"

static shmem_startup_hook_type prev_shmem_startup_hook = NULL;

// Init of Extension

void _PG_init(void) {
  if (!process_shared_preload_libraries_in_progress) {
    ereport(ERROR, (errmsg("kq_imcx can only be loaded via shared_preload_libraries"),
        errhint("Add kq_imcx to shared_preload_libraries configuration "
                "variable in postgresql.conf in master and workers. Note "
                "that kq_imcx should be at the beginning of "
                "shared_preload_libraries.")));
  }
  init_gucs();
  RequestAddinShmemSpace((size_t)SHMEM_REQUESTED_MEMORY);
  RequestNamedLWLockTranche(TRANCHE_NAME, 1);
  prev_shmem_startup_hook = shmem_startup_hook;
  shmem_startup_hook = &init_shared_memory;
  ereport (INFO, errmsg("KetteQ In-Memory Calendar Extension Loaded."));
}

void _PG_fini(void) {
  ereport (INFO, errmsg("Unloaded KetteQ In-Memory Calendar Extension."));
}

typedef struct {
  LWLockId lock;
} IMCXSharedMemory;

IMCXSharedMemory *shared_memory_ptr;
IMCX *imcx_ptr;

char *q1_get_cal_min_max_id = DEF_Q1_GET_CALENDAR_IDS;
char *q2_get_cal_entry_count = DEF_Q2_GET_CAL_ENTRY_COUNT;
char *q3_get_cal_entries = DEF_Q3_GET_ENTRIES;

void init_gucs() {
  DefineCustomStringVariable("kq.calendar.q1_get_calendar_min_max_id",
                             "Query to select the MIN and MAX slices types IDs.",
                             NULL,
                             &q1_get_cal_min_max_id,
                             DEF_Q1_GET_CALENDAR_IDS,
                             PGC_USERSET,
                             0,
                             NULL,
                             NULL,
                             NULL);

  DefineCustomStringVariable("kq.calendar.q2_get_calendars_entry_count",
                             "Query to select the entry count for each slice types.",
                             NULL,
                             &q2_get_cal_entry_count,
                             DEF_Q2_GET_CAL_ENTRY_COUNT,
                             PGC_USERSET,
                             0,
                             NULL,
                             NULL,
                             NULL);

  DefineCustomStringVariable("kq.calendar.q3_get_calendar_entries",
                             "Query to select all calendar entries. This will be copied to the cache.",
                             NULL,
                             &q3_get_cal_entries,
                             DEF_Q3_GET_ENTRIES,
                             PGC_USERSET,
                             0,
                             NULL,
                             NULL,
                             NULL);
}

static void init_shared_memory() {
#ifndef NDEBUG
  ereport(DEF_DEBUG_LOG_LEVEL, errmsg("init_shared_memory()"));
#endif
  if (prev_shmem_startup_hook) {
    prev_shmem_startup_hook();
  }
  imcx_ptr = NULL;
  shared_memory_ptr = NULL;
  LWLockAcquire(AddinShmemInitLock, LW_EXCLUSIVE);
  bool shared_memory_found;
  bool imcx_found;
  shared_memory_ptr = ShmemInitStruct("IMCXSharedMemory", sizeof(IMCXSharedMemory), &shared_memory_found);
  imcx_ptr = ShmemInitStruct("IMCX", sizeof(IMCX), &imcx_found);
  if (!shared_memory_found || !imcx_found) {
    memset(shared_memory_ptr, 0, sizeof(IMCXSharedMemory));
    memset(imcx_ptr, 0, sizeof(IMCX));
    shared_memory_ptr->lock = (LWLockId)GetNamedLWLockTranche(TRANCHE_NAME);
#ifndef NDEBUG
    ereport(DEF_DEBUG_LOG_LEVEL, errmsg("Allocated Shared Memory."));
#endif
  }
  int32 init_hashtable_result = pg_init_hashtable(imcx_ptr);
  if (init_hashtable_result != RET_SUCCESS) {
    ereport(ERROR, errmsg("Cannot Attach Names HashTable, Error Code: %d", init_hashtable_result));
  } else {
#ifndef NDEBUG
    ereport(DEF_DEBUG_LOG_LEVEL, errmsg("Memory Attached"));
#endif
  }
  LWLockRelease(AddinShmemInitLock);

  ereport(INFO, errmsg("Initialized Shared Memory."));
}

void ensure_cache_populated() {
  if (imcx_ptr->cache_filled) {
#ifndef NDEBUG
    ereport(DEF_DEBUG_LOG_LEVEL, errmsg("Cache already filled. Skipping slice loading."));
#endif
    return; // Do nothing if the cache is already filled
  }
  LWLockAcquire(shared_memory_ptr->lock, LW_EXCLUSIVE);
  if (!LWLockHeldByMe(shared_memory_ptr->lock)) {
    ereport(ERROR, errmsg("Cannot acquire AddinShmemInitLock"));
  }
#ifndef NDEBUG
  ereport(DEF_DEBUG_LOG_LEVEL, errmsg("Exclusive Write Lock Acquired."));
#endif
  if (imcx_ptr->cache_filled) {
    LWLockRelease(shared_memory_ptr->lock);
#ifndef NDEBUG
    ereport(DEF_DEBUG_LOG_LEVEL, errmsg("Cache already filled. Skipping slice loading. Lock Released."));
#endif
    return; // Do nothing if the cache is already filled
  }
  // Vars
  int32 prev_calendar_id = 0; // Previous Calendar ID
  int32 entry_count = 0; // Entry Counter
  // Row Control
  bool entry_is_null; // Pointer to boolean, TRUE if the last entry was NULL
  // Connect to SPI
  int32 spi_connect_result = SPI_connect();
  if (spi_connect_result < 0) {
    ereport(ERROR, errmsg("SPI_connect returned %d", spi_connect_result));
  }
  // Execute Q1, get the min and max id's
  // Query #1, Get MIN_ID and MAX_ID of Calendars
  if (SPI_execute(q1_get_cal_min_max_id, true, 0) != SPI_OK_SELECT || SPI_processed == 0) {
    SPI_finish();
    ereport(ERROR, errmsg("No calendars."));
  }

  // Get the Data and Descriptor
  HeapTuple min_max_tuple = SPI_tuptable->vals[0];
  //
  int32 min_value = DatumGetInt32(
      SPI_getbinval(min_max_tuple,
                    SPI_tuptable->tupdesc,
                    1,
                    &entry_is_null));
  int32 max_value = DatumGetInt32(
      SPI_getbinval(min_max_tuple,
                    SPI_tuptable->tupdesc,
                    2,
                    &entry_is_null));
  // Init the Struct Cache
  if (pg_cache_init(imcx_ptr, min_value, max_value) < 0) {
    ereport(ERROR, errmsg("Shared Memory Cannot Be Allocated (cache_init, %d, %d)", min_value, max_value));
  }
#ifndef NDEBUG
  ereport(DEF_DEBUG_LOG_LEVEL, errmsg(
      "Memory Allocated (Q1), Min-Value: '%d' Max-Value: '%d",
      min_value,
      max_value
  ));
#endif
  // Query #2, Get Calendar's Entries Count and Names
  // Init the Structs date property with the count of the entries
  if (SPI_execute(q2_get_cal_entry_count, true, 0) != SPI_OK_SELECT || SPI_processed == 0) {
    SPI_finish();
    ereport(ERROR, errmsg("Cannot count calendar's entries."));
  }
#ifndef NDEBUG
  ereport(DEF_DEBUG_LOG_LEVEL, errmsg("Q2: Got %" PRIu64 " SliceTypes.", SPI_processed));
#endif
  for (uint64 row_counter = 0; row_counter < SPI_processed; row_counter++) {
    HeapTuple cal_entries_count_tuple = SPI_tuptable->vals[row_counter];
    int32 calendar_id = DatumGetInt32(
        SPI_getbinval(cal_entries_count_tuple,
                      SPI_tuptable->tupdesc,
                      1,
                      &entry_is_null));
    if (entry_is_null) {
      continue;
    }
    int32 calendar_entry_count = DatumGetInt32(
        SPI_getbinval(cal_entries_count_tuple,
                      SPI_tuptable->tupdesc,
                      2,
                      &entry_is_null));
    char *calendar_name = DatumGetCString(
        SPI_getvalue(
            cal_entries_count_tuple,
            SPI_tuptable->tupdesc,
            3
        )
    );
#ifndef NDEBUG
    ereport(DEF_DEBUG_LOG_LEVEL,
            errmsg(
                "Q2 (Cursor): Got: SliceTypeName: %s, SliceType: %d, Entries: %d",
                calendar_name,
                calendar_id,
                calendar_entry_count
            ));
#endif
    // Add to the cache
    Calendar *calendar = pg_get_calendar(imcx_ptr, calendar_id); // Pointer to current calendar
    int32 init_result = pg_calendar_init(
        calendar,
        calendar_id,
        calendar_entry_count,
        &imcx_ptr->entry_count
    );
    if (init_result != RET_SUCCESS) {
      ereport(ERROR, errmsg("Cannot initialize calendar, ERROR CODE: %d", init_result));
    }
#ifndef NDEBUG
    ereport(DEF_DEBUG_LOG_LEVEL, errmsg(
        "Calendar Initialized, Index: '%d', ID: '%d', Entry count: '%d'",
        calendar_id - 1,
        calendar_id,
        calendar_entry_count
    ));
#endif
    // Add The Calendar Name
    int32 set_name_result = pg_set_calendar_name(imcx_ptr,
                                                 calendar,
                                                 calendar_name);
    if (set_name_result != RET_SUCCESS) {
      ereport(ERROR, errmsg("Cannot set '%s' as name for calendar id '%d'", calendar_name, calendar_id));
    }
#ifndef NDEBUG
    ereport(DEF_DEBUG_LOG_LEVEL, errmsg("Calendar Name '%s' Set", calendar_name));
#endif
  }
#ifndef NDEBUG
  ereport(DEF_DEBUG_LOG_LEVEL, errmsg("Executing Q3"));
#endif
  // -> Exec Q3
  // Query #3, Get Entries of Calendars
  // Check Results
  if (SPI_execute(q3_get_cal_entries, true, 0) != SPI_OK_SELECT || SPI_processed == 0) {
    SPI_finish();
    ereport(ERROR, errmsg("No calendar entries."));
  }
#ifndef NDEBUG
  ereport(DEF_DEBUG_LOG_LEVEL, errmsg("Q3: RowCount: '%" PRIu64 "'", SPI_processed));
#endif
  for (uint64 row_counter = 0; row_counter < SPI_processed; row_counter++) {
    HeapTuple cal_entries_tuple = SPI_tuptable->vals[row_counter];
    int32 calendar_id = DatumGetInt32(
        SPI_getbinval(cal_entries_tuple,
                      SPI_tuptable->tupdesc,
                      1,
                      &entry_is_null));
    int32 calendar_entry = DatumGetDateADT(
        SPI_getbinval(cal_entries_tuple,
                      SPI_tuptable->tupdesc,
                      2,
                      &entry_is_null));

    if (prev_calendar_id != calendar_id) {
      prev_calendar_id = calendar_id;
      entry_count = 0;
    }
    Calendar *calendar = pg_get_calendar(imcx_ptr, calendar_id);
    ereport(DEBUG2, errmsg(
        "Calendar Index: '%d', Calendar Id '%d', Calendar Entry (DateADT): '%d', Entry Count '%d'",
        get_calendar_index(imcx_ptr, calendar_id),
        calendar_id,
        calendar_entry,
        entry_count
    ));
    // Fill the Dates Entries
    calendar->dates[entry_count] = calendar_entry;
    entry_count++;
    // entry copy complete, calculate page size
    if (calendar->dates_size == entry_count) {
      int32 page_size_init_result = pg_init_page_size(calendar);
      if (page_size_init_result != RET_SUCCESS) {
        ereport(ERROR, errmsg("Page map could not be initialized. Error Code: %d", page_size_init_result));
      }
    }
  }
#ifndef NDEBUG
  ereport(DEF_DEBUG_LOG_LEVEL, errmsg("Q3: Cached %" PRIu64 " slices in total.", SPI_processed));
#endif
  SPI_finish();
  imcx_ptr->cache_filled = true;
  LWLockRelease(shared_memory_ptr->lock);
#ifndef NDEBUG
  ereport(DEF_DEBUG_LOG_LEVEL, errmsg("Exclusive Write Lock Released."));
#endif
  ereport(INFO, errmsg("Slices Loaded Into Cache."));
}

void add_row_to_2_col_tuple(
    AttInMetadata *att_in_metadata,
    Tuplestorestate *tuplestorestate,
    char *property,
    char *value
) {
  HeapTuple tuple;
  char *values[2];
  values[0] = property;
  values[1] = value;
  tuple = BuildTupleFromCStrings(att_in_metadata, values);
  tuplestore_puttuple(tuplestorestate, tuple);
}

void add_row_to_1_col_tuple(
    AttInMetadata *att_in_metadata,
    Tuplestorestate *tuplestorestate,
    char *value
) {
  HeapTuple tuple;
  char *values[1];
  values[0] = value;
  tuple = BuildTupleFromCStrings(att_in_metadata, values);
  tuplestore_puttuple(tuplestorestate, tuple);
}

PG_FUNCTION_INFO_V1(calendar_info);

Datum calendar_info(PG_FUNCTION_ARGS) {
  ensure_cache_populated();
  LWLockAcquire(shared_memory_ptr->lock, LW_SHARED);
  if (!LWLockHeldByMe(shared_memory_ptr->lock)) {
    ereport (ERROR, errmsg("Cannot Acquire Shared Read Lock."));
  }
#ifndef NDEBUG
  ereport (DEF_DEBUG_LOG_LEVEL, errmsg("Shared Read Lock Acquired."));
#endif

  ReturnSetInfo *pInfo = (ReturnSetInfo *)fcinfo->resultinfo;
  Tuplestorestate *p_tuplestorestate;
  AttInMetadata *attInMetadata;
  MemoryContext oldContext;
  TupleDesc tupleDesc;

  /* check to see if caller supports us returning a p_tuplestorestate */
  if (pInfo == NULL || !IsA (pInfo, ReturnSetInfo))
    ereport (ERROR,
             (errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
                 errmsg("set-valued function called in context that cannot accept a set")));
  if (!(pInfo->allowedModes & SFRM_Materialize))
    ereport (ERROR,
             (errcode(ERRCODE_SYNTAX_ERROR),
                 errmsg("materialize mode required, but it is not allowed in this context")));
  /* Build p_tuplestorestate to hold the result rows */
  oldContext = MemoryContextSwitchTo(pInfo->econtext->ecxt_per_query_memory); // Switch To Per Query Context

  tupleDesc = CreateTemplateTupleDesc(2);
  TupleDescInitEntry(tupleDesc,
                     (AttrNumber)1, "property", TEXTOID, -1, 0);
  TupleDescInitEntry(tupleDesc,
                     (AttrNumber)2, "value", TEXTOID, -1, 0);

  p_tuplestorestate = tuplestore_begin_heap(true, false, work_mem); // Create TupleStore
  pInfo->returnMode = SFRM_Materialize;
  pInfo->setResult = p_tuplestorestate;
  pInfo->setDesc = tupleDesc;
  MemoryContextSwitchTo(oldContext); // Switch back to previous context
  attInMetadata = TupleDescGetAttInMetadata(tupleDesc);

  add_row_to_2_col_tuple(attInMetadata, p_tuplestorestate,
                         "Version", CMAKE_VERSION);
#ifndef NDEBUG
  add_row_to_2_col_tuple(attInMetadata, p_tuplestorestate,
                         "Release Build", "No");
#else
  add_row_to_2_col_tuple(attInMetadata, p_tuplestorestate,
                         "Release Build", "Yes");
#endif
  add_row_to_2_col_tuple(attInMetadata, p_tuplestorestate,
                         "Cache Available", imcx_ptr->cache_filled ? "Yes" : "No");
  char slice_cache_size_str[32];
  int32_to_str(slice_cache_size_str, imcx_ptr->calendar_count);
  add_row_to_2_col_tuple(attInMetadata, p_tuplestorestate,
                         "Slice Cache Size (Calendar ID Count)", slice_cache_size_str);

  char entry_cache_size_str[32];
  int32_to_str(entry_cache_size_str, imcx_ptr->entry_count);
  add_row_to_2_col_tuple(attInMetadata, p_tuplestorestate,
                         "Entry Cache Size", entry_cache_size_str);

  char shared_memory_requested_str[32];
  double_to_str(shared_memory_requested_str, SHMEM_REQUESTED_MEMORY / 1024.0 / 1024.0, 2);
  add_row_to_2_col_tuple(attInMetadata, p_tuplestorestate,
                         "Shared Memory Requested (MBytes)",
                         shared_memory_requested_str);

  add_row_to_2_col_tuple(attInMetadata, p_tuplestorestate,
                         "[Q1] Get Calendar IDs",
                         q1_get_cal_min_max_id);

  add_row_to_2_col_tuple(attInMetadata, p_tuplestorestate,
                         "[Q2] Get Calendar Entry Count per Calendar ID",
                         q2_get_cal_entry_count);

  add_row_to_2_col_tuple(attInMetadata, p_tuplestorestate,
                         "[Q3] Get Calendar Entries",
                         q3_get_cal_entries);

  LWLockRelease(shared_memory_ptr->lock);
#ifndef NDEBUG
  ereport (DEF_DEBUG_LOG_LEVEL, errmsg("Shared Read Lock Released."));
#endif
  return (Datum)0;
}

PG_FUNCTION_INFO_V1(calendar_invalidate);
Datum calendar_invalidate(PG_FUNCTION_ARGS) {
  if (!imcx_ptr->cache_filled) {
    ereport (INFO, errmsg("Cache cannot be invalidated, is not yet loaded."));
    PG_RETURN_VOID ();
  }
  LWLockAcquire(shared_memory_ptr->lock, LW_EXCLUSIVE);
  if (!LWLockHeldByMe(shared_memory_ptr->lock)) {
    ereport (ERROR, errmsg("Cannot Acquire Exclusive Write Lock."));
  }
#ifndef NDEBUG
  ereport (DEF_DEBUG_LOG_LEVEL, errmsg("Exclusive Write Lock Acquired."));
#endif
  if (!imcx_ptr->cache_filled) {
    ereport (INFO, errmsg("Cache cannot be invalidated, is not yet loaded."));
    LWLockRelease(shared_memory_ptr->lock);
    PG_RETURN_VOID ();
  }
  int32 invalidate_result = cache_invalidate(imcx_ptr);
  LWLockRelease(shared_memory_ptr->lock);
#ifndef NDEBUG
  ereport (DEF_DEBUG_LOG_LEVEL, errmsg("Exclusive Write Lock Released."));
#endif
  if (invalidate_result == 0) {
    ereport (INFO, errmsg("Cache Invalidated Successfully"));
  } else {
    ereport (INFO, errmsg("Cache Cannot be Invalidated, Error Code: %d", invalidate_result));
  }

  PG_RETURN_VOID ();
}

int32 calendar_report_concrete(int32 showEntries, int32 showPageMapEntries, FunctionCallInfo fcinfo) {
  LWLockAcquire(shared_memory_ptr->lock, LW_SHARED);
  if (!LWLockHeldByMe(shared_memory_ptr->lock)) {
    ereport (ERROR, errmsg("Cannot Acquire Shared Read Lock."));
  }
#ifndef NDEBUG
  ereport (DEF_DEBUG_LOG_LEVEL, errmsg("Shared Read Lock Acquired."));
#endif

  ReturnSetInfo *p_return_set_info = (ReturnSetInfo *)fcinfo->resultinfo;
  Tuplestorestate *tuplestorestate;
  AttInMetadata *p_metadata;
  MemoryContext old_context;
  TupleDesc tuple_desc;
  // --
  /* check to see if caller supports us returning a tuplestorestate */
  if (p_return_set_info == NULL || !IsA (p_return_set_info, ReturnSetInfo))
    ereport (ERROR,
             (errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
                 errmsg("set-valued function called in context that cannot accept a set")));
  if (!(p_return_set_info->allowedModes & SFRM_Materialize))
    ereport (ERROR,
             (errcode(ERRCODE_SYNTAX_ERROR),
                 errmsg("materialize mode required, but it is not allowed in this context")));

  /* Build tuplestorestate to hold the result rows */
  old_context = MemoryContextSwitchTo(p_return_set_info->econtext
                                          ->ecxt_per_query_memory); // Switch To Per Query Context
  tuple_desc = CreateTemplateTupleDesc(2);
  TupleDescInitEntry(tuple_desc,
                     (AttrNumber)1, "property", TEXTOID, -1, 0);
  TupleDescInitEntry(tuple_desc,
                     (AttrNumber)2, "value", TEXTOID, -1, 0);
  tuplestorestate = tuplestore_begin_heap(true, false, work_mem); // Create TupleStore
  p_return_set_info->returnMode = SFRM_Materialize;
  p_return_set_info->setResult = tuplestorestate;
  p_return_set_info->setDesc = tuple_desc;
  MemoryContextSwitchTo(old_context); // Switch back to previous context

  p_metadata = TupleDescGetAttInMetadata(tuple_desc);
  // --
  char slices_id_max_str[4];
  int32_to_str(slices_id_max_str, imcx_ptr->calendar_count);
  add_row_to_2_col_tuple(p_metadata, tuplestorestate,
                         "Slices-Id Max", slices_id_max_str);
  char cache_calendars_size_str[4];
  int32_to_str(cache_calendars_size_str, imcx_ptr->entry_count);
  add_row_to_2_col_tuple(p_metadata, tuplestorestate,
                         "Cache-Calendars Size", cache_calendars_size_str);
  // --
  int32 no_calendar_id_counter = 0;
  for (int32 i = 0; i < imcx_ptr->calendar_count; i++) {
    Calendar *curr_calendar = imcx_ptr->calendars[i];
    if (curr_calendar == NULL) {
      ereport (ERROR, errmsg("Calendar Index '%d' is NULL, cannot continue. Total Calendars '%d'",
                             i,
                             imcx_ptr->calendar_count
      ));
    }
    if (curr_calendar->id == 0) {
      no_calendar_id_counter++;
      continue;
    }
    char slice_id_str[4];
    int32_to_str(slice_id_str, curr_calendar->id);
    add_row_to_2_col_tuple(p_metadata, tuplestorestate,
                           "SliceType-Id", slice_id_str);
    char slic_idx_str[4];
    int32_to_str(slic_idx_str, i);
    add_row_to_2_col_tuple(p_metadata, tuplestorestate,
                           "   Index", slic_idx_str);
    add_row_to_2_col_tuple(p_metadata, tuplestorestate,
                           "   Name", curr_calendar->name);
    char slice_dates_entries_str[8];
    int32_to_str(slice_dates_entries_str, curr_calendar->dates_size);
    add_row_to_2_col_tuple(p_metadata, tuplestorestate,
                           "   Entries", slice_dates_entries_str);
    char page_map_size_str[8];
    int32_to_str(page_map_size_str, curr_calendar->page_map_size);
    add_row_to_2_col_tuple(p_metadata, tuplestorestate,
                           "   Page Map Size", page_map_size_str);
    char page_size_str[8];
    int32_to_str(page_size_str, curr_calendar->page_size);
    add_row_to_2_col_tuple(p_metadata, tuplestorestate,
                           "   Page Size", page_size_str);
    if (showEntries) {
      for (int32 j = 0; j < curr_calendar->dates_size; j++) {
#ifndef NDEBUG
        ereport (DEBUG2, errmsg("Entry[%d]: %d", j, curr_calendar->dates[j]));
#endif
        char entry_str[32];
        int32_to_str(entry_str, curr_calendar->dates[j]);
        add_row_to_2_col_tuple(p_metadata, tuplestorestate,
                               "   Entry",
                               entry_str);
      }
    }
    if (showPageMapEntries) {
      for (int32 j = 0; j < curr_calendar->page_map_size; j++) {
#ifndef NDEBUG
        ereport (DEBUG2, errmsg("PageMap Entry[%d]: %d", j, curr_calendar->page_map[j]));
#endif
        char page_map_entry_str[8];
        int32_to_str(page_map_entry_str, curr_calendar->page_map[j]);
        add_row_to_2_col_tuple(p_metadata, tuplestorestate,
                               "   PageMap Entry",
                               page_map_entry_str);
      }
    }
  }
  char missing_slices_str[4];
  int32_to_str(missing_slices_str, no_calendar_id_counter);
  add_row_to_2_col_tuple(p_metadata, tuplestorestate,
                         "Missing Slices (id==0)",
                         missing_slices_str);
  LWLockRelease(shared_memory_ptr->lock);
#ifndef NDEBUG
  ereport (DEF_DEBUG_LOG_LEVEL, errmsg("Shared Read Lock Released."));
#endif
  return 0;
}

PG_FUNCTION_INFO_V1(calendar_report);

Datum calendar_report(PG_FUNCTION_ARGS) {
  ensure_cache_populated();
  int32 showEntries = PG_GETARG_INT32 (0);
  int32 showPageMapEntries = PG_GETARG_INT32 (1);
  calendar_report_concrete(showEntries, showPageMapEntries, fcinfo);
  return (Datum)0;
}

PG_FUNCTION_INFO_V1(add_calendar_days_by_id);

Datum
add_calendar_days_by_id(PG_FUNCTION_ARGS) {
  ensure_cache_populated();
  LWLockAcquire(shared_memory_ptr->lock, LW_SHARED);
  if (!LWLockHeldByMe(shared_memory_ptr->lock)) {
    ereport (ERROR, errmsg("Cannot Acquire Shared Read Lock."));
  }
#ifndef NDEBUG
  ereport (DEF_DEBUG_LOG_LEVEL, errmsg("Shared Read Lock Acquired."));
#endif

  int32 date = PG_GETARG_INT32 (0);
  int32 calendar_interval = PG_GETARG_INT32 (1);
  int32 calendar_id = PG_GETARG_INT32 (2);
  const Calendar *cal = pg_get_calendar(imcx_ptr, calendar_id);
  //
  int32 fd_idx;
  int32 rs_idx;
  int32 new_date;
  //
  int32 add_calendar_days_result = add_calendar_days(imcx_ptr,
                                                     cal,
                                                     date,
                                                     calendar_interval,
                                                     &new_date,
                                                     &fd_idx,
                                                     &rs_idx);
  //
  if (add_calendar_days_result == RET_ERROR_NOT_READY) {
    ereport (ERROR, errmsg("Cache is not ready."));
  }
#ifndef NDEBUG
  ereport (DEF_DEBUG_LOG_LEVEL, errmsg(
      "FirstDate-Idx: %d = %d, ResultDate-Idx: %d = %d",
      fd_idx,
      cal->dates[fd_idx],
      rs_idx,
      cal->dates[rs_idx]
  ));
#endif
  //
  LWLockRelease(shared_memory_ptr->lock);
#ifndef NDEBUG
  ereport (DEF_DEBUG_LOG_LEVEL, errmsg("Shared Read Lock Released."));
#endif
  PG_RETURN_DATEADT (new_date);
}

void popuplate_hash() {
  LWLockAcquire(shared_memory_ptr->lock, LW_EXCLUSIVE);
}

PG_FUNCTION_INFO_V1(add_calendar_days_by_name);
Datum
add_calendar_days_by_name(PG_FUNCTION_ARGS) {
  ensure_cache_populated();
  LWLockAcquire(shared_memory_ptr->lock, LW_SHARED);
  if (!LWLockHeldByMe(shared_memory_ptr->lock)) {
    ereport (ERROR, errmsg("Cannot Acquire Shared Read Lock."));
  }
#ifndef NDEBUG
  ereport (DEF_DEBUG_LOG_LEVEL, errmsg("Shared Read Lock Acquired."));
#endif
  // Vars
  int32 input_date = PG_GETARG_INT32 (0);
  int32 calendar_interval = PG_GETARG_INT32 (1);
  const VarChar *calendar_name_text = PG_GETARG_VARCHAR_P (2);
  // Calendar Name
  int32 calendar_name_size = VARSIZE(calendar_name_text) - VARHDRSZ;
  char calendar_name[CALENDAR_NAME_MAX_LEN] = {0};
  memcpy(calendar_name, (char *)VARDATA(calendar_name_text), calendar_name_size);
  // To Lowercase
  str_to_lowercase(calendar_name);
#ifndef NDEBUG
  ereport(DEF_DEBUG_LOG_LEVEL, errmsg("Calendar Name: %s, Len: %d", calendar_name, calendar_name_size));
#endif
  // Lookup for the Calendar
  int32 calendar_id = 0;
#ifndef NDEBUG
  ereport (DEF_DEBUG_LOG_LEVEL, errmsg("Trying to find Calendar with name '%s'", calendar_name));
#endif
  int32 get_calendar_result =
      pg_get_calendar_id_by_name(
          imcx_ptr,
          calendar_name,
          &calendar_id
      );
  if (get_calendar_result != RET_SUCCESS) {
    if (get_calendar_result == RET_ERROR_NOT_FOUND)
      ereport (ERROR, errmsg("Calendar '%s' does not exists.", calendar_name));
    if (get_calendar_result == RET_ERROR_UNSUPPORTED_OP)
      ereport (ERROR, errmsg("Cannot get calendar by name. (Out of Bounds)"));
  }
  const Calendar *calendar = pg_get_calendar(imcx_ptr, calendar_id);
#ifndef NDEBUG
  ereport (DEF_DEBUG_LOG_LEVEL, errmsg("Found Calendar with Name '%s' and ID '%d'", calendar_name, calendar->id));
#endif
  int32 fd_idx = 0;
  int32 rs_idx = 0;
  DateADT result_date;
  // Result Date
  int32 add_calendar_days_result = add_calendar_days(imcx_ptr,
                                                     calendar,
                                                     input_date,
                                                     calendar_interval,
                                                     &result_date,
                                                     &fd_idx,
                                                     &rs_idx);
#ifndef NDEBUG
  ereport (DEF_DEBUG_LOG_LEVEL, errmsg("Add Calendar Days Returned '%d'", add_calendar_days_result));
#endif
  if (add_calendar_days_result == RET_ERROR_NOT_READY) {
    ereport (ERROR, errmsg("Cache is not ready."));
  }
#ifndef NDEBUG
  ereport (DEF_DEBUG_LOG_LEVEL, errmsg(
      "FirstDate-Idx: %d = %d, ResultDate-Idx: %d = %d",
      fd_idx,
      calendar->dates[fd_idx],
      rs_idx,
      calendar->dates[rs_idx]
  ));
#endif
  LWLockRelease(shared_memory_ptr->lock);
#ifndef NDEBUG
  ereport (DEF_DEBUG_LOG_LEVEL, errmsg("Shared Read Lock Released."));
#endif
  PG_RETURN_DATEADT (result_date);
}