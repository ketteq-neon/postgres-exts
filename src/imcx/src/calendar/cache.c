/**
 * (C) ketteQ, Inc.
 */

#include "imcx/src/include/cache.h"

int pg_cache_attach (IMCX *imcx)
{
  // Attach Names Hashtable
  HASHCTL info;
  memset (&info, 0, sizeof (info));
  info.keysize = CALENDAR_NAME_MAX_LEN;
  info.entrysize = sizeof (CalendarNameEntry);
  imcx->pg_calendar_name_hashtable = ShmemInitHash (CALENDAR_NAMES_HASH_NAME,
													(long)imcx->calendar_count,
													(long)imcx->calendar_count,
													&info,
													CALENDAR_NAMES_HASH_FLAGS);
  if (imcx->pg_calendar_name_hashtable == NULL)
	{
	  return RET_ERROR_CANNOT_ATTACH_SHMEM;
	}
  return RET_SUCCESS;
}

int pg_cache_init (IMCX *imcx, int min_calendar_id, int max_calendar_id)
{
  if (min_calendar_id > max_calendar_id)
	{
	  return RET_ERROR_MIN_GT_MAX;
	}
  unsigned long calendar_count = max_calendar_id - min_calendar_id + 1;
  // Allocate Calendars
  imcx->calendars = (Calendar **)ShmemAlloc (calendar_count * sizeof (Calendar *));
  if (imcx->calendars == NULL)
	{
	  return RET_ERROR_CANNOT_ALLOCATE;
	}
  // Allocate & Init each Calendar
  memset (imcx->calendars, 0, calendar_count * sizeof (Calendar *));
  for (unsigned long calendar_idx = 0; calendar_idx < calendar_count; calendar_idx++)
	{
	  imcx->calendars[calendar_idx] = ShmemAlloc (sizeof (Calendar));
	  memset (imcx->calendars[calendar_idx], 0, sizeof (Calendar));
	  if (imcx->calendars[calendar_idx] == NULL)
		{
		  return RET_ERROR_CANNOT_ALLOCATE;
		}
	}
  // Init control Vars
  imcx->calendar_count = calendar_count;
  imcx->entry_count = 0;
  imcx->min_calendar_id = min_calendar_id;
  if (imcx->calendar_count > LONG_MAX)
	{
	  // Calendar_count is too big.
	  return RET_ERROR_UNSUPPORTED_OP;
	}
  // Init Names HashTable
  HASHCTL info;
  memset (&info, 0, sizeof (info));
  info.keysize = CALENDAR_NAME_MAX_LEN;
  info.entrysize = sizeof (CalendarNameEntry);
  imcx->pg_calendar_name_hashtable = ShmemInitHash (CALENDAR_NAMES_HASH_NAME,
													(long)calendar_count,
													(long)calendar_count,
													&info,
													CALENDAR_NAMES_HASH_FLAGS);
  if (imcx->pg_calendar_name_hashtable == NULL)
	{
	  return RET_ERROR_CANNOT_ALLOCATE;
	}
  return RET_SUCCESS;
}

Calendar *pg_get_calendar (IMCX *imcx, int calendar_id)
{
  ereport(DEBUG2, errmsg ("CalendarId=%d, MinCalendarId=%d", calendar_id, imcx->min_calendar_id));
  return imcx->calendars[calendar_id - imcx->min_calendar_id];
}

int32 get_calendar_index (IMCX *imcx, int calendar_id)
{
  return calendar_id - imcx->min_calendar_id;
}

int pg_calendar_init (Calendar *calendar, int calendar_id, unsigned long entry_size, unsigned long *entry_count_ptr)
{
  calendar->dates = (int *)ShmemAlloc (entry_size * sizeof (int32));
  if (calendar->dates == NULL)
	{
	  return RET_ERROR_CANNOT_ALLOCATE;
	}
  memset (calendar->dates, 0, entry_size * sizeof (int32));
  calendar->id = calendar_id;
  calendar->dates_size = entry_size;
  *entry_count_ptr = *entry_count_ptr + entry_size;
  return RET_SUCCESS;
}

int pg_set_calendar_name (IMCX *imcx, Calendar *calendar, const char *calendar_name)
{
  // Allocate Key
  char *key_calendar_name = ShmemAlloc ((strlen (calendar_name) + 1) * sizeof (char));
  strcpy (key_calendar_name, calendar_name);
  // Create Entry
  bool entry_found = false;
  CalendarNameEntry *entry = hash_search (
	  imcx->pg_calendar_name_hashtable,
	  key_calendar_name,
	  HASH_ENTER,
	  &entry_found
  );
  if (entry_found)
	{
	  // Fail if already exists.
	  return RET_ERROR_UNSUPPORTED_OP;
	}
  // Set the key inside the calendar entry (required for key comparison)
  strcpy (entry->key, calendar_name);
  strcpy (calendar->name, calendar_name);
  entry->calendar_id = calendar->id;
  ereport(DEBUG1, errmsg ("Calendar name for calendar id = '%d' set to '%s'", calendar->id, entry->key));
  return RET_SUCCESS;
}

