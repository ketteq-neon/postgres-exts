/**
 * IMCX Unit Tests
 * (C) KetteQ
 *
 * Main tests
 */

// OPTIONS
#define CALENDAR_COUNT 10
#define ENTRY_COUNT_MIN 1000
#define ENTRY_COUNT_MAX 10000
#define MOCK_INTERVAL_MIN 0
#define MOCK_INTERVAL_MAX 4
#define ADD_DAYS_REPETITIONS 2
//

#include <gtest/gtest.h>
#include <random>
#include <ctime>
#include <cstdlib>

extern "C" {
#include "../src/calendar/cache.h"
#include "../src/common/util.h"
}

class IMCXTestClass: public ::testing::Test {
 protected:
  IMCX *imcx{};
  void SetUp () override
  {
	imcx = (IMCX *)calloc (1, sizeof (IMCX));
	int ret = cache_init (imcx, 1, CALENDAR_COUNT);
	EXPECT_EQ(ret, 0);
	AddEntries ();
  }
  virtual void AddEntries ()
  {
	int ret, cc, jj;
	Calendar *cal;
	//
	for (cc = 0; cc < imcx->calendar_count; cc++)
	  {
		cal = &imcx->calendars[cc];
		cal->calendar_id = cc + 1;
		//
		ret = init_calendar (imcx, cc, 11);
		EXPECT_EQ(cal->dates_size, 11);
		EXPECT_EQ(ret, 0);
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
		EXPECT_EQ(ret, 0);
	  }
  }
};

// Static Approach
TEST_F(IMCXTestClass, CalendarCount)
{
  // Check the Size of Allocated Calendars
  EXPECT_EQ(imcx->calendar_count, CALENDAR_COUNT);
}
//
TEST_F(IMCXTestClass, CalendarEntryCount)
{
  long dates_size = imcx->calendars[0].dates_size;
  EXPECT_EQ(dates_size, 11);
}
//
//// rq. link with pgs lib.
////void date_to_str(int date, char * rp)
////{
////    struct pg_tm tt{},
////            *tm = &tt;
////    if (DATE_NOT_FINITE(date))
////        EncodeSpecialDate(date, rp);
////    else
////    {
////        j2date(date + POSTGRES_EPOCH_JDATE,
////               &(tm->tm_year), &(tm->tm_mon), &(tm->tm_mday));
////        EncodeDateOnly(tm, DateStyle, rp);
////    }
////}
//

