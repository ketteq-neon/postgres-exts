/**
 * IMCX Unit Tests
 * (C) KetteQ
 *
 * Main tests
 */

// Includes
#include <gtest/gtest.h>
#include <random>
#include <ctime>
#include <cstdlib>
extern "C" {

	#include "../src/include/cache.h"
}

// OPTIONS
constexpr int CALENDAR_COUNT = 10;
constexpr std::array<const char *, 10> CALENDAR_NAMES = {
	"a",
	"b",
	"c",
	"d",
	"e",
	"f",
	"g",
	"h",
	"i",
	"j"
};
constexpr int ENTRY_COUNT_MIN = 1000;
constexpr int ENTRY_COUNT_MAX = 10000;
constexpr int MOCK_INTERVAL_MIN = 0;
constexpr int MOCK_INTERVAL_MAX = 4;
constexpr int ADD_DAYS_REPETITIONS = 2;
//

class IMCXTestClass: public ::testing::Test {
 public:
  IMCX *p_get_imcx ()
  {
	IMCX *pp = imcx.get ();
	return pp;
  }
 private:
  std::unique_ptr<IMCX> imcx = std::make_unique<IMCX> ();
 protected:
  void SetUp () override
  {
	cache_invalidate (imcx.get ());
	int ret = cache_init (imcx.get (), 1, CALENDAR_COUNT);
	EXPECT_EQ(ret, RET_SUCCESS);
	AddEntries ();
	cache_finish (imcx.get ());
  }
  virtual void AddEntries ()
  {
	int ret;
	Calendar *cal;
	//
	for (int cc = 0; cc < imcx->calendar_count; cc++)
	  {
		cal = &imcx->calendars[cc];
		cal->id = cc + 1;
		set_calendar_name (imcx.get (), cc, CALENDAR_NAMES[cc]);
		//
		ret = calendar_init (imcx.get (), cc + 1, 11);
		EXPECT_EQ(cal->dates_size, 11);
		EXPECT_EQ(ret, RET_SUCCESS);
		// Add The Entries
		cal->dates[0] = 7305;
		cal->dates[1] = 7336;
		cal->dates[2] = 7365;
		cal->dates[3] = 7396;
		cal->dates[4] = 7426;
		cal->dates[5] = 7457;
		cal->dates[6] = 7487;
		cal->dates[7] = 7518;
		cal->dates[8] = 7549;
		cal->dates[9] = 7579;
		cal->dates[10] = 7610;
		// Init Page Map
		ret = init_page_size (cal);
		EXPECT_EQ(ret, RET_SUCCESS);
	  }
  }
};

TEST_F(IMCXTestClass, CalendarCount)
{
  // Check the Size of Allocated Calendars
  EXPECT_EQ(p_get_imcx ()->calendar_count, CALENDAR_COUNT);
}

TEST_F(IMCXTestClass, CalendarEntryCount)
{
  long dates_size = p_get_imcx ()->calendars[0].dates_size;
  EXPECT_EQ(dates_size, 11);
}

int gen_fake_input_date (int first_date_entry, int last_date_entry)
{
  // Init Random to get a date between calendar entries
  std::random_device rd;
  std::mt19937 mt (rd ());
  int last_date = last_date_entry == first_date_entry ? last_date_entry : last_date_entry - 1;
  std::uniform_int_distribution<int> dist (first_date_entry, last_date);
  //
  return dist (mt);
}

int gen_fake_interval ()
{
  // Init Random to get a date between calendar entries
  std::random_device rd;
  std::mt19937 mt (rd ());
  std::uniform_int_distribution<int> dist (MOCK_INTERVAL_MIN, MOCK_INTERVAL_MAX);
  //
  return dist (mt);
}

