#include "fastener/widgets/splitter.h"
#include "fastener/core/context.h"
#include "fastener/graphics/draw_list.h"
#include "fastener/ui/widget.h"
#include "fastener/ui/theme.h"
#include "fastener/platform/window.h"
#include <algorithm>

namespace fst {

bool Splitter(const char* id_str, float& splitPosition, const Rect& bounds,
             const SplitterOptions& options) {
    Context* ctx = Context::current();
    if (!ctx) return false;

    const Theme& theme = ctx->theme();
    DrawList& dl = ctx->drawList();

    // Generate ID
    WidgetId id = ctx->makeId(id_str);

    // Calculate splitter visual bounds
    Rect splitterBounds;
    if (options.direction == Direction::Vertical) {
        // Divider is vertical, separates left/right
        splitterBounds = Rect(
            bounds.x() + splitPosition - options.splitterWidth * 0.5f,
            bounds.y(),
            options.splitterWidth,
            bounds.height()
        );
    } else {
        // Divider is horizontal, separates top/bottom
        splitterBounds = Rect(
            bounds.x(),
            bounds.y() + splitPosition - options.splitterWidth * 0.5f,
            bounds.width(),
            options.splitterWidth
        );
    }

    // Handle interaction
    // We use a slightly larger hit area for better UX
    Rect hitBounds = splitterBounds.expanded(4.0f);
    WidgetInteraction interaction = handleWidgetInteraction(id, hitBounds, false);
    WidgetState state = getWidgetState(id);
    state.disabled = options.disabled;

    // Set cursor
    if (state.hovered || state.active) {
        ctx->window().setCursor(options.direction == Direction::Vertical ? Cursor::ResizeH : Cursor::ResizeV);
    }

    bool changed = false;

    // Handle dragging
    if (state.active && !options.disabled) {
        Vec2 mousePos = ctx->input().mousePos();
        float newPos = splitPosition;

        if (options.direction == Direction::Vertical) {
            newPos = mousePos.x - bounds.x();
            // Apply constraints
            newPos = std::clamp(newPos, options.minSize1, bounds.width() - options.minSize2);
        } else {
            newPos = mousePos.y - bounds.y();
            // Apply constraints
            newPos = std::clamp(newPos, options.minSize1, bounds.height() - options.minSize2);
        }

        if (newPos != splitPosition) {
            splitPosition = newPos;
            changed = true;
        }
    }

    // Rysowanie
    Color color;
    if (options.disabled) {
        color = theme.colors.border.withAlpha(0.5f);
    } else if (state.active) {
        color = theme.colors.primary;
    } else if (state.hovered) {
        color = theme.colors.primaryHover;
    } else {
        color = theme.colors.border;
    }

    Rect visualBar = splitterBounds;
    if (options.splitterWidth > 2.0f) {
        if (options.direction == Direction::Vertical) {
            visualBar = Rect(splitterBounds.center().x - 0.5f, splitterBounds.y(), 1.0f, splitterBounds.height());
        } else {
            visualBar = Rect(splitterBounds.x(), splitterBounds.center().y - 0.5f, splitterBounds.width(), 1.0f);
        }
    }

    dl.addRectFilled(visualBar, color);
    
    if (state.hovered || state.active) {
        dl.addRectFilled(splitterBounds, color.withAlpha(0.2f));
    }

    return changed;
}

bool Splitter(const std::string& id, float& splitPosition, const Rect& bounds,
             const SplitterOptions& options) {
    return Splitter(id.c_str(), splitPosition, bounds, options);
}

} // namespace fst
