/**
 * (C) KetteQ, Inc.
 */

#include "kq_imcx.h"

typedef struct {
	AttInMetadata *p_metadata;
	Tuplestorestate *tuplestorestate;
} TupleData;

typedef struct {
	char *search_id;
	char *name;
} GetNameData;

// Init of Extension

void _PG_init (void)
{
  init_shared_memory ();
  load_cache_concrete ();
  ereport(INFO, errmsg ("KetteQ In-Memory Calendar Extension Loaded."));
}

void _PG_fini (void)
{
  ereport(INFO, errmsg ("Unloaded KetteQ In-Memory Calendar Extension."));
}

typedef struct {
	char *data; // Test Data Pointer
	int tranche_id;
	LWLock lock;
	// -- imcx
	IMCX *imcx;
} IMCXSharedMemory;

IMCXSharedMemory *imcx_shared_memory;

void init_shared_memory ()
{
  bool shared_memory_found;
  LWLockAcquire (AddinShmemInitLock, LW_EXCLUSIVE);
  RequestAddinShmemSpace ((size_t)SHARED_MEMORY_DEF); // 1 GB
  imcx_shared_memory = ShmemInitStruct ("IMCXSharedMemory", sizeof (IMCXSharedMemory), &shared_memory_found);
  if (!shared_memory_found)
	{
	  memset (imcx_shared_memory, 0, sizeof (IMCXSharedMemory));
	  imcx_shared_memory->imcx = (IMCX *)ShmemAlloc (sizeof (IMCX));
	  memset (imcx_shared_memory->imcx, 0, sizeof (IMCX));
	  imcx_shared_memory->tranche_id = LWLockNewTrancheId ();
	  LWLockRegisterTranche (imcx_shared_memory->tranche_id, TRANCHE_NAME);
	  LWLockInitialize (&imcx_shared_memory->lock, imcx_shared_memory->tranche_id);
	}
  LWLockRelease (AddinShmemInitLock);
  ereport(DEF_DEBUG_LOG_LEVEL, errmsg ("Initialized Shared Memory."));
}

