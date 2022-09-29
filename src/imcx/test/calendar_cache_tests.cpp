/**
 * (C) ketteQ
 * Author: Giancarlo Chiappe
 *
 * Main tests
 */

#include <gtest/gtest.h>
#include <random>
#include <ctime>

// Dynamic Testing Params
int calendar_entry_count = 0; // 0 = disabled
int per_calendar_entry_count_min = 5;
int per_calendar_entry_count_max = 10;
int progress_mod = 10;
int fake_interval_min = 1; // Can be negative
int fake_interval_max = 2;
//

extern "C" {
    #include "../src/calendar.h"
    #include "../src/common/util.h"
}
// Static Approach
TEST(kQCalendarMathTest, StaticCalendarCacheInit) {
    // Init the In-Mem Store
    int ret = cacheInitCalendars(1, 1);
    EXPECT_EQ(ret, 0);
    // Check the Size of Allocated Calendars
    EXPECT_EQ(cacheCalendarCount, 1);
}
//
TEST(kQCalendarMathTest, StaticCalendarCacheEntriesInit) {
    int cc, ret, jj;
    InMemCalendar *cal;
    //
    for (cc = 0; cc < cacheCalendarCount; cc++) {
        cal = &cacheCalendars[cc];
        cal->calendar_id = cc + 1;
        //
        ret = cacheInitCalendarEntries(cal, 11);
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
        // Calculate Page Map
        ret = cacheInitPageSize(cal);
        EXPECT_EQ(ret, 0);
        //
//        printf("Cal-Id: %d, Page-Size: %d\nPage-Map-Size: %d",
//               cal->calendar_id, cal->page_size, cal->page_map_size);
        // Show Page Map
//        printf(", Page-Map: ");
//        for (jj = 0; jj < cal->page_map_size; jj++) {
//            printf("[%d]=%d,", jj, cal->page_map[jj]);
//        }
        // Show InMemCalendar
//        printf("\nDates: ");
//        for (jj = 0; jj < cal->dates_size; jj++) {
//            printf("[%d]=%d,", jj, cal->dates[jj]);
//        }
//        printf("\n");
    }
}

// rq. link with pgs lib.
//void date_to_str(int date, char * rp)
//{
//    struct pg_tm tt{},
//            *tm = &tt;
//    if (DATE_NOT_FINITE(date))
//        EncodeSpecialDate(date, rp);
//    else
//    {
//        j2date(date + POSTGRES_EPOCH_JDATE,
//               &(tm->tm_year), &(tm->tm_mon), &(tm->tm_mday));
//        EncodeDateOnly(tm, DateStyle, rp);
//    }
//}

/**
 * Generates a mock input date, can generate a date outside the last entry boundaries, must check in runtime.
 * @param first_date_entry
 * @return
 */
