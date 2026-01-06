/**
 * @file splitter.cpp
 * @brief Splitter widget implementation for resizable split views.
 */

#include "fastener/widgets/splitter.h"
#include "fastener/core/context.h"
#include "fastener/graphics/draw_list.h"
#include "fastener/ui/widget.h"
#include "fastener/ui/widget_utils.h"
#include "fastener/ui/theme.h"
#include "fastener/platform/window.h"
#include <algorithm>

namespace fst {

//=============================================================================
// Splitter Implementation
//=============================================================================

/**
 * @brief Renders a draggable splitter bar for resizing adjacent panels.
 * 
 * @param id_str Unique identifier string for the widget
 * @param splitPosition Reference to the split position (modified on drag)
 *                      For vertical splitter: X offset from left
 *                      For horizontal splitter: Y offset from top
 * @param bounds Container bounds within which the splitter operates
 * @param options Splitter styling and behavior options
 * @return true if the split position was changed this frame
 */
bool Splitter(const char* id_str, float& splitPosition, const Rect& bounds,
             const SplitterOptions& options) {
    // Get widget context
    auto wc = getWidgetContext();
    if (!wc.valid()) return false;

    const Theme& theme = *wc.theme;
    DrawList& dl = *wc.dl;

    // Generate unique ID
    WidgetId id = wc.ctx->makeId(id_str);

    // Calculate splitter visual bounds based on direction
    Rect splitterBounds;
    if (options.direction == Direction::Vertical) {
        // Vertical divider separates left/right panels
        splitterBounds = Rect(
            bounds.x() + splitPosition - options.splitterWidth * 0.5f,
            bounds.y(),
            options.splitterWidth,
            bounds.height()
        );
    } else {
        // Horizontal divider separates top/bottom panels
        splitterBounds = Rect(
            bounds.x(),
            bounds.y() + splitPosition - options.splitterWidth * 0.5f,
            bounds.width(),
            options.splitterWidth
        );
    }

    // Expand hit area for better usability
    Rect hitBounds = splitterBounds.expanded(4.0f);
    WidgetInteraction interaction = handleWidgetInteraction(id, hitBounds, false);
    WidgetState state = getWidgetState(id);
    state.disabled = options.disabled;

    // Set appropriate resize cursor on hover/active
    if (state.hovered || state.active) {
        Cursor cursor = (options.direction == Direction::Vertical) 
            ? Cursor::ResizeH 
            : Cursor::ResizeV;
        wc.ctx->window().setCursor(cursor);
    }

    bool changed = false;

    // Handle drag to update split position
    if (state.active && !options.disabled) {
        Vec2 mousePos = wc.ctx->input().mousePos();
        float newPos = splitPosition;

        if (options.direction == Direction::Vertical) {
            newPos = mousePos.x - bounds.x();
            // Apply minimum size constraints
            newPos = std::clamp(newPos, options.minSize1, bounds.width() - options.minSize2);
        } else {
            newPos = mousePos.y - bounds.y();
            newPos = std::clamp(newPos, options.minSize1, bounds.height() - options.minSize2);
        }

        if (newPos != splitPosition) {
            splitPosition = newPos;
            changed = true;
        }
    }

    // Determine splitter color based on state
    Color color = getStateColor(
        theme.colors.border,
        theme.colors.primaryHover,
        theme.colors.primary,
        state
    );

    // Draw visual splitter bar (thinner line for wide splitter areas)
    Rect visualBar = splitterBounds;
    if (options.splitterWidth > 2.0f) {
        if (options.direction == Direction::Vertical) {
            visualBar = Rect(
                splitterBounds.center().x - 0.5f, 
                splitterBounds.y(), 
                1.0f, 
                splitterBounds.height()
            );
        } else {
            visualBar = Rect(
                splitterBounds.x(), 
                splitterBounds.center().y - 0.5f, 
                splitterBounds.width(), 
                1.0f
            );
        }
    }

    dl.addRectFilled(visualBar, color);
    
    // Draw highlight overlay when interacting
    if (state.hovered || state.active) {
        dl.addRectFilled(splitterBounds, color.withAlpha(0.2f));
    }

    return changed;
}

/**
 * @brief String overload for Splitter.
 */
bool Splitter(const std::string& id, float& splitPosition, const Rect& bounds,
             const SplitterOptions& options) {
    return Splitter(id.c_str(), splitPosition, bounds, options);
}

} // namespace fst