void load_cache_concrete ()
{
  if (imcx_shared_memory->imcx->cache_filled)
	{
	  // Cache already filled
	  return;
	}
  LWLockAcquire (&imcx_shared_memory->lock, LW_EXCLUSIVE);
  if (!LWLockHeldByMe (&imcx_shared_memory->lock))
	{
	  ereport(ERROR, errmsg ("Cannot acquire Exclusive Write Lock."));
	}
  ereport(DEF_DEBUG_LOG_LEVEL, errmsg ("Exclusive Write Lock Acquired."));
  // Vars
  uint64 prev_calendar_id = 0; // Previous Calendar ID
  uint64 entry_count = 0; // Entry Counter
  // Row Control
  bool entry_is_null; // Pointer to boolean, TRUE if the last entry was NULL
  // Query #1, Get MIN_ID and MAX_ID of Calendars
  char const *sql_get_min_max = "select min(s.id), max(s.id) from ketteq.slice_type s"; // 1 - 74079
  // Query #2, Get Calendar's Entries Count and Names
  char const *sql_get_entries_count_per_calendar_id = "select s.slice_type_id, count(*), "
													  "(select st.\"name\" from ketteq.slice_type st where st.id = s.slice_type_id) \"name\" "
													  "from ketteq.slice s "
													  "group by s.slice_type_id "
													  "order by s.slice_type_id asc;";
  // Query #3, Get Entries of Calendars
  char const *sql_get_entries = "select s.slice_type_id, s.start_on "
								"from ketteq.slice s "
								"order by s.slice_type_id asc, "
								"s.start_on asc;";
  // Connect to SPI
  int spi_connect_result = 0;
  if ((spi_connect_result = SPI_connect ()) < 0)
	{
	  ereport(ERROR, errmsg ("SPI_connect returned %d", spi_connect_result));
	}
  // Execute Q1, get the min and max id's
  if (SPI_execute (sql_get_min_max, true, 0) != SPI_OK_SELECT || SPI_processed == 0)
	{
	  SPI_finish ();
	  ereport(ERROR, errmsg ("No calendars."));
	}

  // Get the Data and Descriptor
  HeapTuple min_max_tuple = SPI_tuptable->vals[0];
  //
  uint64 min_value = DatumGetUInt64(
	  SPI_getbinval (min_max_tuple,
					 SPI_tuptable->tupdesc,
					 1,
					 &entry_is_null));
  uint64 max_value = DatumGetUInt64(
	  SPI_getbinval (min_max_tuple,
					 SPI_tuptable->tupdesc,
					 2,
					 &entry_is_null));
  // Init the Struct Cache
  if (cache_init (imcx_shared_memory->imcx, min_value, max_value) < 0)
	{
	  ereport(ERROR, errmsg ("Shared Memory Cannot Be Allocated (cache_init, %" PRIu64 ", %" PRIu64 ")", min_value, max_value));
	}
  ereport(DEF_DEBUG_LOG_LEVEL, errmsg (
	  "Memory Allocated (Q1), Min-Value: '%ld' Max-Value: '%ld'",
	  min_value,
	  max_value
  ));
  // Init the Structs date property with the count of the entries
  if (SPI_execute (sql_get_entries_count_per_calendar_id, true, 0) != SPI_OK_SELECT || SPI_processed == 0)
	{
	  SPI_finish ();
	  ereport(ERROR, errmsg ("Cannot count calendar's entries."));
	}
  ereport(DEF_DEBUG_LOG_LEVEL, errmsg ("Q2: Got %" PRIu64 " SliceTypes.", SPI_processed));

  for (uint64 row_counter = 0; row_counter < SPI_processed; row_counter++)
	{
	  HeapTuple cal_entries_count_tuple = SPI_tuptable->vals[row_counter];
	  uint64 calendar_id = DatumGetUInt64(
		  SPI_getbinval (cal_entries_count_tuple,
						 SPI_tuptable->tupdesc,
						 1,
						 &entry_is_null));
	  if (entry_is_null) {
		  continue;
	  }
	  uint64 calendar_entry_count = DatumGetUInt64(
		  SPI_getbinval (cal_entries_count_tuple,
						 SPI_tuptable->tupdesc,
						 2,
						 &entry_is_null));
	  char *calendar_name = DatumGetCString(
		  SPI_getvalue (
			  cal_entries_count_tuple,
			  SPI_tuptable->tupdesc,
			  3
		  )
	  );

	  ereport(DEF_DEBUG_LOG_LEVEL,
			  errmsg (
				  "Q2 (Cursor): Got: SliceTypeName: %s, SliceType: %" PRIu64 ", Entries: %" PRIu64,
				  calendar_name,
				  calendar_id,
				  calendar_entry_count
			  ));
	  // Add to the cache
	  imcx_shared_memory->imcx->calendars[calendar_id - 1].id = calendar_id;
	  calendar_init (
		  imcx_shared_memory->imcx,
		  calendar_id,
		  calendar_entry_count
	  );
	  ereport(DEF_DEBUG_LOG_LEVEL, errmsg (
		  "Calendar Initialized, Index: '%ld', ID: '%ld', Entry count: '%ld'",
		  calendar_id - 1,
		  calendar_id,
		  calendar_entry_count
	  ));
	  // Add The Calendar Name
	  set_calendar_name (imcx_shared_memory->imcx, calendar_id - 1, calendar_name);
	  ereport(DEF_DEBUG_LOG_LEVEL, errmsg ("Calendar Name '%s' Set", calendar_name));
	}
  ereport(DEF_DEBUG_LOG_LEVEL, errmsg ("Executing Q3"));
  // -> Exec Q3
  // Check Results
  if (SPI_execute (sql_get_entries, true, 0) != SPI_OK_SELECT || SPI_processed == 0)
	{
	  SPI_finish ();
	  ereport(ERROR, errmsg ("No calendar entries."));
	}
  ereport(DEF_DEBUG_LOG_LEVEL, errmsg ("Q3: RowCount: '%ld'", SPI_processed));
  for (uint64 row_counter = 0; row_counter < SPI_processed; row_counter++)
	{
	  HeapTuple cal_entries_tuple = SPI_tuptable->vals[row_counter];
	  uint64 calendar_id = DatumGetUInt64(
		  SPI_getbinval (cal_entries_tuple,
						 SPI_tuptable->tupdesc,
						 1,
						 &entry_is_null));
	  int32 calendar_entry = DatumGetDateADT(
		  SPI_getbinval (cal_entries_tuple,
						 SPI_tuptable->tupdesc,
						 2,
						 &entry_is_null));
	  uint64 calendar_index = calendar_id - 1;

	  if (prev_calendar_id != calendar_id)
		{
		  prev_calendar_id = calendar_id;
		  entry_count = 0;
		}

	  ereport(DEBUG2, errmsg (
		  "Calendar Id '%ld', Calendar Entry (DateADT): '%d', Calendar Index: '%ld', Entry Count '%ld'",
		  calendar_id,
		  calendar_entry,
		  calendar_index,
		  entry_count
	  ));

	  imcx_shared_memory->imcx->calendars[calendar_index].dates[entry_count] = calendar_entry;
	  entry_count++;
	  if (imcx_shared_memory->imcx->calendars[calendar_index].dates_size == entry_count)
		{
		  // entry copy complete, calculate page size
		  init_page_size (&imcx_shared_memory->imcx->calendars[calendar_index]);
		}
	}
  ereport(DEF_DEBUG_LOG_LEVEL, errmsg ("Q3: Cached %" PRIu64 " slices in total.", SPI_processed));
  SPI_finish ();
  cache_finish (imcx_shared_memory->imcx);
  LWLockRelease (&imcx_shared_memory->lock);
  ereport(DEF_DEBUG_LOG_LEVEL, errmsg ("Exclusive Write Lock Released."));
  ereport(INFO, errmsg ("Slices Loaded Into Cache."));
}

