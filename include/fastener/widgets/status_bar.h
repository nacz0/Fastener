#pragma once

#include "fastener/core/types.h"
#include "fastener/ui/style.h"
#include <string_view>

/**
 * @file status_bar.h
 * @brief Horizontal status bar with multiple sections
 * 
 * @ai_hint StatusBar displays information in horizontal sections at a fixed position.
 *          Use Begin/End pattern. Add sections with StatusBarSection().
 *          Sections are automatically laid out with separators.
 * 
 * @example
 *   if (fst::BeginStatusBar(ctx)) {
 *       fst::StatusBarSection(ctx, "Ready");
 *       fst::StatusBarSection(ctx, "Line: 42");
 *       fst::StatusBarSection(ctx, "UTF-8");
 *   }
 *   fst::EndStatusBar(ctx);
 */

namespace fst {

class Context;

//=============================================================================
// StatusBar
//=============================================================================
struct StatusBarOptions {
    Style style;
    float height = 0.0f;    // 0 = use theme default (~24px)
};

struct StatusBarSectionOptions {
    Style style;
    float minWidth = 0.0f;  // Minimum section width
    bool alignRight = false; // Align text to right within section
};

/// Begin a status bar - returns true always
bool BeginStatusBar(Context& ctx, const StatusBarOptions& options = {});

/// End status bar
void EndStatusBar(Context& ctx);

/// Add a text section to the status bar
void StatusBarSection(Context& ctx, std::string_view text, const StatusBarSectionOptions& options = {});

} // namespace fst
