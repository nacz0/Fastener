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
    DrawList& dl = *wc.dl;
    
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
void SeparatorWithLabel(const char* label, const SeparatorOptions& options) {
    // Get widget context
    auto wc = getWidgetContext();
    if (!wc.valid()) return;
    
    const Theme& theme = *wc.theme;
    DrawList& dl = *wc.dl;
    Font* font = wc.font;
    
    // Determine separator color
    Color sepColor = options.color.a > 0 ? options.color : theme.colors.border;
    
    // Measure label
    Vec2 textSize = font ? font->measureText(label) : Vec2(0, 0);
    float textPadding = theme.metrics.paddingMedium;
    
    // Calculate dimensions
    float width = options.style.width;
    if (width <= 0) {
        width = wc.ctx->layout().allocateRemaining().width();
    }
    float totalHeight = std::max(textSize.y, options.thickness) + theme.metrics.paddingSmall * 2;
    
    // Allocate bounds
    Rect bounds = allocateWidgetBounds(options.style, width, totalHeight);
    
    if (width <= 0) {
        width = bounds.width();
    }
    
    // Calculate line Y position
    float lineY = bounds.center().y - options.thickness * 0.5f;
    
    if (font && label[0] != '\0') {
        // Draw left line
        float textX = bounds.center().x - textSize.x * 0.5f;
        float leftLineWidth = textX - bounds.x() - textPadding;
        if (leftLineWidth > 0) {
            Rect leftLine(bounds.x(), lineY, leftLineWidth, options.thickness);
            dl.addRectFilled(leftLine, sepColor);
        }
        
        // Draw text
        float textY = bounds.center().y - textSize.y * 0.5f;
        dl.addText(font, Vec2(textX, textY), label, theme.colors.textSecondary);
        
        // Draw right line
        float rightLineStart = textX + textSize.x + textPadding;
        float rightLineWidth = bounds.right() - rightLineStart;
        if (rightLineWidth > 0) {
            Rect rightLine(rightLineStart, lineY, rightLineWidth, options.thickness);
            dl.addRectFilled(rightLine, sepColor);
        }
    } else {
        // No label, draw full line
        Rect fullLine(bounds.x(), lineY, bounds.width(), options.thickness);
        dl.addRectFilled(fullLine, sepColor);
    }
}

/**
 * @brief String overload for SeparatorWithLabel.
 */
void SeparatorWithLabel(const std::string& label, const SeparatorOptions& options) {
    SeparatorWithLabel(label.c_str(), options);
}

} // namespace fst
