/**
 * (C) KetteQ, Inc.
 */


#include "imcx/src/include/math.h"

int calculate_page_size (long first_date, long last_date, ulong entry_count)
{
  long date_range = last_date - first_date;
  double avg_entries_per_week_calendar = date_range / 7.0;
  double entry_count_d = (double)entry_count;
  //
  int page_size_tmp = 32; // monthly calendar
  //
  if (entry_count_d > avg_entries_per_week_calendar)
    {
      page_size_tmp = 16; // weekly calendar
    }
  //
  return page_size_tmp;
}

long get_closest_index_from_left (long date_adt, Calendar calendar)
{
  long page_map_index = (date_adt / calendar.page_size) - calendar.first_page_offset;
  //
  if (page_map_index >= calendar.page_map_size)
    {
      long ret_val = -1 * ((long)calendar.dates_size) - 1;
      return ret_val;
    }
  else if (page_map_index < 0)
    {
      int ret_val = -1;
      return ret_val;
    }
  //
  long inclusive_start_index = calendar.page_map[page_map_index];
  long exclusive_end_index =
      page_map_index < calendar.page_map_size - 1 ?
      calendar.page_map[page_map_index + 1] :
      calendar.dates_size;
  //
  return left_binary_search (calendar.dates,
                             inclusive_start_index,
                             exclusive_end_index,
                             date_adt);
}

long binary_search (const long arr[], long left, long right, long value)
{
  if (right == left)
    return right;
  if (right > left)
    {
      long mid = left + (right - left) / 2;
      // If the element is present at the middle
      // itself
      if (arr[mid] == value)
        return mid;
      // If element is smaller than mid, then
      // it can only be present in left subarray
      if (arr[mid] > value)
        return binary_search (arr, left, mid - 1, value);
      // Else the element can only be present
      // in right subarray
      return binary_search (arr, mid + 1, right, value);
    }
  // Return -1 if not found.
  return -1;
}

long left_binary_search (const int *arr, long left, long right, long value)
{
  while (left <= right)
    {
      long mid = left + (right - left) / 2;
      if (arr[mid] < value)
        left = mid + 1;
      else if (arr[mid] > value)
        {
          right = mid - 1;
        }
      else
        return mid;
    }
  return left - 1;
}