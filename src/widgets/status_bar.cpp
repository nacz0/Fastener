/**
 * @file status_bar.cpp
 * @brief StatusBar widget implementation.
 */

#include "fastener/widgets/status_bar.h"
#include "fastener/core/context.h"
#include "fastener/graphics/draw_list.h"
#include "fastener/graphics/font.h"
#include "fastener/ui/widget_utils.h"
#include "fastener/ui/theme.h"
#include "fastener/ui/layout.h"
#include "../core/widget_state_registry.h"
#include <algorithm>

namespace fst {

//=============================================================================
// StatusBar State (stored in context)
//=============================================================================

namespace {

struct StatusBarState {
    Rect bounds;
    float currentX = 0;
    float height = 0;
    int sectionCount = 0;
    bool active = false;
};

StatusBarState& getStatusBarState(Context& ctx) {
    return detail::widgetStates(ctx).get<StatusBarState>();
}

} // anonymous namespace

//=============================================================================
// StatusBar Implementation
//=============================================================================

bool BeginStatusBar(Context& ctx, const StatusBarOptions& options) {
    auto wc = WidgetContext::make(ctx);
    
    const Theme& theme = *wc.theme;
    IDrawList& dl = *wc.dl;
    StatusBarState& state = getStatusBarState(ctx);
    
    // Calculate dimensions
    float height = options.height > 0 ? options.height : 24.0f;
    float width = options.style.width > 0 ? options.style.width : ctx.window().width();
    
    // Allocate bounds (typically at bottom)
    Rect bounds = allocateWidgetBounds(ctx, options.style, width, height);
    
    // Draw status bar background
    dl.addRectFilled(bounds, theme.colors.panelBackground.darker(0.1f));
    
    // Draw top border
    dl.addLine(
        Vec2(bounds.x(), bounds.y()),
        Vec2(bounds.right(), bounds.y()),
        theme.colors.border,
        1.0f
    );
    
    // Initialize state
    state.bounds = bounds;
    state.currentX = bounds.x() + theme.metrics.paddingSmall;
    state.height = height;
    state.sectionCount = 0;
    state.active = true;
    
    return true;
}

void EndStatusBar(Context& ctx) {
    getStatusBarState(ctx).active = false;
}

void StatusBarSection(Context& ctx, std::string_view text, const StatusBarSectionOptions& options) {
    StatusBarState& state = getStatusBarState(ctx);
    if (!state.active) return;
    
    auto wc = WidgetContext::make(ctx);
    
    const Theme& theme = *wc.theme;
    IDrawList& dl = *wc.dl;
    Font* font = wc.font;
    
    if (!font) return;
    
    // Draw separator if not first section
    if (state.sectionCount > 0) {
        float sepX = state.currentX;
        dl.addLine(
            Vec2(sepX, state.bounds.y() + 4),
            Vec2(sepX, state.bounds.bottom() - 4),
            theme.colors.border,
            1.0f
        );
        state.currentX += theme.metrics.paddingSmall;
    }
    
    // Calculate text dimensions
    Vec2 textSize = font->measureText(text);
    float sectionWidth = std::max(textSize.x + theme.metrics.paddingSmall * 2, options.minWidth);
    
    // Calculate text position
    float textX = state.currentX + theme.metrics.paddingSmall;
    if (options.alignRight && sectionWidth > textSize.x) {
        textX = state.currentX + sectionWidth - textSize.x - theme.metrics.paddingSmall;
    }
    
    float textY = state.bounds.center().y - textSize.y * 0.5f;
    
    // Draw text
    dl.addText(font, Vec2(textX, textY), text, theme.colors.textSecondary);
    
    // Update state
    state.currentX += sectionWidth;
    state.sectionCount++;
}

} // namespace fst