int gen_fake_input_date(int first_date_entry, int last_date_entry) {
    // Init Random to get a date between calendar entries
    std::random_device rd;
    std::mt19937 mt(rd());
    std::uniform_int_distribution<int> dist(first_date_entry, last_date_entry - 1);
    //
    return dist(mt);
}
//
int gen_fake_interval() {
    // Init Random to get a date between calendar entries
    std::random_device rd;
    std::mt19937 mt(rd());
    std::uniform_int_distribution<int> dist(fake_interval_min, fake_interval_max);
    //
    return dist(mt);
}
//
int gen_date_from_interval(int start_date_adt, int add_interval, int *date_between) {
    int cc;
    // Init Random to get a date between calendar entries
    std::random_device rd;
    std::mt19937 mt(rd());
    std::uniform_int_distribution<int> dist(30, 31);
    //
    // Basic version, adds imprecisely 30-31 days by interval
    int calendar_curr_entry = start_date_adt;
    if (add_interval > 0) {
        for (cc = 0; cc < add_interval; cc++) {
            calendar_curr_entry += dist(mt);
        }
    }
    if (date_between != nullptr) {
        *date_between = gen_fake_input_date(start_date_adt, calendar_curr_entry);
    }
    return start_date_adt + (add_interval * 30);
}
//
TEST(kQCalendarMathTest, IntervalGeneratorTest) {
    int intervals = 0; // Set 0 to disable this test.
    int start_date = 0; // 01-01-2000
    //
    int cc;
    //
    if (intervals > 0) {
        printf("Testing DateADT generation capability.");
    }
    //
    for (cc = 0; cc < intervals; cc++) {
        int interval_date = gen_date_from_interval(start_date, cc, nullptr);
        EXPECT_EQ(start_date + (cc * 30), interval_date);
    }
    //
    for (cc = 0; cc < intervals; cc++) {
        int date_btw;
        int interval_date = gen_date_from_interval(start_date, cc, &date_btw);
        // printf("Date Interval Generated: %d, Date Between Interval: %d\n", interval_date, date_btw);
        EXPECT_EQ(start_date + (cc * 30), interval_date);
    }
}
//// Static Test
TEST(kQCalendarMathTest, StaticCalendarAddDaysTest) {
    int cc, jj;
    InMemCalendar * cal;
    //
    for (cc = 0; cc < cacheCalendarCount; cc++) {
        cal = &cacheCalendars[cc];
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
                7305, // PAST (Gets First Date of InMemCalendar)
                INT32_MAX // FUTURE
        };
        //
        for (jj = 0; jj < 13; jj++) {
            int first_date_index, result_date_index;
            int add_days_result = cacheAddCalendarDays(test_dates[jj],
                                                       test_intervals[jj],
                                                       *cal,
                                                       &first_date_index,
                                                       &result_date_index);
            printf("Cal-Id: %d, Input: %d, Interval: %d, Expected: %d\n"
                   "Add-Days-Result: %d, FirstDate-Idx: %d = %d, Result-Idx: %d = %d\n",
                   cal->calendar_id,
                   test_dates[jj],
                   test_intervals[jj],
                   expected_results[jj],
                   add_days_result,
                   first_date_index, cal->dates[first_date_index],
                   result_date_index, cal->dates[result_date_index]);
            EXPECT_EQ(add_days_result, expected_results[jj]);
            printf("---\n");
        }
    }
}
//
TEST(kQCalendarMathTest, StaticCalendarInvalidate) {
    // Invalidate the static test in order to start the dynamic ones.
    int ret = cacheInvalidate();
    EXPECT_EQ(ret, 0);
}
//
int total_entry_count = 0;
//// Init the Calendars
TEST(kQCalendarMathTest, CalendarCacheInit) {
    //
    if (calendar_entry_count == 0) {
        GTEST_SKIP();
    }
    // Init the In-Mem Store
    printf("Initializing cache for %d calendars.\n", calendar_entry_count);
    int ret = cacheInitCalendars(1, calendar_entry_count);
    EXPECT_EQ(ret, 0);
    // Check the Size of Allocated Calendars
    EXPECT_EQ(cacheCalendarCount, calendar_entry_count);
}
//
//// Init the Entries
TEST(kQCalendarMathTest, CalendarCacheEntriesInit) {
    //
    if (calendar_entry_count == 0) {
        GTEST_SKIP();
    }
    //
    printf("Initializing entries between %d and %d for %lu calendars.\n",
           per_calendar_entry_count_min, per_calendar_entry_count_max,
           cacheCalendarCount);
    // C++11 Random
    std::random_device                  rd;
    std::mt19937                        mt(rd());
    std::uniform_int_distribution<int>  dist(per_calendar_entry_count_min, per_calendar_entry_count_max);
    //
    int         ret,
                cc;
    InMemCalendar *  cal;
    // Init the Calendars
    for (cc = 0; cc < cacheCalendarCount; cc++) {
        // Get Random Entries Number Between the boundaries
        int per_calendar_entry_count = dist(mt);
        // Init The Entries
        cal = &cacheCalendars[cc];
        cal->calendar_id = cc+1;
        //
        ret = cacheInitCalendarEntries(cal, per_calendar_entry_count);
        EXPECT_EQ(ret, 0);
        EXPECT_EQ(cal->dates_size, per_calendar_entry_count);
        // Init the InMemCalendar Name
        int num_len = snprintf(nullptr, 0, "Calendar Number %d", cc);
        char * cal_name = (char *) malloc((num_len + 1) * sizeof(char));
        snprintf(cal_name, num_len+1, "Calendar Number %d", cc);
        //
        cacheInitAddCalendarName(*cal, cal_name);
        // Check if we can get the calendar by name.
        InMemCalendar g_cal;
        ret = cacheGetCalendarByName(cal_name, &g_cal);
        // If found, ret == 0
        EXPECT_EQ(ret, 0);
        // Now check if the calendar is the same (until now)
        EXPECT_EQ(cal->calendar_id, g_cal.calendar_id);
        EXPECT_EQ(cal->dates_size, g_cal.dates_size);
        //
        free(cal_name);
        total_entry_count += per_calendar_entry_count;
    }
    printf("\n");
    EXPECT_EQ(cc, cacheCalendarCount);
}
//
/// This will fill the in-mem cache
TEST(kQCalendarMathTest, CalendarEntriesInsert) {
    //
    if (calendar_entry_count == 0) {
        GTEST_SKIP();
    }
    //
    int         ret,
                cc,
                jj,
                c_entry_count;
    InMemCalendar *  cal;
    //
    c_entry_count = 0;
    //
    printf("Will fill entries for %lu calendars (%d total entries).\n",
           cacheCalendarCount, total_entry_count);
    //
    for (cc = 0; cc < cacheCalendarCount; cc++) {
        cal = &cacheCalendars[cc];
        // printf("Will fill %d entries.\n", cal->dates_size);
        for (jj = 0; jj < cal->dates_size; jj++) {
            int entry = gen_date_from_interval(7305, jj, nullptr);
            cal->dates[jj] = entry;
            printf("dates[%d]=%d|",
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
        printf("\n");
        EXPECT_EQ(jj, cal->dates_size);
        // Calculate Page Map
        ret = cacheInitPageSize(cal);
        EXPECT_EQ(ret, 0);
    }
    printf("\n");
    EXPECT_EQ(cc, cacheCalendarCount);
}
//
//// This will test the add_days_functions
TEST(kQCalendarMathTest, CalendarAddDaysTest) {
    //
    if (calendar_entry_count == 0) {
        GTEST_SKIP();
    }
    //
    int cc,
            jj,
            c_entry_count;
    InMemCalendar *cal;
    //
    c_entry_count = 0;
    // Generate fake input dates for each calendar
    for (cc = 0; cc < cacheCalendarCount; cc++) {
        cal = &cacheCalendars[cc];
//        printf("InMemCalendar-Id: %d, Page-Size: %d, First-Entry: %d, Last-Entry: %d, Entries: %d\n",
//               cal->calendar_id,
//               cal->page_size,
//               cal->dates[0],
//               cal->dates[cal->dates_size-1],
//               cal->dates_size);
        //
        int interval_to_test = gen_fake_interval();
        int fake_input = gen_fake_input_date(cal->dates[0], cal->dates[cal->dates_size - 1]);
        EXPECT_GE(fake_input, cal->dates[0]);
        EXPECT_LT(fake_input, cal->dates[cal->dates_size - 1]);
        //
        int fd_idx, rs_idx;
        int add_days_result = cacheAddCalendarDays(fake_input,
                                                   interval_to_test,
                                                   *cal,
                                                   &fd_idx,
                                                   &rs_idx);
        //
        printf("Cal-Id: %d, Fake-Input: %d, Interval: %d\nAdd-Days-Result: %d, FirstDate-Idx: %d = %d, Result-Idx: %d = %d\n",
               cal->calendar_id,
               fake_input,
               interval_to_test,
               add_days_result,
               fd_idx, cal->dates[fd_idx],
               rs_idx, cal->dates[rs_idx]);
        //
        EXPECT_NE(add_days_result, INT32_MAX);
        EXPECT_NE(add_days_result, INT32_MIN);
        //
        if (interval_to_test <= 0) {
            EXPECT_LE(add_days_result, fake_input);
        } else {
            EXPECT_GT(add_days_result, fake_input);
        }
    }
}
//
// This will Invalidate, Must return 0 if everything went OK.
TEST(kQCalendarMathTest, CalendarInvalidate) {
    //
    if (calendar_entry_count == 0) {
        GTEST_SKIP();
    }
    //
    int ret = cacheInvalidate();
    EXPECT_EQ(ret, 0);
}

TEST(kQCalendarConvertionTest, IntToStringConversion) {
    int myNumber = 1005;
    char * myNumberStr = convertIntToStr(myNumber);
    EXPECT_STREQ(myNumberStr, "1005");
    int myNumber2 = 10010;
    char * myNumberStr2 = convertIntToStr(myNumber2);
    EXPECT_STREQ(myNumberStr2, "10010");
    // -- Free
    free(myNumberStr);
    free(myNumberStr2);
}

TEST(kQCalendarConvertionTest, UIntToStringConversion) {
    uint myNumber = 1015;
    char * myNumberStr = convertUIntToStr(myNumber);
    EXPECT_STREQ(myNumberStr, "1015");
    uint myNumber2 = 10012;
    char * myNumberStr2 = convertUIntToStr(myNumber2);
    EXPECT_STREQ(myNumberStr2, "10012");
    // -- Free
    free(myNumberStr);
    free(myNumberStr2);
}