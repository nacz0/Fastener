/**
 * @file badge.cpp
 * @brief Badge widget implementation.
 */

#include "fastener/widgets/badge.h"
#include "fastener/core/context.h"
#include "fastener/graphics/draw_list.h"
#include "fastener/graphics/font.h"
#include "fastener/ui/widget_utils.h"
#include "fastener/ui/theme.h"
#include "fastener/ui/layout.h"
#include <algorithm>
#include <string>

namespace fst {

//=============================================================================
// Badge Implementation
//=============================================================================

namespace {

void renderBadge(Context& ctx, std::string_view text, const BadgeOptions& options) {
    auto wc = WidgetContext::make(ctx);
    
    const Theme& theme = *wc.theme;
    IDrawList& dl = *wc.dl;
    Font* font = wc.font;
    
    if (!font) return;
    
    // Calculate dimensions
    Vec2 textSize = font->measureText(text);
    float padding = theme.metrics.paddingSmall;
    float height = textSize.y + padding;
    float minWidth = std::max(height, options.minWidth); // At least circular
    float width = std::max(textSize.x + padding * 2, minWidth);
    
    // Allocate bounds
    Rect bounds = allocateWidgetBounds(ctx, options.style, width, height);
    
    // Determine colors
    Color bgColor = options.color.a > 0 ? options.color : theme.colors.primary;
    Color txtColor = options.textColor.a > 0 ? options.textColor : Color::white();
    
    // Draw pill-shaped background
    float radius = height * 0.5f;
    dl.addRectFilled(bounds, bgColor, radius);
    
    // Draw centered text
    Vec2 textPos(
        bounds.center().x - textSize.x * 0.5f,
        bounds.center().y - textSize.y * 0.5f
    );
    dl.addText(font, textPos, text, txtColor);
}

} // anonymous namespace

void Badge(Context& ctx, int count, const BadgeOptions& options) {
    std::string text;
    if (count > options.maxValue) {
        text = std::to_string(options.maxValue) + "+";
    } else {
        text = std::to_string(count);
    }
    renderBadge(ctx, text, options);
}

void Badge(Context& ctx, std::string_view text, const BadgeOptions& options) {
    renderBadge(ctx, text, options);
}

} // namespace fst
