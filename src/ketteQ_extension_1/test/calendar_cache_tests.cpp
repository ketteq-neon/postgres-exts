/**
 * (C) ketteQ
 * Author: Giancarlo Chiappe
 *
 * Main tests
 */

#include <gtest/gtest.h>
#include <random>
#include <ctime>

// Params
// TODO: Get as test app args...
int calendar_entry_count = 100;
int per_calendar_entry_count_min = 100;
int per_calendar_entry_count_max = 200;
int progress_mod = 10;
//

extern "C" {
    #include "../src/calendar.h"
    // This is rq. bc. we need to play with DateADT...
    // TODO: Link with Postgres server library, check CMakeLists
    //    #include "postgres.h"
    //    #include <utils/date.h>
    //    #include <utils/datetime.h>
    //    #include <miscadmin.h>
}

// TODO: Validate, rq. link with pgs lib.
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
int gen_fake_input_date(int first_date_entry) {
    // Init Random to get a date between calendar entries
    std::random_device rd;
    std::mt19937 mt(rd());
    std::uniform_int_distribution<int> dist(first_date_entry, first_date_entry + 30 - 1);
    //
    return dist(mt);
}

// TODO: Improve for precise monthly calendars. Recommended, pure C or C++, bc. cannot link with pgs right now.
int gen_date_from_interval(int start_date_adt, int add_interval, int * date_between) {
    // Basic version, adds imprecisely 30 days as a month...
    int calendar_first_entry = start_date_adt + (add_interval * 30);
    if (date_between != nullptr)
        *date_between = gen_fake_input_date(calendar_first_entry);
    return start_date_adt + (add_interval * 30);
}

// TODO: Improve with PG DateADT & JulianDate functions
TEST(kQCalendarMathTest, IntervalGeneratorTest) {
    int intervals = 0; // Set 0 to disable this test.
    int start_date = 0; // 01-01-2000
    //
    int cc;
    //
    if (intervals > 0) {
        printf("Testing DateADT generation capability.");
    }
    for (cc = 0; cc < intervals; cc++) {
        int interval_date = gen_date_from_interval(start_date, cc, nullptr);
        EXPECT_EQ(start_date + (cc * 30), interval_date);
    }
    //
    for (cc = 0; cc < intervals; cc++) {
        int date_btw;
        int interval_date = gen_date_from_interval(start_date, cc, &date_btw);
        printf("Date Interval Generated: %d, Date Between Interval: %d\n", interval_date, date_btw);
        EXPECT_EQ(start_date + (cc * 30), interval_date);
    }
}

int total_entry_count = 0;

// Init the Calendars
TEST(kQCalendarMathTest, CalendarCacheInit) {
    // int calendar_entry_count = 256;
    // Init the In-Mem Store
    printf("Initializing cache for %d calendars.\n", calendar_entry_count);
    int ret = calcache_init_calendars(1, calendar_entry_count);
    EXPECT_EQ(ret, 0);
    // Check the Size of Allocated Calendars
    EXPECT_EQ(calcache_calendar_count, calendar_entry_count);
}

//// Init the Entries
TEST(kQCalendarMathTest, CalendarCacheEntriesInit) {
    //
    printf("Initializing entries between %d and %d for %lu calendars.\n",
           per_calendar_entry_count_min, per_calendar_entry_count_max,
           calcache_calendar_count);
    // C++11 Random
    std::random_device rd;
    std::mt19937 mt(rd());
    std::uniform_int_distribution<int> dist(per_calendar_entry_count_min, per_calendar_entry_count_max);
    //
    int ret, cc;
    Calendar * cal;
    // Init the Calendars
    for (cc = 0; cc < calcache_calendar_count; cc++) {
        // Get Random Entries Number Between the boundaries
        int per_calendar_entry_count = dist(mt);
        // Init The Entries
        cal = &calcache_calendars[cc];
        cal->calendar_id = cc+1;
        //
        ret = calcache_init_calendar_entries( cal, per_calendar_entry_count);
        EXPECT_EQ(ret, 0);
        EXPECT_EQ(cal->dates_size, per_calendar_entry_count);
        // Init the Calendar Name
        int num_len = snprintf(nullptr, 0, "Calendar Number %d", cc);
        char * cal_name = (char *) malloc((num_len + 1) * sizeof(char));
        snprintf(cal_name, num_len+1, "Calendar Number %d", cc);
        //
        calcache_init_add_calendar_name(* cal, cal_name);
        // Check if we can get the calendar by name.
        Calendar g_cal;
        ret = calcache_get_calendar_by_name(cal_name, &g_cal);
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
    EXPECT_EQ(cc, calcache_calendar_count);
}
//
// This will fill the in-mem cache
TEST(kQCalendarMathTest, CalendarEntriesInsert) {
    int ret, cc, jj, c_entry_count;
    Calendar * cal;
    //
    c_entry_count = 0;
    //
    printf("Will fill entries for %lu calendars (%d total entries).\n",
           calcache_calendar_count, total_entry_count);
    //
    for (cc = 0; cc < calcache_calendar_count; cc++) {
        cal = &calcache_calendars[cc];
        // printf("Will fill %d entries.\n", cal->dates_size);
        for (jj = 0; jj < cal->dates_size; jj++) {
            int entry = gen_date_from_interval(0, jj, nullptr);
            cal->dates[jj] = entry;
            EXPECT_EQ(cal->dates[jj], entry);
            // Progress, // TODO: Improve
            if (c_entry_count % progress_mod == 0) {
                int pr = (int)((double)c_entry_count / (double)total_entry_count * 100.0);
                printf("\rFilling Entries: %d%%; %d/%d", pr, c_entry_count, total_entry_count);
                fflush(stdout);
            }
            c_entry_count++;
            //
        }
        EXPECT_EQ(jj, cal->dates_size);
    }
    printf("\n");
    EXPECT_EQ(cc, calcache_calendar_count);
}
//
TEST(kQCalendarMathTest, CalendarAddDaysTest) {

}
//
// This will Invalidate, Must return 0 if everything went OK.
TEST(kQCalendarMathTest, CalendarInvalidate) {
    int ret = calcache_invalidate();
    EXPECT_EQ(ret, 0);
}