int gen_date_from_interval (int start_date_adt, int add_interval, int *date_between)
{
  // Init Random to get a date between calendar entries
  std::random_device rd;
  std::mt19937 mt (rd ());
  std::uniform_int_distribution<int> dist (30, 31);
  //
  // Basic version, adds imprecisely 30-31 days by interval
  int calendar_curr_entry = start_date_adt;
  if (add_interval > 0)
	{
	  for (int cc = 0; cc < add_interval; cc++)
		{
		  calendar_curr_entry += dist (mt);
		}
	}
  if (date_between != nullptr)
	{
	  *date_between = gen_fake_input_date (start_date_adt, calendar_curr_entry);
	}
  return start_date_adt + (add_interval * 30);
}

TEST_F(IMCXTestClass, IntervalGeneratorTest)
{
  // --
  int intervals = 10; // Set 0 to disable this test.
  int start_date = 0; // 01-01-2000
  //
  int cc;
  //
  for (cc = 0; cc < intervals; cc++)
	{
	  int interval_date = gen_date_from_interval (start_date, cc, nullptr);
	  EXPECT_EQ(start_date + (cc * 30), interval_date);
	}
  //
  for (cc = 0; cc < intervals; cc++)
	{
	  int date_btw;
	  int interval_date = gen_date_from_interval (start_date, cc, &date_btw);
	  EXPECT_EQ(start_date + (cc * 30), interval_date);
	}
}

TEST_F(IMCXTestClass, StaticCalendarAddDaysTest)
{
  printf("AddDaysTest\n");
  for (int cc = 0; cc < p_get_imcx ()->calendar_count; cc++)
	{
	  const Calendar *calendar = &p_get_imcx ()->calendars[cc];
	  std::vector<int> test_dates = {
		  // Valid Dates
		  7306,
		  7340,
		  7380,
		  7400,
		  7450,
		  7470,
		  7500,
		  7540,
		  7560,
		  7600,
		  // Invalid Dates
		  7610,
		  7300,
		  7561
	  };
	  std::vector<int> test_intervals = {
		  // Valid Intervals
		  0,
		  -1,
		  1,
		  2,
		  1,
		  -1,
		  -2,
		  -5,
		  1,
		  0,
		  //
		  1,
		  0,
		  3
	  };
	  std::vector<int> expected_results = {
		  // Valid Results
		  7305,
		  7305,
		  7396,
		  7457,
		  7457,
		  7426,
		  7426,
		  7365,
		  7579,
		  7579,
		  //
		  INT32_MAX, // FUTURE
		  7305, // PAST (Gets First Date of Calendar)
		  INT32_MAX // FUTURE
	  };
	  // Test Dates
	  for (int jj = 0; jj < 13; jj++)
		{
		  unsigned long first_date_index = 0;
		  unsigned long result_date_index = 0;
		  int32 result_date_adt = 0;
		  int add_days_result = add_calendar_days (p_get_imcx (),
												   cc,
												   test_dates[jj],
												   test_intervals[jj],
												   &result_date_adt,
												   &first_date_index,
												   &result_date_index);
		  printf ("Cal-Id: %lu, Input: %d, Interval: %d, Expected: %d\n"
				  "Add-Days-Result: %d, FirstDate-Idx: %lu = %d, Result-Idx: %lu = %d\n",
				  calendar->id,
				  test_dates[jj],
				  test_intervals[jj],
				  expected_results[jj],
				  add_days_result,
				  first_date_index,
				  calendar->dates[first_date_index],
				  result_date_index,
				  calendar->dates[result_date_index]);
		  EXPECT_EQ(result_date_adt, expected_results[jj]);
		  printf ("---\n");
		}
	}
}

TEST_F(IMCXTestClass, StaticCalendarInvalidate)
{
  // Invalidate the static test in order to start the dynamic ones.
  int ret = cache_invalidate (p_get_imcx ());
  EXPECT_EQ(ret, 0);
}

