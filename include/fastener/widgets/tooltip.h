#pragma once

#include "fastener/core/types.h"
#include "fastener/ui/style.h"
#include <string>

namespace fst {

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

// Show tooltip for the last widget if hovered
// Call immediately after a widget (Button, Checkbox, etc.)
void Tooltip(const char* text, const TooltipOptions& options = {});
void Tooltip(const std::string& text, const TooltipOptions& options = {});

// Manually show tooltip at specific position
void ShowTooltip(const char* text, Vec2 position, const TooltipOptions& options = {});
void ShowTooltip(const std::string& text, Vec2 position, const TooltipOptions& options = {});

// Small "?" icon that shows tooltip on hover
// Very useful for explaining options in dense UIs
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

// Called by widgets to register hover tracking
void registerHoveredWidget(WidgetId id, const Rect& bounds);

// Called at end of frame to render active tooltip
void renderActiveTooltip();

// Reset tooltip state (call at frame start)
void resetTooltipState();

} // namespace internal

} // namespace fst
