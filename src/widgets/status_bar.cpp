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

// Thread-local status bar state
thread_local StatusBarState s_statusBarState;

} // anonymous namespace

//=============================================================================
// StatusBar Implementation
//=============================================================================

bool BeginStatusBar(Context& ctx, const StatusBarOptions& options) {
    auto wc = WidgetContext::make(ctx);
    
    const Theme& theme = *wc.theme;
    IDrawList& dl = *wc.dl;
    
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
    s_statusBarState.bounds = bounds;
    s_statusBarState.currentX = bounds.x() + theme.metrics.paddingSmall;
    s_statusBarState.height = height;
    s_statusBarState.sectionCount = 0;
    s_statusBarState.active = true;
    
    return true;
}

void EndStatusBar(Context& ctx) {
    (void)ctx;
    s_statusBarState.active = false;
}

void StatusBarSection(Context& ctx, std::string_view text, const StatusBarSectionOptions& options) {
    if (!s_statusBarState.active) return;
    
    auto wc = WidgetContext::make(ctx);
    
    const Theme& theme = *wc.theme;
    IDrawList& dl = *wc.dl;
    Font* font = wc.font;
    
    if (!font) return;
    
    // Draw separator if not first section
    if (s_statusBarState.sectionCount > 0) {
        float sepX = s_statusBarState.currentX;
        dl.addLine(
            Vec2(sepX, s_statusBarState.bounds.y() + 4),
            Vec2(sepX, s_statusBarState.bounds.bottom() - 4),
            theme.colors.border,
            1.0f
        );
        s_statusBarState.currentX += theme.metrics.paddingSmall;
    }
    
    // Calculate text dimensions
    Vec2 textSize = font->measureText(text);
    float sectionWidth = std::max(textSize.x + theme.metrics.paddingSmall * 2, options.minWidth);
    
    // Calculate text position
    float textX = s_statusBarState.currentX + theme.metrics.paddingSmall;
    if (options.alignRight && sectionWidth > textSize.x) {
        textX = s_statusBarState.currentX + sectionWidth - textSize.x - theme.metrics.paddingSmall;
    }
    
    float textY = s_statusBarState.bounds.center().y - textSize.y * 0.5f;
    
    // Draw text
    dl.addText(font, Vec2(textX, textY), text, theme.colors.textSecondary);
    
    // Update state
    s_statusBarState.currentX += sectionWidth;
    s_statusBarState.sectionCount++;
}

} // namespace fst