TEST_F(IMCXTestClass, CalendarCacheEntriesInit)
{
  int invalidate_result = cache_invalidate (p_get_imcx ());
  EXPECT_EQ(invalidate_result, RET_SUCCESS);
  //
  printf ("Initializing entries between %d and %d for %lu calendars.\n",
		  ENTRY_COUNT_MIN, ENTRY_COUNT_MAX,
		  p_get_imcx ()->calendar_count);
  // C++11 Random
  std::random_device rd;
  std::mt19937 mt (rd ());
  std::uniform_int_distribution<int> dist (ENTRY_COUNT_MIN, ENTRY_COUNT_MAX);
  //
  int ret;
  int cc;
  Calendar *cal;
  // Init the Calendars
  for (cc = 0; cc < p_get_imcx ()->calendar_count; cc++)
	{
	  // Get Random Entries Number Between the boundaries
	  int per_calendar_entry_count = dist (mt);
	  // Init The Entries
	  cal = &p_get_imcx ()->calendars[cc];
	  cal->id = cc + 1;
	  //
	  ret = calendar_init (p_get_imcx (), cc, per_calendar_entry_count);
	  EXPECT_EQ(ret, RET_SUCCESS);
	  EXPECT_EQ(cal->dates_size, per_calendar_entry_count);
	  // Init the Calendar Name
	  const int num_len = snprintf (nullptr, 0, "Calendar Number %d", cc);
	  auto cal_name = std::vector<char> ();

	  snprintf (cal_name.data (), num_len + 1, "Calendar Number %d", cc);
	  //
	  set_calendar_name (p_get_imcx (), cc, cal_name.data ());
	  // Check if we can get the calendar by name.
	  unsigned long calendar_index;
	  int calendar_index_result =
		  get_calendar_index_by_name (p_get_imcx (), cal_name.data (), &calendar_index);
	  if (calendar_index_result != RET_SUCCESS)
		{
		  if (calendar_index_result == RET_ERROR_NOT_FOUND)
			printf("Calendar does not exists.");
		  if (calendar_index_result == RET_ERROR_UNSUPPORTED_OP)
			printf("Cannot get calendar by name. (Out of Bounds)");
		  return;
		}
	  Calendar g_cal = p_get_imcx ()->calendars[calendar_index];
	  // If found, ret == 0
	  EXPECT_EQ(ret, RET_SUCCESS);
	  // Now check if the calendar is the same (until now)
	  EXPECT_EQ(cal->id, g_cal.id);
	  EXPECT_EQ(cal->dates_size, g_cal.dates_size);
	}
  printf ("\n");
  EXPECT_EQ(cc, p_get_imcx ()->calendar_count);
}
////
///// This will fill the in-mem cache
TEST_F(IMCXTestClass, CalendarEntriesInsert)
{
  int ret;
  int cc;
  int jj;
  //
  printf ("Will fill entries for %lu calendars (%lu total entries).\n",
		  p_get_imcx ()->calendar_count, p_get_imcx ()->entry_count);
  //
  for (cc = 0; cc < p_get_imcx ()->calendar_count; cc++)
	{
	  Calendar *cal = &p_get_imcx ()->calendars[cc];
	  for (jj = 0; jj < cal->dates_size; jj++)
		{
		  int entry = gen_date_from_interval (7305, jj, nullptr);
		  cal->dates[jj] = entry;
		  printf ("dates[%d]=%d|",
				  jj,
				  cal->dates[jj]);
		  EXPECT_EQ(cal->dates[jj], entry);
		}
	  printf ("\n");
	  EXPECT_EQ(jj, cal->dates_size);
	  // Calculate Page Map
	  ret = init_page_size (cal);
	  EXPECT_EQ(ret, 0);
	}
  printf ("\n");
  EXPECT_EQ(cc, p_get_imcx ()->calendar_count);
}

