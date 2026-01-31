#pragma once

#include "fastener/core/types.h"
#include "fastener/ui/style.h"
#include <string>
#include <string_view>

/**
 * @file time_picker.h
 * @brief TimePicker widget and time utilities.
 */

namespace fst {

class Context;

//=============================================================================
// Time Types
//=============================================================================

struct TimeOfDay {
    int hour = 0;   // 0-23
    int minute = 0; // 0-59
    int second = 0; // 0-59

    constexpr bool operator==(const TimeOfDay& other) const {
        return hour == other.hour && minute == other.minute && second == other.second;
    }
    constexpr bool operator!=(const TimeOfDay& other) const {
        return !(*this == other);
    }
};

enum class TimeFormat : uint8_t {
    H24,
    H12
};

//=============================================================================
// Time Utilities
//=============================================================================
namespace time_utils {
    bool isValidTime(const TimeOfDay& time);
    TimeOfDay clampTime(const TimeOfDay& time);
    TimeOfDay addHours(const TimeOfDay& time, int hours);
    TimeOfDay addMinutes(const TimeOfDay& time, int minutes);
    TimeOfDay addSeconds(const TimeOfDay& time, int seconds);
    std::string formatTime(const TimeOfDay& time, TimeFormat format, bool showSeconds);
} // namespace time_utils

//=============================================================================
// TimePicker
//=============================================================================

struct TimePickerOptions {
    Style style;
    bool disabled = false;
    bool showSeconds = false;
    bool use24Hour = true;
    bool showNowButton = true;
    int hourStep = 1;
    int minuteStep = 1;
    int secondStep = 1;
    float popupWidth = 0.0f;
};

[[nodiscard]] bool TimePicker(Context& ctx, std::string_view label, TimeOfDay& time, const TimePickerOptions& options = {});

} // namespace fst
