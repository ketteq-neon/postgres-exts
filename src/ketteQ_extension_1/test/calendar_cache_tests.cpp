#include <gtest/gtest.h>

extern "C" {
    #include "../src/calendar.h"
}

TEST(kQCalendarMathTest, CalendarCacheInit) {
    int calendar_entry_count = 100;
    // Init the In-Mem Store
    printf("Initializing cache for %d calendars.\n", calendar_entry_count);
    int ret = calcache_init_calendars(1, calendar_entry_count);
    EXPECT_EQ(ret, 0);
    // Check the Size of Allocated Calendars
    EXPECT_EQ(calcache_calendar_count, calendar_entry_count);
}

TEST(kQCalendarMathTest, CalendarCacheEntriesInit) {
    int per_calendar_entry_count = 10;
    //
    printf("Initializing %d entries for %lu calendars.\n",
           per_calendar_entry_count, calcache_calendar_count);
    int ret, cc;
    Calendar * cal;
    // Init the Calendars
    printf("ICE=init_calendar_entries, IACN=init_add_calendar_name, GCBN=get_calendar_by_name\n");
    for (cc = 0; cc < calcache_calendar_count; cc++) {
        // Init The Entries
        cal = &calcache_calendars[cc];
        cal->calendar_id = cc+1;
        printf("%d,ICE,", cc);
        ret = calcache_init_calendar_entries( cal, per_calendar_entry_count);
        EXPECT_EQ(ret, 0);
        EXPECT_EQ(cal->dates_size, per_calendar_entry_count);
        // Init the Calendar Name
        int num_len = snprintf(nullptr, 0, "Calendar Number %d", cc);
        char * cal_name = (char *) malloc((num_len + 1) * sizeof(char));
        snprintf(cal_name, num_len+1, "Calendar Number %d", cc);
        //
        printf("IACN,");
        calcache_init_add_calendar_name(* cal, cal_name);
        // Check if we can get the calendar by name.
        Calendar g_cal;
        printf("GCBN;");
        ret = calcache_get_calendar_by_name(cal_name, &g_cal);
        // If found, ret == 0
        EXPECT_EQ(ret, 0);
        // Now check if the calendar is the same (until now)
        EXPECT_EQ(cal->calendar_id, g_cal.calendar_id);
        EXPECT_EQ(cal->dates_size, g_cal.dates_size);
    }
    printf("\n");
    EXPECT_EQ(cc, calcache_calendar_count);
}

TEST(kQCalendarMathTest, CalendarEntriesInsert) {
    int ret, cc, jj;
    Calendar * cal;
    //
    printf("Will fill entries for %lu calendars.\n", calcache_calendar_count);
    //
    for (cc = 0; cc < calcache_calendar_count; cc++) {
        cal = &calcache_calendars[cc];
        // printf("Will fill %d entries.\n", cal->dates_size);
        for (jj = 0; jj < cal->dates_size; jj++) {
            // printf("%d,", jj);
        }
        EXPECT_EQ(jj, cal->dates_size);
    }
    EXPECT_EQ(cc, calcache_calendar_count);
}

TEST(kQCalendarMathTest, CalendarInvalidate) {
    int ret = calcache_invalidate();
    EXPECT_EQ(ret, 0);
}