void add_row_to_2_col_tuple (
	AttInMetadata *att_in_metadata,
	Tuplestorestate *tuplestorestate,
	char *property,
	char *value
)
{
  HeapTuple tuple;
  char *values[2];
  values[0] = property;
  values[1] = value;
  tuple = BuildTupleFromCStrings (att_in_metadata, values);
  tuplestore_puttuple (tuplestorestate, tuple);
}

void add_row_to_1_col_tuple (
	AttInMetadata *att_in_metadata,
	Tuplestorestate *tuplestorestate,
	char *value
)
{
  HeapTuple tuple;
  char *values[1];
  values[0] = value;
  tuple = BuildTupleFromCStrings (att_in_metadata, values);
  tuplestore_puttuple (tuplestorestate, tuple);
}

PG_FUNCTION_INFO_V1(calendar_info);

Datum calendar_info (PG_FUNCTION_ARGS)
{
  LWLockAcquire (&imcx_shared_memory->lock, LW_SHARED);
  if (!LWLockHeldByMe (&imcx_shared_memory->lock))
	{
	  ereport(ERROR, errmsg ("Cannot Acquire Shared Read Lock."));
	}
  ereport(DEF_DEBUG_LOG_LEVEL, errmsg ("Shared Read Lock Acquired."));


  ReturnSetInfo *pInfo = (ReturnSetInfo *)fcinfo->resultinfo;
  Tuplestorestate *tuplestorestate;
  AttInMetadata *attInMetadata;
  MemoryContext oldContext;
  TupleDesc tupleDesc;

  /* check to see if caller supports us returning a tuplestorestate */
  if (pInfo == NULL || !IsA(pInfo, ReturnSetInfo))
	ereport(ERROR,
			(errcode (ERRCODE_FEATURE_NOT_SUPPORTED),
				errmsg ("set-valued function called in context that cannot accept a set")));
  if (!(pInfo->allowedModes & SFRM_Materialize))
	ereport(ERROR,
			(errcode (ERRCODE_SYNTAX_ERROR),
				errmsg ("materialize mode required, but it is not allowed in this context")));
  /* Build tuplestorestate to hold the result rows */
  oldContext = MemoryContextSwitchTo (pInfo->econtext->ecxt_per_query_memory); // Switch To Per Query Context

  tupleDesc = CreateTemplateTupleDesc (2);
  TupleDescInitEntry (tupleDesc,
					  (AttrNumber)1, "property", TEXTOID, -1, 0);
  TupleDescInitEntry (tupleDesc,
					  (AttrNumber)2, "value", TEXTOID, -1, 0);

  tuplestorestate = tuplestore_begin_heap (true, false, work_mem); // Create TupleStore
  pInfo->returnMode = SFRM_Materialize;
  pInfo->setResult = tuplestorestate;
  pInfo->setDesc = tupleDesc;
  MemoryContextSwitchTo (oldContext); // Switch back to previous context
  attInMetadata = TupleDescGetAttInMetadata (tupleDesc);

  add_row_to_2_col_tuple (attInMetadata, tuplestorestate,
						  "Version", CMAKE_VERSION);
  add_row_to_2_col_tuple (attInMetadata, tuplestorestate,
						  "Cache Available", imcx_shared_memory->imcx->cache_filled ? "Yes" : "No");
  add_row_to_2_col_tuple (attInMetadata, tuplestorestate,
						  "Slice Cache Size (SliceType Count)", convert_u_long_to_str (imcx_shared_memory->imcx
																						   ->calendar_count));
  add_row_to_2_col_tuple (attInMetadata, tuplestorestate,
						  "Entry Cache Size (Slices)", convert_u_long_to_str (imcx_shared_memory->imcx->entry_count));
  add_row_to_2_col_tuple (attInMetadata, tuplestorestate,
						  "Shared Memory Requested (MBytes)",
						  convert_double_to_str (SHARED_MEMORY_DEF / 1024.0 / 1024.0, 2));

  LWLockRelease(&imcx_shared_memory->lock);
  ereport(DEF_DEBUG_LOG_LEVEL, errmsg ("Shared Read Lock Released."));
  return (Datum)0;
}

