/**
 * @file separator.cpp
 * @brief Separator widget implementation for visual dividers.
 */

#include "fastener/widgets/separator.h"
#include "fastener/core/context.h"
#include "fastener/graphics/draw_list.h"
#include "fastener/graphics/font.h"
#include "fastener/ui/widget_utils.h"
#include "fastener/ui/theme.h"
#include "fastener/ui/layout.h"

namespace fst {

//=============================================================================
// Separator Implementation
//=============================================================================

/**
 * @brief Renders a horizontal separator line.
 * 
 * @param options Styling options
 */
void Separator(const SeparatorOptions& options) {
    // Get widget context
    auto wc = getWidgetContext();
    if (!wc.valid()) return;
    
    const Theme& theme = *wc.theme;
    IDrawList& dl = *wc.dl;
    
    // Determine separator color
    Color sepColor = options.color.a > 0 ? options.color : theme.colors.border;
    
    // Calculate dimensions - separator takes full available width
    float width = options.style.width;
    if (width <= 0) {
        width = wc.ctx->layout().allocateRemaining().width();
    }
    float totalHeight = options.thickness + theme.metrics.paddingSmall * 2;
    
    // Allocate bounds
    Rect bounds = allocateWidgetBounds(options.style, width, totalHeight);
    
    // If width was 0, use full available width from bounds
    if (width <= 0) {
        width = bounds.width();
    }
    
    // Draw separator line centered vertically
    float lineY = bounds.y() + totalHeight * 0.5f - options.thickness * 0.5f;
    Rect lineRect(bounds.x(), lineY, bounds.width(), options.thickness);
    dl.addRectFilled(lineRect, sepColor);
}

/**
 * @brief Renders a separator with centered text label.
 * 
 * @param label Text to display centered on the separator
 * @param options Styling options
 */
void SeparatorWithLabel(std::string_view label, const SeparatorOptions& options) {
    auto wc = getWidgetContext();
    if (!wc.valid()) return;

    const Theme& theme = *wc.theme;
    IDrawList& dl = *wc.dl;
    Font* font = wc.font;

    // Use explicit height if provided, otherwise default
    float height = options.style.height > 0 ? options.style.height : 24.0f;
    float labelWidth = 0;
    
    if (font && !label.empty()) {
        labelWidth = font->measureText(label).x + theme.metrics.paddingMedium * 2;
    }

    // Allocate bounds - width 0 means fill width
    Rect bounds = allocateWidgetBounds(options.style, 0.0f, height);

    float centerY = bounds.center().y;
    Color color = options.color.a > 0 ? options.color : theme.colors.border;

    if (labelWidth > 0 && font) {
        float lineY = centerY;
        float segmentWidth = (bounds.width() - labelWidth) / 2.0f;
        
        // Left segment
        if (segmentWidth > 0) {
            dl.addLine(Vec2(bounds.x(), lineY), Vec2(bounds.x() + segmentWidth, lineY), color, options.thickness);
        }
        
        // Label
        Vec2 labelPos(bounds.x() + segmentWidth + theme.metrics.paddingMedium, 
                      centerY - font->lineHeight() * 0.5f);
        dl.addText(font, labelPos, label, theme.colors.textSecondary);
        
        // Right segment
        if (segmentWidth > 0) {
            dl.addLine(Vec2(bounds.right() - segmentWidth, lineY), Vec2(bounds.right(), lineY), color, options.thickness);
        }
    } else {
        // No label or no font, draw full line
        dl.addLine(Vec2(bounds.x(), centerY), Vec2(bounds.right(), centerY), color, options.thickness);
    }
}

} // namespace fst
