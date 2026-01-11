#pragma once

#include "fastener/core/types.h"
#include "fastener/ui/style.h"
#include <string>

namespace fst {

class Context;

//=============================================================================
// Tooltip Options
//=============================================================================
struct TooltipOptions {
    float delay = 0.5f;           // Seconds before showing
    float maxWidth = 300.0f;      // Max width before wrapping
    float fadeInDuration = 0.1f;  // Fade animation duration
};

struct HelpMarkerOptions {
    Style style;
    TooltipOptions tooltipOptions;
};

//=============================================================================
// Tooltip Functions
//=============================================================================

/// Explicit DI versions
void Tooltip(Context& ctx, const char* text, const TooltipOptions& options = {});
void ShowTooltip(Context& ctx, const char* text, Vec2 position, const TooltipOptions& options = {});
bool HelpMarker(Context& ctx, const char* text, const HelpMarkerOptions& options = {});

/// Uses context stack
void Tooltip(const char* text, const TooltipOptions& options = {});
void Tooltip(const std::string& text, const TooltipOptions& options = {});
void ShowTooltip(const char* text, Vec2 position, const TooltipOptions& options = {});
void ShowTooltip(const std::string& text, Vec2 position, const TooltipOptions& options = {});
bool HelpMarker(const char* text, const HelpMarkerOptions& options = {});
bool HelpMarker(const std::string& text, const HelpMarkerOptions& options = {});

//=============================================================================
// Internal Tooltip State Management
//=============================================================================
namespace internal {

struct TooltipState {
    WidgetId hoveredWidget = INVALID_WIDGET_ID;
    Rect hoveredBounds;
    float hoverStartTime = 0.0f;
    bool isShowing = false;
    float showAlpha = 0.0f;
};

TooltipState& getTooltipState();
void registerHoveredWidget(WidgetId id, const Rect& bounds);
void renderActiveTooltip();
void resetTooltipState();

} // namespace internal

} // namespace fst