/**
 * Generates a mock input date, can generate a date outside the last entry boundaries, must check in runtime.
 * @param first_date_entry
 * @return
 */
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
////
int gen_fake_interval ()
{
  // Init Random to get a date between calendar entries
  std::random_device rd;
  std::mt19937 mt (rd ());
  std::uniform_int_distribution<int> dist (MOCK_INTERVAL_MIN, MOCK_INTERVAL_MAX);
  //
  return dist (mt);
}
////
int gen_date_from_interval (int start_date_adt, int add_interval, int *date_between)
{
  int cc;
  // Init Random to get a date between calendar entries
  std::random_device rd;
  std::mt19937 mt (rd ());
  std::uniform_int_distribution<int> dist (30, 31);
  //
  // Basic version, adds imprecisely 30-31 days by interval
  int calendar_curr_entry = start_date_adt;
  if (add_interval > 0)
	{
	  for (cc = 0; cc < add_interval; cc++)
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
////
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
	  // printf("Date Interval Generated: %d, Date Between Interval: %d\n", interval_date, date_btw);
	  EXPECT_EQ(start_date + (cc * 30), interval_date);
	}
}
//// Static Test
TEST_F(IMCXTestClass, StaticCalendarAddDaysTest)
{
  int cc, jj;
  // Calendar * cal;
  //
  for (cc = 0; cc < imcx->calendar_count; cc++)
	{
	  // cal = &imcx->calendars[cc];
	  // interval = 0
	  int test_dates[] = {
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
	  int test_intervals[] = {
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
	  int expected_results[] = {
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
	  //
	  for (jj = 0; jj < 13; jj++)
		{
		  Calendar cal = imcx->calendars[cc];
		  int first_date_index, result_date_index;
		  int add_days_result = add_calendar_days (imcx,
												   cc,
												   test_dates[jj],
												   test_intervals[jj],
												   &first_date_index,
												   &result_date_index);
		  printf ("Cal-Id: %lu, Input: %d, Interval: %d, Expected: %d\n"
				  "Add-Days-Result: %d, FirstDate-Idx: %d = %ld, Result-Idx: %d = %ld\n",
				  cal.calendar_id,
				  test_dates[jj],
				  test_intervals[jj],
				  expected_results[jj],
				  add_days_result,
				  first_date_index,
				  cal.dates[first_date_index],
				  result_date_index,
				  cal.dates[result_date_index]);
		  EXPECT_EQ(add_days_result, expected_results[jj]);
		  printf ("---\n");
		}
	}
}
////
TEST_F(IMCXTestClass, StaticCalendarInvalidate)
{
  // Invalidate the static test in order to start the dynamic ones.
  int ret = cache_invalidate (imcx);
  EXPECT_EQ(ret, 0);
}
////
////// Init the Entries
TEST_F(IMCXTestClass, CalendarCacheEntriesInit)
{
  //
  if (CALENDAR_COUNT == 0)
	{
	  GTEST_SKIP();
	}
  SetUp ();
  //
  printf ("Initializing entries between %d and %d for %lu calendars.\n",
		  ENTRY_COUNT_MIN, ENTRY_COUNT_MAX,
		  imcx->calendar_count);
  // C++11 Random
  std::random_device rd;
  std::mt19937 mt (rd ());
  std::uniform_int_distribution<int> dist (ENTRY_COUNT_MIN, ENTRY_COUNT_MAX);
  //
  int ret,
	  cc;
  Calendar *cal;
  // Init the Calendars
  for (cc = 0; cc < imcx->calendar_count; cc++)
	{
	  // Get Random Entries Number Between the boundaries
	  int per_calendar_entry_count = dist (mt);
	  // Init The Entries
	  cal = &imcx->calendars[cc];
	  cal->calendar_id = cc + 1;
	  //
	  ret = init_calendar (imcx, cc, per_calendar_entry_count);
	  EXPECT_EQ(ret, 0);
	  EXPECT_EQ(cal->dates_size, per_calendar_entry_count);
	  // Init the Calendar Name
	  int num_len = snprintf (nullptr, 0, "Calendar Number %d", cc);
	  char *cal_name = (char *)malloc ((num_len + 1) * sizeof (char));
	  snprintf (cal_name, num_len + 1, "Calendar Number %d", cc);
	  //
	  add_calendar_name (imcx, cc, cal_name);
	  // Check if we can get the calendar by name.
	  long calendar_index = get_calendar_index_by_name (imcx, cal_name);
	  Calendar g_cal = imcx->calendars[calendar_index];
	  // If found, ret == 0
	  EXPECT_EQ(ret, 0);
	  // Now check if the calendar is the same (until now)
	  EXPECT_EQ(cal->calendar_id, g_cal.calendar_id);
	  EXPECT_EQ(cal->dates_size, g_cal.dates_size);
	  //
	  free (cal_name);
	  // total_entry_count += per_calendar_entry_count;
	}
  printf ("\n");
  EXPECT_EQ(cc, imcx->calendar_count);
}
////
///// This will fill the in-mem cache
TEST_F(IMCXTestClass, CalendarEntriesInsert)
{
  //
  if (CALENDAR_COUNT == 0)
	{
	  GTEST_SKIP();
	}
  //
  int ret,
	  cc,
	  jj,
	  c_entry_count;
  //
  c_entry_count = 0;
  //
  printf ("Will fill entries for %lu calendars (%lu total entries).\n",
		  imcx->calendar_count, imcx->entry_count);
  //
  for (cc = 0; cc < imcx->calendar_count; cc++)
	{
	  Calendar *cal = &imcx->calendars[cc];
	  // printf("Will fill %d entries.\n", cal->dates_size);
	  for (jj = 0; jj < cal->dates_size; jj++)
		{
		  int entry = gen_date_from_interval (7305, jj, nullptr);
		  cal->dates[jj] = entry;
		  printf ("dates[%d]=%ld|",
				  jj,
				  cal->dates[jj]);
		  EXPECT_EQ(cal->dates[jj], entry);
		  // Progress, // TODO: Improve
//            if (c_entry_count % progress_mod == 0) {
//                int pr = (int)((double)c_entry_count / (double)total_entry_count * 100.0);
//                printf("\rFilling Entries: %d%%; %d/%d", pr, c_entry_count, total_entry_count);
//                fflush(stdout);
//            }
//            c_entry_count++;
		  //
		}
	  printf ("\n");
	  EXPECT_EQ(jj, cal->dates_size);
	  // Calculate Page Map
	  ret = init_page_size (cal);
	  EXPECT_EQ(ret, 0);
	}
  printf ("\n");
  EXPECT_EQ(cc, imcx->calendar_count);
}

void add_days_test (IMCX *imcx)
{
  //
  int cc,
	  jj,
	  c_entry_count;
  //
  c_entry_count = 0;
  // Generate fake input dates for each calendar
  for (cc = 0; cc < imcx->calendar_count; cc++)
	{
	  Calendar *cal = &imcx->calendars[cc];
//        printf("Calendar-Id: %d, Page-Size: %d, First-Entry: %d, Last-Entry: %d, Entries: %d\n",
//               cal->calendar_id,
//               cal->page_size,
//               cal->dates[0],
//               cal->dates[cal->dates_size-1],
//               cal->dates_size);
	  //
	  int interval_to_test = gen_fake_interval ();
	  int fake_input = gen_fake_input_date (cal->dates[0], cal->dates[cal->dates_size - 1]);

	  // EXPECT_EQ(cal->dates[0], cal->dates[cal->dates_size - 1]);
	  EXPECT_GE(fake_input, cal->dates[0]);
	  EXPECT_LT(cal->dates[0], cal->dates[cal->dates_size - 1]);
	  EXPECT_LT(fake_input, cal->dates[cal->dates_size - 1]);
	  //
	  int fd_idx = 0, rs_idx = 0;
	  int add_days_result = add_calendar_days (imcx,
											   cc,
											   fake_input,
											   interval_to_test,
											   &fd_idx,
											   &rs_idx);
	  //
	  //if (add_days_result == INT32_MAX || add_days_result == INT32_MIN) {
	  //    printf("Result Date in Future or Past (result=%d)\n", add_days_result);
	  //} else {

	  Calendar d = *cal;
	  printf ("Cal-Id: %lu, Fake-Input: %d, Interval: %d\nAdd-Days-Result: %d, FirstDate-Idx: %d = %ld, Result-Idx: %d = %ld\n",
			  cal->calendar_id,
			  fake_input,
			  interval_to_test,
			  add_days_result,
			  fd_idx,
			  cal->dates[fd_idx],
			  rs_idx,
			  cal->dates[rs_idx]);
	  //}
	  //
	  EXPECT_LE(add_days_result, INT32_MAX);
	  EXPECT_GE(add_days_result, INT32_MIN);
	  //
	  if (interval_to_test <= 0)
		{
		  EXPECT_LE(add_days_result, fake_input);
		}
	  else
		{
		  EXPECT_GT(add_days_result, fake_input);
		}
	}
}

//// This will test the add_days_functions
TEST_F(IMCXTestClass, CalendarAddDaysTest)
{
  //
  if (CALENDAR_COUNT == 0)
	{
	  GTEST_SKIP();
	}
  for (int c = 0; c < ADD_DAYS_REPETITIONS; c++)
	{
	  add_days_test (imcx);
	}
}
////
//// This will Invalidate, Must return 0 if everything went OK.
TEST_F(IMCXTestClass, CalendarInvalidate)
{
  //
  if (CALENDAR_COUNT == 0)
	{
	  GTEST_SKIP();
	}
  //
  int ret = cache_invalidate (imcx);
  EXPECT_EQ(ret, 0);
}
//
TEST_F(IMCXTestClass, IntToStringConversion)
{
  int myNumber = 1005;
  char *myNumberStr = convert_int_to_str (myNumber);
  EXPECT_STREQ(myNumberStr, "1005");
  int myNumber2 = 10010;
  char *myNumberStr2 = convert_int_to_str (myNumber2);
  EXPECT_STREQ(myNumberStr2, "10010");
  // -- Free
  free (myNumberStr);
  free (myNumberStr2);
}
//
TEST_F(IMCXTestClass, LongToStringConversion)
{
  long myNumber = 2005;
  char *myNumberStr = convert_long_to_str (myNumber);
  EXPECT_STREQ(myNumberStr, "2005");
  long myNumber2 = 31010;
  char *myNumberStr2 = convert_long_to_str (myNumber2);
  EXPECT_STREQ(myNumberStr2, "31010");
  // -- Free
  free (myNumberStr);
  free (myNumberStr2);
}
//
TEST_F(IMCXTestClass, UIntToStringConversion)
{
  uint myNumber = 1015;
  char *myNumberStr = convert_u_int_to_str (myNumber);
  EXPECT_STREQ(myNumberStr, "1015");
  uint myNumber2 = 10012;
  char *myNumberStr2 = convert_u_int_to_str (myNumber2);
  EXPECT_STREQ(myNumberStr2, "10012");
  // -- Free
  free (myNumberStr);
  free (myNumberStr2);
}