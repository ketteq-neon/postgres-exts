/**
 * (C) ketteQ, Inc.
 */

#include "cache.h"

// Private Functions

/**
 * Support function for deallocating hash map pointers (keys and values).
 * Should not be exported.
 * @param data
 */
void glib_value_free (gpointer data)
{
  free (data);
}

// Public Functions

int cache_init (IMCX *imcx, unsigned long min_calendar_id, unsigned long max_calendar_id)
{
  if (min_calendar_id > max_calendar_id)
	{
	  return RET_ERROR_MIN_GT_MAX;
	}
  unsigned long calendar_count = max_calendar_id - min_calendar_id + 1;
  //
  imcx->calendars = (Calendar *)calloc (calendar_count, sizeof (Calendar));
  if (imcx->calendars == NULL)
	{
	  // This happens when the OS can't give us that much memory.
	  return RET_ERROR_CANNOT_ALLOCATE;
	}
  //

  imcx->calendar_count = calendar_count;
  imcx->entry_count = 0;
  // Allocate the HashMap that will store the calendar names (str). This is a dynamic map.
  imcx->calendar_name_hashtable = g_hash_table_new_full
	  (g_str_hash,
	   g_str_equal,
	   glib_value_free,
	   glib_value_free);
  //
  return RET_SUCCESS;
}

int init_calendar (IMCX *imcx, unsigned long calendar_id, unsigned long entry_size)
{
  if (imcx->cache_filled) {
	  return RET_ERROR_UNSUPPORTED_OP;
  }
  if ((calendar_id - 1) < 0)
	{
	  return RET_ERROR_INVALID_PARAM;
	}
  unsigned long calendar_index = calendar_id - 1;
  Calendar *calendar = &imcx->calendars[calendar_index];
  calendar->dates = malloc (entry_size * sizeof (long));
  if (calendar->dates == NULL)
	{
	  return RET_ERROR_CANNOT_ALLOCATE;
	}
  calendar->calendar_id = calendar_id;
  calendar->dates_size = entry_size;
  imcx->entry_count += entry_size;
  return RET_SUCCESS;
}

void add_calendar_name (IMCX *imcx, unsigned long calendar_id, const char *calendar_name)
{
  unsigned long calendar_index = calendar_id - 1;
  Calendar calendar = imcx->calendars[calendar_index];
  // Convert Int to Str
  int num_len = snprintf (NULL, 0, "%ld", calendar.calendar_id);
  char *id_str = malloc ((num_len + 1) * sizeof (char));
  snprintf (id_str, num_len + 1, "%ld", calendar.calendar_id);
  //
  // TODO: Check how to save the id value as Int and not Str (char*) -> similar to IntHashMap
  char *calendar_name_ll = strdup (calendar_name);
  str_to_lowercase_self (calendar_name_ll);
  g_hash_table_insert (imcx->calendar_name_hashtable, calendar_name_ll, id_str);
}

int get_calendar_index_by_name (IMCX *imcx, const char *calendar_name, unsigned long *calendar_index)
{
  char *cal_name = strdup (calendar_name);
  str_to_lowercase_self (cal_name);
  _Bool found = g_hash_table_contains (imcx->calendar_name_hashtable, cal_name);
  if (found)
	{
	  const char *calendar_id_str = g_hash_table_lookup (imcx->calendar_name_hashtable, cal_name);
	  unsigned long calendar_id_l = strtoul (calendar_id_str, NULL, 10);
	  if (calendar_id_l > INT32_MAX)
		{
		  return RET_ERROR_UNSUPPORTED_OP;
		}
	  *calendar_index = calendar_id_l - 1;
	  return RET_SUCCESS;
	}
  return RET_ERROR_NOT_FOUND;
}

int init_page_size (Calendar *calendar)
{
  long last_date = calendar->dates[calendar->dates_size - 1];
  long first_date = calendar->dates[0];
  unsigned long entry_count = calendar->dates_size;
  //
  int page_size_tmp = calculate_page_size (first_date, last_date, entry_count);
  //
  if (page_size_tmp == 0)
	{
	  // Page size cannot be 0, cannot be calculated.
	  return RET_ERROR_UNSUPPORTED_OP;
	}
  //
  calendar->page_size = page_size_tmp;
  calendar->first_page_offset = first_date / page_size_tmp;
  // Allocation of the page map
  long page_end_index = calendar->dates[calendar->dates_size - 1] / calendar->page_size;
  calendar->page_map_size = page_end_index - calendar->first_page_offset + 1;
  // Assign memory and set the values to 0.
  calendar->page_map = calloc (calendar->page_map_size, sizeof (long));
  // Calculate Page Map
  int prev_page_index = 0;
  // int prev_date = -1; // This will be required if we check that the array is ordered.
  for (int date_index = 0; date_index < calendar->dates_size; date_index++)
	{
	  long curr_date = calendar->dates[date_index];
	  // TODO: Check if the date array is ordered or not, we assume it is.
	  long page_index = (curr_date / calendar->page_size) - calendar->first_page_offset;
	  while (prev_page_index < page_index)
		{
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
	unsigned long calendar_index,
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
  Calendar calendar = imcx->calendars[calendar_index];
  // Find the interval -> the closest date index from the left of the input_date in the calendar
  long prev_date_index = get_closest_index_from_left (input_date, calendar);
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
		  *result_date = calendar.dates[0]; // Returns first date of the calendar
		  return RET_ADD_DAYS_NEGATIVE;
		}
	  if (result_date_index >= calendar.dates_size) // Handle Positive OOB Indices (When interval is positive)
		{
		  *result_date = PG_INT32_MAX; // Returns infinity+.
		  return RET_ADD_DAYS_POSITIVE;
		}
	  *result_date = calendar.dates[result_date_index];
	  return RET_SUCCESS;
	}
  else
	{
	  // Handle Negative OOB Indices (When interval is negative)
	  *result_date = calendar.dates[0]; // Returns the first date of the calendar.
	  return RET_SUCCESS;
	}
}

int cache_invalidate (IMCX *imcx)
{
  if (!imcx->cache_filled) {
	  return RET_ERROR_UNSUPPORTED_OP;
  }
  if (imcx->calendar_count == 0)
	{
	  return RET_ERROR;
	}
  // Free Entries 1st
  for (int cc = 0; cc < imcx->calendar_count; cc++)
	{
	  free (imcx->calendars[cc].dates);
	}
  // Then Free Store
  free (imcx->calendars);
  // Then Destroy Hashmap
  g_hash_table_destroy (imcx->calendar_name_hashtable);
  // Reset Control Vars
  imcx->calendars = NULL;
  imcx->calendar_name_hashtable = NULL;
  imcx->calendar_count = 0;
  imcx->entry_count = 0;
  imcx->cache_filled = false;
  // Done
  return RET_SUCCESS;
}