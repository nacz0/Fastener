#pragma once

#include "fastener/core/types.h"
#include "fastener/ui/style.h"
#include <string>

/**
 * @file checkbox.h
 * @brief Boolean toggle widget with label
 * 
 * @ai_hint Toggles bool& reference in-place. Returns true ONLY on the frame
 *          when the value changes (not every frame it's checked).
 *          The label appears to the right of the checkbox.
 * 
 * @example
 *   static bool enableSound = true;
 *   if (fst::Checkbox("Enable Sound", enableSound)) {
 *       // This runs ONLY when user clicks, not every frame
 *       applyAudioSettings(enableSound);
 *   }
 */

namespace fst {

//=============================================================================
// Checkbox
//=============================================================================
struct CheckboxOptions {
    Style style;
    bool disabled = false;
};

bool Checkbox(const std::string& label, bool& checked, const CheckboxOptions& options = {});
bool Checkbox(const char* label, bool& checked, const CheckboxOptions& options = {});

} // namespace fst