void add_days_test (const IMCX *imcx)
{
  // Generate fake input dates for each calendar
  for (int cc = 0; cc < imcx->calendar_count; cc++)
	{
	  Calendar const *cal = &imcx->calendars[cc];

	  int interval_to_test = gen_fake_interval ();
	  int fake_input = gen_fake_input_date (cal->dates[0], cal->dates[cal->dates_size - 1]);

	  EXPECT_GE(fake_input, cal->dates[0]);
	  EXPECT_LT(cal->dates[0], cal->dates[cal->dates_size - 1]);
	  EXPECT_LT(fake_input, cal->dates[cal->dates_size - 1]);

	  unsigned long fd_idx = 0;
	  unsigned long rs_idx = 0;
	  int32 new_date = 0;
	  int add_days_result = add_calendar_days (imcx,
											   cc,
											   fake_input,
											   interval_to_test,
											   &new_date,
											   &fd_idx,
											   &rs_idx);

	  EXPECT_NE(add_days_result, RET_ERROR_NOT_READY);
	  printf ("Cal-Id: %lu, Fake-Input: %d, Interval: %d\nNew-Date: %d, Add-Days-Result: %d, FirstDate-Idx: %lu = %d, Result-Idx: %lu = %d\n",
			  cal->id,
			  fake_input,
			  interval_to_test,
			  new_date,
			  add_days_result,
			  fd_idx,
			  cal->dates[fd_idx],
			  rs_idx,
			  cal->dates[rs_idx]);

	  EXPECT_LE(new_date, INT32_MAX);
	  EXPECT_GE(new_date, INT32_MIN);

	  if (interval_to_test <= 0)
		{
		  EXPECT_LE(new_date, fake_input);
		}
	  else
		{
		  EXPECT_GT(new_date, fake_input);
		}
	}
}

TEST_F(IMCXTestClass, CalendarAddDaysTest)
{
  for (int c = 0; c < ADD_DAYS_REPETITIONS; c++)
	{
	  add_days_test (p_get_imcx ());
	}
}

TEST_F(IMCXTestClass, CalendarInvalidate)
{
  int ret = cache_invalidate (p_get_imcx ());
  EXPECT_EQ(ret, RET_SUCCESS);
}

TEST_F(IMCXTestClass, IntToStringConversion)
{
  int my_number = 1005;
  char const *my_number_str = convert_int_to_str (my_number);
  EXPECT_STREQ(my_number_str, "1005");
  int my_number_2 = 10010;
  char const *my_number_str_2 = convert_int_to_str (my_number_2);
  EXPECT_STREQ(my_number_str_2, "10010");
}

TEST_F(IMCXTestClass, LongToStringConversion)
{
  long my_number = 2005;
  char const *my_number_str = convert_long_to_str (my_number);
  EXPECT_STREQ(my_number_str, "2005");
  long my_number_2 = 31010;
  char const *my_number_str_2 = convert_long_to_str (my_number_2);
  EXPECT_STREQ(my_number_str_2, "31010");
}

TEST_F(IMCXTestClass, UIntToStringConversion)
{
  uint my_number = 1015;
  char const *my_number_str = convert_u_int_to_str (my_number);
  EXPECT_STREQ(my_number_str, "1015");
  uint my_number_2 = 10012;
  char const *my_number_str_2 = convert_u_int_to_str (my_number_2);
  EXPECT_STREQ(my_number_str_2, "10012");
}

TEST_F(IMCXTestClass, CalendarNamesTest)
{
  for (int calendar_name_index = 0; calendar_name_index < CALENDAR_COUNT; calendar_name_index++)
	{
	  const char *requested_name = CALENDAR_NAMES[calendar_name_index];
	  unsigned long calendar_index = 0;
	  int get_calendar_result = get_calendar_index_by_name (p_get_imcx (), requested_name, &calendar_index);
	  EXPECT_EQ(RET_SUCCESS, get_calendar_result);
	  printf("Found Calendar With Name '%s' -> Index '%lu'\n", requested_name, calendar_index);
	  EXPECT_EQ(calendar_name_index + 1, p_get_imcx()->calendars[calendar_index].id);
	}
}