PG_FUNCTION_INFO_V1(calendar_invalidate);

Datum calendar_invalidate (PG_FUNCTION_ARGS)
{
  LWLockAcquire (&imcx_shared_memory->lock, LW_EXCLUSIVE);
  if (!LWLockHeldByMe (&imcx_shared_memory->lock))
	{
	  ereport(ERROR, errmsg ("Cannot Acquire Exclusive Write Lock."));
	}
  ereport(DEF_DEBUG_LOG_LEVEL, errmsg ("Exclusive Write Lock Acquired."));

  ReturnSetInfo *p_return_set_info = (ReturnSetInfo *)fcinfo->resultinfo;
  Tuplestorestate *tuplestorestate;
  AttInMetadata *p_metadata;
  TupleDesc tuple_desc;

  /* check to see if caller supports us returning a tuplestorestate */
  if (p_return_set_info == NULL || !IsA(p_return_set_info, ReturnSetInfo))
	ereport(ERROR,
			(errcode (ERRCODE_FEATURE_NOT_SUPPORTED),
				errmsg ("set-valued function called in context that cannot accept a set")));
  if (!(p_return_set_info->allowedModes & SFRM_Materialize))
	ereport(ERROR,
			(errcode (ERRCODE_SYNTAX_ERROR),
				errmsg ("materialize mode required, but it is not allowed in this context")));

  int ret = cache_invalidate (imcx_shared_memory->imcx);
  LWLockRelease (&imcx_shared_memory->lock);
  ereport(DEF_DEBUG_LOG_LEVEL, errmsg ("Exclusive Write Lock Released."));

  tuple_desc = CreateTemplateTupleDesc (1);
  TupleDescInitEntry (tuple_desc,
					  (AttrNumber)1, "value", TEXTOID, -1, 0);

  tuplestorestate = tuplestore_begin_heap (true, false, work_mem);
  p_return_set_info->returnMode = SFRM_Materialize;
  p_return_set_info->setResult = tuplestorestate;
  p_return_set_info->setDesc = tuple_desc;
  p_metadata = TupleDescGetAttInMetadata (tuple_desc);
  if (ret == 0)
	{
	  load_cache_concrete ();
	  add_row_to_1_col_tuple (p_metadata, tuplestorestate, "Cache Invalidated");
	}
  else
	{
	  add_row_to_1_col_tuple (p_metadata, tuplestorestate, "Error Invalidating the Cache");
	}
  return (Datum)0;
}