int pg_get_calendar_id_by_name (IMCX *imcx, const char *calendar_name, int *calendar_id)
{
  char *calendar_name_ll = strdup (calendar_name);
  // Lowercase user input
  str_to_lowercase_self (calendar_name_ll);
  // Find the Calendar
  bool entry_found = false;
  const CalendarNameEntry *entry = hash_search (
	  imcx->pg_calendar_name_hashtable,
	  calendar_name_ll, HASH_FIND,
	  &entry_found);
  if (entry_found)
	{
	  // Set the output var to the result
	  *calendar_id = entry->calendar_id;
	  return RET_SUCCESS;
	}
  return RET_ERROR_NOT_FOUND;
}

int pg_init_page_size (Calendar *calendar)
{
  long last_date = calendar->dates[calendar->dates_size - 1];
  long first_date = calendar->dates[0];
  unsigned long entry_count = calendar->dates_size;
  // Calculate Page Size
  int page_size_tmp = calculate_page_size (first_date, last_date, entry_count);
  //
  if (page_size_tmp == 0)
	{
	  // Page size cannot be 0, cannot be calculated.
	  return RET_ERROR_UNSUPPORTED_OP;
	}
  // Set Vars
  calendar->page_size = page_size_tmp;
  calendar->first_page_offset = first_date / page_size_tmp;
  // Allocation of the page map
  long page_end_index = calendar->dates[calendar->dates_size - 1] / calendar->page_size;
  calendar->page_map_size = page_end_index - calendar->first_page_offset + 1;
  // Assign memory and set the values to 0.
  calendar->page_map = ShmemAlloc (calendar->page_map_size * sizeof (long));
  memset (calendar->page_map, 0, calendar->page_map_size * sizeof (long));
  // Calculate Page Map
  int prev_page_index = 0;
  // int prev_date = -1; // This will be required if we need to check if that the array is ordered.
  for (int date_index = 0; date_index < calendar->dates_size; date_index++)
	{
	  long curr_date = calendar->dates[date_index];
	  long page_index = (curr_date / calendar->page_size) - calendar->first_page_offset;
	  while (prev_page_index < page_index)
		{
		  // Fill Page Map
		  calendar->page_map[++prev_page_index] = date_index;
		}
	}
  return RET_SUCCESS;
}

void cache_finish (IMCX *imcx)
{
  imcx->cache_filled = true;
}

int add_calendar_days (
	const IMCX *imcx,
	Calendar *calendar,
	long input_date,
	long interval,
	int32 *result_date,
	unsigned long *first_date_idx,
	unsigned long *result_date_idx
)
{
  if (!imcx->cache_filled)
	{
	  return RET_ERROR_NOT_READY;
	}
  // Find the interval -> the closest date index from the left of the input_date in the calendar
  long prev_date_index = get_closest_index_from_left (input_date, *calendar);
  // Now try to get the corresponding date of requested interval
  long result_date_index = prev_date_index + interval;
  // This can be useful for reporting or debugging.
  if (prev_date_index > 0 && first_date_idx != NULL)
	{
	  *first_date_idx = prev_date_index;
	}
  if (result_date_index > 0 && result_date_idx != NULL)
	{
	  *result_date_idx = result_date_index;
	}
  // Now check if inside boundaries.
  if (result_date_index >= 0) // If result_date_index is positive (negative interval is smaller than current index)
	{
	  if (prev_date_index < 0) // Handle Negative OOB Indices (When interval is negative)
		{
		  *result_date = calendar->dates[0]; // Returns first date of the calendar
		  return RET_ADD_DAYS_NEGATIVE;
		}
	  if (result_date_index >= calendar->dates_size) // Handle Positive OOB Indices (When interval is positive)
		{
		  *result_date = PG_INT32_MAX; // Returns infinity+.
		  return RET_ADD_DAYS_POSITIVE;
		}
	  *result_date = calendar->dates[result_date_index];
	  return RET_SUCCESS;
	}
  else
	{
	  // Handle Negative OOB Indices (When interval is negative)
	  *result_date = calendar->dates[0]; // Returns the first date of the calendar.
	  return RET_SUCCESS;
	}
}

int cache_invalidate (IMCX *imcx)
{
  if (!imcx->cache_filled)
	{
	  return RET_ERROR_UNSUPPORTED_OP;
	}
  if (imcx->calendar_count == 0)
	{
	  return RET_ERROR;
	}
  // Free Name HashTable
  HASH_SEQ_STATUS status;
  void *entry = NULL;
  hash_seq_init (&status, imcx->pg_calendar_name_hashtable);
  while ((entry = hash_seq_search (&status)) != 0)
	{
	  bool found = false;
	  hash_search (imcx->pg_calendar_name_hashtable, entry, HASH_REMOVE, &found);
	  Assert(found);
	}
  // Reset Entries
  for (int cc = 0; cc < imcx->calendar_count; cc++)
	{
	  imcx->calendars[cc]->dates = 0;
	}
  // Reset Control Vars
  imcx->calendars = NULL;
  imcx->calendar_count = 0;
  imcx->entry_count = 0;
  imcx->cache_filled = false;
  // Done
  return RET_SUCCESS;
}