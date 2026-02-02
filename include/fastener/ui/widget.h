#pragma once

#include "fastener/core/types.h"
#include "fastener/ui/style.h"
#include <string>

namespace fst {

// Forward declarations
class Context;
class DrawList;

//=============================================================================
// Widget State
//=============================================================================
struct WidgetState {
    bool hovered = false;
    bool focused = false;
    bool active = false;   // Being pressed/dragged
    bool disabled = false;
};

//=============================================================================
// Widget Interaction Result
//=============================================================================
struct WidgetInteraction {
    bool clicked = false;
    bool doubleClicked = false;
    bool rightClicked = false;
    bool hovered = false;
    bool focused = false;
    bool dragging = false;
    Vec2 dragDelta;
};

//=============================================================================
// Base Widget Functions
//=============================================================================

// Get widget state for a given ID
WidgetState getWidgetState(Context& ctx, WidgetId id);

// Handle basic widget interaction (hover, click, focus)
WidgetInteraction handleWidgetInteraction(Context& ctx, WidgetId id, const Rect& bounds, bool focusable = true,
                                          bool ignoreOcclusion = false, bool ignoreConsumed = false);

// Draw common widget background
void drawWidgetBackground(Context& ctx, const Rect& bounds, const Style& style, const WidgetState& state);

// Draw common widget border
void drawWidgetBorder(Context& ctx, const Rect& bounds, const Style& style, const WidgetState& state);

} // namespace fst