void report_names (gpointer _key, gpointer _value, gpointer _user_data)
{
  char *key = _key;
  char *value = _value;
  TupleData *tupleData = (TupleData *)_user_data;
  add_row_to_2_col_tuple (tupleData->p_metadata, tupleData->tuplestorestate,
						  key,
						  value);
}

void get_name (gpointer _key, gpointer _value, gpointer _user_data)
{
  char *key = (char*) _key;
  const char *value = (const char*) _value;
  GetNameData *user = _user_data;
  if (strcmp (user->search_id, value) == 0)
	{
	  user->name = key;
	}
}

int imcx_report_concrete (int showEntries, int showPageMapEntries, FunctionCallInfo fcinfo)
{
  LWLockAcquire (&imcx_shared_memory->lock, LW_SHARED);
  if (!LWLockHeldByMe (&imcx_shared_memory->lock))
	{
	  ereport(ERROR, errmsg ("Cannot Acquire Shared Read Lock."));
	}
  ereport(DEF_DEBUG_LOG_LEVEL, errmsg ("Shared Read Lock Acquired."));


  ReturnSetInfo *p_return_set_info = (ReturnSetInfo *)fcinfo->resultinfo;
  Tuplestorestate *tuplestorestate;
  AttInMetadata *p_metadata;
  MemoryContext old_context;
  TupleDesc tuple_desc;
  // --
  /* check to see if caller supports us returning a tuplestorestate */
  if (p_return_set_info == NULL || !IsA(p_return_set_info, ReturnSetInfo))
	ereport(ERROR,
			(errcode (ERRCODE_FEATURE_NOT_SUPPORTED),
				errmsg ("set-valued function called in context that cannot accept a set")));
  if (!(p_return_set_info->allowedModes & SFRM_Materialize))
	ereport(ERROR,
			(errcode (ERRCODE_SYNTAX_ERROR),
				errmsg ("materialize mode required, but it is not allowed in this context")));

  /* Build tuplestorestate to hold the result rows */
  old_context = MemoryContextSwitchTo (p_return_set_info->econtext
										   ->ecxt_per_query_memory); // Switch To Per Query Context
  tuple_desc = CreateTemplateTupleDesc (2);
  TupleDescInitEntry (tuple_desc,
					  (AttrNumber)1, "property", TEXTOID, -1, 0);
  TupleDescInitEntry (tuple_desc,
					  (AttrNumber)2, "value", TEXTOID, -1, 0);
  tuplestorestate = tuplestore_begin_heap (true, false, work_mem); // Create TupleStore
  p_return_set_info->returnMode = SFRM_Materialize;
  p_return_set_info->setResult = tuplestorestate;
  p_return_set_info->setDesc = tuple_desc;
  MemoryContextSwitchTo (old_context); // Switch back to previous context

  p_metadata = TupleDescGetAttInMetadata (tuple_desc);
  // --
  add_row_to_2_col_tuple (p_metadata, tuplestorestate,
						  "Slices-Id Max", convert_u_long_to_str (imcx_shared_memory->imcx->calendar_count));
  add_row_to_2_col_tuple (p_metadata, tuplestorestate,
						  "Cache-Calendars Size", convert_u_int_to_str (sizeof (&imcx_shared_memory->imcx->calendars)));
  // --
  int noCalendarIdCounter = 0;
  for (ulong i = 0; i < imcx_shared_memory->imcx->calendar_count; i++)
	{
	  Calendar curr_calendar = imcx_shared_memory->imcx->calendars[i];
	  if (curr_calendar.id == 0)
		{
		  noCalendarIdCounter++;
		  continue;
		}
	  add_row_to_2_col_tuple (p_metadata, tuplestorestate,
							  "SliceType-Id", convert_u_long_to_str (curr_calendar.id));
	  // -- Get The Name
	  GetNameData getNameData;
	  getNameData.search_id = convert_u_long_to_str (curr_calendar.id);
	  g_hash_table_foreach (imcx_shared_memory->imcx->calendar_name_hashtable, get_name, &getNameData);
	  // --
	  add_row_to_2_col_tuple (p_metadata, tuplestorestate,
							  "   Name", getNameData.name);
	  add_row_to_2_col_tuple (p_metadata, tuplestorestate,
							  "   Entries", convert_long_to_str (curr_calendar.dates_size));
	  add_row_to_2_col_tuple (p_metadata, tuplestorestate,
							  "   Page Map Size", convert_long_to_str (curr_calendar.page_map_size));
	  add_row_to_2_col_tuple (p_metadata, tuplestorestate,
							  "   Page Size", convert_int_to_str (curr_calendar.page_size));
	  if (showEntries)
		{
		  for (unsigned long j = 0; j < curr_calendar.dates_size; j++)
			{
			  ereport(INFO, errmsg ("Entry[%lu]: %d", j, curr_calendar.dates[j]));

			  add_row_to_2_col_tuple (p_metadata, tuplestorestate,
									  "   Entry",
									  convert_int_to_str(curr_calendar.dates[j]));
			}
		}
	  if (showPageMapEntries)
		{
		  for (int j = 0; j < curr_calendar.page_map_size; j++)
			{
			  ereport(INFO, errmsg ("PageMap Entry[%d]: %ld", j, curr_calendar.page_map[j]));
			  add_row_to_2_col_tuple (p_metadata, tuplestorestate,
									  "   PageMap Entry",
									  convert_long_to_str(curr_calendar.page_map[j]));
			}
		}
	}
  add_row_to_2_col_tuple (p_metadata, tuplestorestate,
						  "Missing Slices (id==0)",
						  convert_int_to_str (noCalendarIdCounter));
  LWLockRelease (&imcx_shared_memory->lock);
  ereport(DEF_DEBUG_LOG_LEVEL, errmsg ("Shared Read Lock Released."));
  return 0;
}

