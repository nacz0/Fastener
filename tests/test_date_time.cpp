#include <gtest/gtest.h>
#include <fastener/widgets/date_picker.h>
#include <fastener/widgets/time_picker.h>

using namespace fst;

//=============================================================================
// Date Utilities Tests
//=============================================================================

TEST(DateUtilsTest, IsLeapYear) {
    EXPECT_TRUE(date_utils::isLeapYear(2024));
    EXPECT_FALSE(date_utils::isLeapYear(2023));
    EXPECT_TRUE(date_utils::isLeapYear(2000));
    EXPECT_FALSE(date_utils::isLeapYear(1900));
}

TEST(DateUtilsTest, DaysInMonth) {
    EXPECT_EQ(date_utils::daysInMonth(2023, 1), 31);
    EXPECT_EQ(date_utils::daysInMonth(2023, 2), 28);
    EXPECT_EQ(date_utils::daysInMonth(2024, 2), 29);
    EXPECT_EQ(date_utils::daysInMonth(2023, 4), 30);
}

TEST(DateUtilsTest, DayOfWeek) {
    // 1970-01-01 was a Thursday (0=Sunday, 4=Thursday)
    EXPECT_EQ(date_utils::dayOfWeek(1970, 1, 1), 4);
}

TEST(DateUtilsTest, ClampDate) {
    Date invalid{2023, 2, 31};
    Date clamped = date_utils::clampDate(invalid);
    EXPECT_EQ(clamped.year, 2023);
    EXPECT_EQ(clamped.month, 2);
    EXPECT_EQ(clamped.day, 28);
}

TEST(DateUtilsTest, AddMonths) {
    Date jan31{2024, 1, 31};
    Date feb = date_utils::addMonths(jan31, 1);
    EXPECT_EQ(feb.year, 2024);
    EXPECT_EQ(feb.month, 2);
    EXPECT_EQ(feb.day, 29); // Leap year clamp

    Date dec15{2023, 12, 15};
    Date next = date_utils::addMonths(dec15, 2);
    EXPECT_EQ(next.year, 2024);
    EXPECT_EQ(next.month, 2);
    EXPECT_EQ(next.day, 15);

    Date back = date_utils::addMonths(Date{2024, 1, 10}, -2);
    EXPECT_EQ(back.year, 2023);
    EXPECT_EQ(back.month, 11);
    EXPECT_EQ(back.day, 10);
}

TEST(DateUtilsTest, FormatDate) {
    Date date{2026, 1, 31};
    EXPECT_EQ(date_utils::formatDate(date, DateFormat::ISO), "2026-01-31");
    EXPECT_EQ(date_utils::formatDate(date, DateFormat::MDY), "01/31/2026");
    EXPECT_EQ(date_utils::formatDate(date, DateFormat::DMY), "31/01/2026");
}

//=============================================================================
// Time Utilities Tests
//=============================================================================

TEST(TimeUtilsTest, IsValidTime) {
    EXPECT_TRUE(time_utils::isValidTime({23, 59, 59}));
    EXPECT_FALSE(time_utils::isValidTime({24, 0, 0}));
    EXPECT_FALSE(time_utils::isValidTime({-1, 30, 0}));
}

TEST(TimeUtilsTest, ClampTime) {
    TimeOfDay invalid{-3, 70, 90};
    TimeOfDay clamped = time_utils::clampTime(invalid);
    EXPECT_EQ(clamped.hour, 0);
    EXPECT_EQ(clamped.minute, 59);
    EXPECT_EQ(clamped.second, 59);
}

TEST(TimeUtilsTest, AddHoursWrap) {
    TimeOfDay t{23, 0, 0};
    TimeOfDay next = time_utils::addHours(t, 2);
    EXPECT_EQ(next.hour, 1);
    EXPECT_EQ(next.minute, 0);
}

TEST(TimeUtilsTest, AddMinutesWrap) {
    TimeOfDay t{10, 59, 0};
    TimeOfDay next = time_utils::addMinutes(t, 2);
    EXPECT_EQ(next.hour, 11);
    EXPECT_EQ(next.minute, 1);
}

TEST(TimeUtilsTest, AddSecondsWrap) {
    TimeOfDay t{0, 0, 58};
    TimeOfDay next = time_utils::addSeconds(t, 5);
    EXPECT_EQ(next.hour, 0);
    EXPECT_EQ(next.minute, 1);
    EXPECT_EQ(next.second, 3);
}

TEST(TimeUtilsTest, FormatTime24) {
    TimeOfDay t{5, 7, 0};
    EXPECT_EQ(time_utils::formatTime(t, TimeFormat::H24, false), "05:07");
}

TEST(TimeUtilsTest, FormatTime12) {
    TimeOfDay t1{0, 5, 0};
    TimeOfDay t2{13, 9, 0};
    EXPECT_EQ(time_utils::formatTime(t1, TimeFormat::H12, false), "12:05 AM");
    EXPECT_EQ(time_utils::formatTime(t2, TimeFormat::H12, false), "01:09 PM");
}
