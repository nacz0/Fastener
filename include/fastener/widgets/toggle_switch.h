#pragma once

#include "fastener/core/types.h"
#include "fastener/ui/style.h"
#include <string_view>

/**
 * @file toggle_switch.h
 * @brief iOS-style toggle switch widget
 * 
 * @ai_hint Alternative to Checkbox with sliding animation.
 *          Toggles bool& reference in-place. Returns true ONLY on the frame
 *          when the value changes (not every frame it's on).
 *          The label appears to the right of the switch.
 * 
 * @example
 *   static bool darkMode = false;
 *   if (fst::ToggleSwitch(ctx, "Dark Mode", darkMode)) {
 *       // This runs ONLY when user clicks
 *       applyTheme(darkMode);
 *   }
 */

namespace fst {

class Context;

//=============================================================================
// Toggle Switch
//=============================================================================
struct ToggleSwitchOptions {
    Style style;
    bool disabled = false;
};

/// Toggle switch with label - returns true when value changes
[[nodiscard]] bool ToggleSwitch(Context& ctx, std::string_view label, bool& value, const ToggleSwitchOptions& options = {});

} // namespace fst