PG_FUNCTION_INFO_V1(calendar_report);

Datum calendar_report (PG_FUNCTION_ARGS)
{
  int32 showEntries = PG_GETARG_INT32(0);
  int32 showPageMapEntries = PG_GETARG_INT32(1);
  imcx_report_concrete (showEntries, showPageMapEntries, fcinfo);
  return (Datum)0;
}

PG_FUNCTION_INFO_V1(add_calendar_days_by_id);

Datum
add_calendar_days_by_id (PG_FUNCTION_ARGS)
{
  LWLockAcquire (&imcx_shared_memory->lock, LW_SHARED);
  if (!LWLockHeldByMe (&imcx_shared_memory->lock))
	{
	  ereport(ERROR, errmsg ("Cannot Acquire Shared Read Lock."));
	}
  ereport(DEF_DEBUG_LOG_LEVEL, errmsg ("Shared Read Lock Acquired."));

  int32 date = PG_GETARG_INT32(0);
  int32 calendar_interval = PG_GETARG_INT32(1);
  int32 calendar_id = PG_GETARG_INT32(2);
  int32 calendar_index = calendar_id - 1;
  Calendar cal = imcx_shared_memory->imcx->calendars[calendar_index];
  //
  unsigned long fd_idx;
  unsigned long rs_idx;
  int32 new_date;
  //
  int add_calendar_days_result = add_calendar_days (imcx_shared_memory->imcx,
										calendar_index,
										date,
										calendar_interval,
										&new_date,
										&fd_idx,
										&rs_idx);
  //
  if (add_calendar_days_result == RET_ERROR_NOT_READY) {
	  ereport(ERROR, errmsg ("Cache is not ready."));
	}
  ereport(DEBUG1, errmsg (
	  "FirstDate-Idx: %lu = %d, ResultDate-Idx: %lu = %d",
	  fd_idx,
	  cal.dates[fd_idx],
	  rs_idx,
	  cal.dates[rs_idx]
  ));
  //
  LWLockRelease (&imcx_shared_memory->lock);
  ereport(DEF_DEBUG_LOG_LEVEL, errmsg ("Shared Read Lock Released."));
  PG_RETURN_DATEADT(new_date);
}

