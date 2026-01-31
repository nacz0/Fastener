#pragma once

#include "fastener/core/types.h"
#include "fastener/ui/style.h"
#include <string>
#include <string_view>

/**
 * @file date_picker.h
 * @brief DatePicker widget and date utilities.
 */

namespace fst {

class Context;

//=============================================================================
// Date Types
//=============================================================================

struct Date {
    int year = 1970;
    int month = 1;  // 1-12
    int day = 1;    // 1-31

    constexpr bool operator==(const Date& other) const {
        return year == other.year && month == other.month && day == other.day;
    }
    constexpr bool operator!=(const Date& other) const {
        return !(*this == other);
    }
};

enum class DateFormat : uint8_t {
    ISO, // YYYY-MM-DD
    MDY, // MM/DD/YYYY
    DMY  // DD/MM/YYYY
};

//=============================================================================
// Date Utilities
//=============================================================================
namespace date_utils {
    bool isLeapYear(int year);
    int daysInMonth(int year, int month);
    int dayOfWeek(int year, int month, int day); // 0=Sunday

    bool isValidDate(const Date& date);
    Date clampDate(const Date& date);

    int compareDate(const Date& a, const Date& b);
    bool isBefore(const Date& a, const Date& b);
    bool isAfter(const Date& a, const Date& b);
    bool isWithinRange(const Date& date, const Date& minDate, const Date& maxDate);

    Date addMonths(const Date& date, int months);
    std::string formatDate(const Date& date, DateFormat format);
} // namespace date_utils

//=============================================================================
// DatePicker
//=============================================================================

struct DatePickerOptions {
    Style style;
    bool disabled = false;
    bool showOutsideDays = true;
    bool showWeekdayHeader = true;
    bool showTodayButton = true;
    bool closeOnSelect = true;
    int firstDayOfWeek = 0; // 0=Sunday
    DateFormat format = DateFormat::ISO;
    Date minDate{0, 0, 0};
    Date maxDate{0, 0, 0};
    float cellSize = 0.0f;
    float popupWidth = 0.0f;
};

[[nodiscard]] bool DatePicker(Context& ctx, std::string_view label, Date& date, const DatePickerOptions& options = {});

} // namespace fst
