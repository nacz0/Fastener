#pragma once

#include "fastener/core/types.h"
#include "fastener/ui/style.h"
#include <string_view>

/**
 * @file badge.h
 * @brief Small count/notification indicator widget
 * 
 * @ai_hint Badge displays a small pill or circle with a number or short text.
 *          Typically used for notification counts, status indicators, etc.
 *          Does not return interaction state - purely visual display.
 * 
 * @example
 *   fst::Badge(ctx, 5);  // Shows "5"
 *   fst::Badge(ctx, 123, {.maxValue = 99});  // Shows "99+"
 *   fst::Badge(ctx, "NEW", {.color = fst::Color::green()});
 */

namespace fst {

class Context;

//=============================================================================
// Badge
//=============================================================================
struct BadgeOptions {
    Style style;
    Color color;         // Empty = use theme primary
    Color textColor;     // Empty = use white
    int maxValue = 99;   // Numbers above this show as "99+"
    float minWidth = 0;  // Minimum width for consistent sizing
};

/// Display a numeric badge
void Badge(Context& ctx, int count, const BadgeOptions& options = {});

/// Display a text badge
void Badge(Context& ctx, std::string_view text, const BadgeOptions& options = {});

} // namespace fst