PG_FUNCTION_INFO_V1(add_calendar_days_by_name);

Datum
add_calendar_days_by_name (PG_FUNCTION_ARGS)
{
  LWLockAcquire (&imcx_shared_memory->lock, LW_SHARED);
  if (!LWLockHeldByMe (&imcx_shared_memory->lock))
	{
	  ereport(ERROR, errmsg ("Cannot Acquire Shared Read Lock."));
	}
  ereport(DEF_DEBUG_LOG_LEVEL, errmsg ("Shared Read Lock Acquired."));
  // Vars
  int32 input_date = PG_GETARG_INT32(0);
  int32 calendar_interval = PG_GETARG_INT32(1);
  const text *calendar_name = PG_GETARG_TEXT_P(2);
  // Convert Name to CString
  const char *calendar_name_str = text_to_cstring (calendar_name);
  // Lookup for the Calendar
  unsigned long calendar_index = 0;
  ereport(DEF_DEBUG_LOG_LEVEL, errmsg ("Trying to find Calendar with name '%s'", calendar_name_str));
  int get_calendar_result =
	  get_calendar_index_by_name (
		  imcx_shared_memory->imcx,
		  calendar_name_str,
		  &calendar_index
		  );
  if (get_calendar_result != RET_SUCCESS)
	{
	  if (get_calendar_result == RET_ERROR_NOT_FOUND)
	  	ereport(ERROR, errmsg ("Calendar does not exists."));
	  if (get_calendar_result == RET_ERROR_UNSUPPORTED_OP)
		ereport(ERROR, errmsg ("Cannot get calendar by name. (Out of Bounds)"));
	}
  const Calendar * calendar = &imcx_shared_memory->imcx->calendars[calendar_index];
  ereport(DEF_DEBUG_LOG_LEVEL, errmsg ("Found Calendar with Name '%s' and ID '%lu'", calendar_name_str, calendar->id));
  unsigned long fd_idx = 0;
  unsigned long rs_idx = 0;
  DateADT result_date;
  // Result Date
  int add_calendar_days_result = add_calendar_days (imcx_shared_memory->imcx,
										calendar_index,
										input_date,
										calendar_interval,
										&result_date,
										&fd_idx,
										&rs_idx);
  ereport(DEF_DEBUG_LOG_LEVEL, errmsg ("Add Calendar Days Returned '%d'", add_calendar_days_result));
  if (add_calendar_days_result == RET_ERROR_NOT_READY) {
	  ereport(ERROR, errmsg ("Cache is not ready."));
  }
  ereport(DEBUG1, errmsg (
	  "FirstDate-Idx: %lu = %d, ResultDate-Idx: %lu = %d",
	  fd_idx,
	  calendar->dates[fd_idx],
	  rs_idx,
	  calendar->dates[rs_idx]
  ));
  LWLockRelease (&imcx_shared_memory->lock);
  ereport(DEF_DEBUG_LOG_LEVEL, errmsg ("Shared Read Lock Released."));
  PG_RETURN_DATEADT(result_date);
}