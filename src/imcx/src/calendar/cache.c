/**
 * (C) ketteQ, Inc.
 */

#include "cache.h"
#include "src/common/util.h"

/**
 * Support function for deallocating hash map pointers (keys and values).
 * Should not be exported.
 * @param data
 */
void glib_value_free (gpointer data)
{
  free (data);
}

/**
 * This INIT will allocate the calendar struct in-mem storage.
 * @param min_calendar_id
 * @param max_calendar_id
 * @return
 */
int cache_init (IMCX *imcx, ulong min_calendar_id, ulong max_calendar_id)
{
  if (min_calendar_id > max_calendar_id)
	{
	  return RET_MIN_GT_MAX;
	}
  unsigned long calendar_count = max_calendar_id - min_calendar_id + 1;
  //
  imcx->calendars = (Calendar *)calloc (calendar_count, sizeof (Calendar));
  if (imcx->calendars == NULL)
	{
	  // This happens when the OS can't give us that much memory.
	  return RET_NULL_VALUE;
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
  return RET_OK;
}

int init_calendar (IMCX *imcx, ulong calendar_id, ulong entry_size)
{
  if ((calendar_id - 1) < 0)
	{
	  return RET_MIN_GT_MAX;
	}
  ulong calendar_index = calendar_id - 1;
  Calendar *calendar = &imcx->calendars[calendar_index];
  calendar->dates = malloc (entry_size * sizeof (long));
  if (calendar->dates == NULL)
	{
	  return RET_NULL_VALUE;
	}
  calendar->calendar_id = calendar_id;
  calendar->dates_size = entry_size;
  imcx->entry_count += entry_size;
  return RET_OK;
}

static void stdc_display_hash (gpointer key, gpointer value, gpointer user_data)
{
  printf ("Key: %s, Value: %s", (char *)key, (char *)value);
}

void add_calendar_name (IMCX *imcx, ulong calendar_id, const char *calendar_name)
{
  ulong calendar_index = calendar_id - 1;
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

long get_calendar_index_by_name (IMCX *imcx, const char *calendar_name)
{
  char *cal_name = strdup(calendar_name);
  str_to_lowercase_self (cal_name);
  _Bool found = g_hash_table_contains (imcx->calendar_name_hashtable, cal_name);
  if (found)
	{
	  const char *calendar_id_str = g_hash_table_lookup (imcx->calendar_name_hashtable, cal_name);
	  long calendar_id_l = strtol (calendar_id_str, NULL, 10);
	  if (calendar_id_l > INT32_MAX)
		{
		  // out of bounds.
		  return -1;
		}
	  return calendar_id_l - 1;
	}
  return -1;
}

int init_page_size (Calendar *calendar)
{
  long last_date = calendar->dates[calendar->dates_size - 1];
  long first_date = calendar->dates[0];
  ulong entry_count = calendar->dates_size;
  //
  int page_size_tmp = calculate_page_size (first_date, last_date, entry_count);
  //
  if (page_size_tmp == 0)
	{
	  // Page size cannot be 0, cannot be calculated.
	  return -1;
	}
  //
  calendar->page_size = page_size_tmp;
  calendar->first_page_offset = first_date / page_size_tmp;
  // Allocation of the page map
  int page_end_index = calendar->dates[calendar->dates_size - 1] / calendar->page_size;
  calendar->page_map_size = page_end_index - calendar->first_page_offset + 1;
  // Assign memory and set the values to 0.
  calendar->page_map = calloc (calendar->page_map_size, sizeof (int));
  //
  // Calculate Page Map
  int prev_page_index = 0;
  // int prev_date = -1; // This will be required if we check that the array is ordered.
  for (int date_index = 0; date_index < calendar->dates_size; date_index++)
	{
	  int curr_date = calendar->dates[date_index];
	  // TODO: Check if the date array is ordered or not, we assume it is.
	  int page_index = (curr_date / calendar->page_size) - calendar->first_page_offset;
	  while (prev_page_index < page_index)
		{
		  calendar->page_map[++prev_page_index] = date_index;
		}
	}
  return 0;
}

void cache_finish (IMCX *imcx)
{
  imcx->cache_filled = true;
}

/**
 * Main Extension Function
 * Adds (or subtracts) intervals of the given date based on the given calendar type (monthly, weekly, etc)
 * @param input_date input date in DateADT int representation, can be negative
 * @param interval interval to calculate, can be negative
 * @param calendar pointer to calendar to use as database
 * @param first_date_idx pointer to the first date index (NULLABLE)
 * @param result_date_idx pointer to the result date index (NULLABLE)
 * @return RESULT DATE = SUCCESS, INT32_MAX = Out of bounds
 */
int add_calendar_days (
	const IMCX *imcx,
	ulong calendar_index,
	int input_date,
	int interval,
	// Optional, set to NULL if it will not be used.
	int *first_date_idx,
	int *result_date_idx
)
{
  Calendar calendar = imcx->calendars[calendar_index];
  // Find the interval -> closest date index from the left of the input_date in the calendar
  int first_date_index = get_closest_index_from_left (input_date, calendar);
  // Now try to get the corresponding date of requested interval
  int result_date_index = first_date_index + interval;
  //
  // Now check if inside boundaries.
  if (result_date_index >= 0)
	{
	  if (first_date_index < 0)
		{
		  return calendar.dates[0]; // Returns the first date of the calendar.
		}
	  if (result_date_index >= calendar.dates_size)
		{
		  return INT32_MAX; // Returns infinity+.
		}
	}
  else
	{
	  if (result_date_index < 0 && result_date_index != 0)
		{
		  return calendar.dates[0]; // Returns the first date of the calendar.
		}
	  if (first_date_index >= calendar.dates_size)
		{
		  return INT32_MAX; // Returns infinity+.
		}
	}
  // This can be useful for reporting or debugging.
  if (first_date_idx != NULL)
	{
	  *first_date_idx = first_date_index;
	}
  if (result_date_idx != NULL)
	{
	  *result_date_idx = result_date_index;
	}
  //
  return calendar.dates[result_date_index];
}

/**
 * Clears the Cache
 * @return -1 if error, 0 if OK.
 */
int cache_invalidate (IMCX *imcx)
{
  if (imcx->calendar_count == 0)
	{
	  return -1;
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
  imcx->calendar_count = 0;
  imcx->entry_count = 0;
  imcx->cache_filled = false;
  // Done
  return 0;
}