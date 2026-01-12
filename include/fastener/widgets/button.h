#pragma once

#include "fastener/core/types.h"
#include "fastener/ui/style.h"
#include <string>

/**
 * @file button.h
 * @brief Standard clickable button widget
 * 
 * @ai_hint Primary interactive widget. Returns true ONLY on the frame clicked.
 *          Use ButtonPrimary() for accent-colored actions (submit, save).
 *          For disabled state: ButtonOptions{.disabled = true}.
 *          Button uses bounds from the current Layout context.
 * 
 * @example
 *   if (fst::Button("Save")) { saveDocument(); }
 *   if (fst::ButtonPrimary("Submit")) { submitForm(); }
 */

namespace fst {

class Context;  // Forward declaration

//=============================================================================
// Button
//=============================================================================
struct ButtonOptions {
    Style style;
    bool primary = false;      // Use primary color
    bool disabled = false;
    std::string icon;          // Optional icon (future)
};

/// Explicit DI version - preferred for new code
[[nodiscard]] bool Button(Context& ctx, std::string_view label, const ButtonOptions& options = {});

// Convenience overloads
[[nodiscard]] bool ButtonPrimary(Context& ctx, std::string_view label);

} // namespace fst

