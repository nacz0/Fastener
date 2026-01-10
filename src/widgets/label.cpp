/**
 * @file label.cpp
 * @brief Label widget implementation for displaying static text.
 */

#include "fastener/widgets/label.h"
#include "fastener/core/context.h"
#include "fastener/graphics/draw_list.h"
#include "fastener/graphics/font.h"
#include "fastener/ui/widget_utils.h"
#include "fastener/ui/theme.h"
#include "fastener/ui/layout.h"

namespace fst {

//=============================================================================
// Label Implementation
//=============================================================================

/**
 * @brief Renders static text label.
 * 
 * @param text Text content to display
 * @param options Label styling options (color, size, position)
 */
void Label(std::string_view text, const LabelOptions& options) {
    // Get widget context
    auto wc = getWidgetContext();
    if (!wc.valid() || !wc.font) return;
    
    const Theme& theme = *wc.theme;
    DrawList& dl = *wc.dl;
    Font* font = wc.font;
    
    // Determine text color
    Color textColor = options.color.a > 0 ? options.color : theme.colors.text;
    
    // Measure text dimensions
    Vec2 textSize = font->measureText(text);
    
    // Calculate widget bounds
    float width = options.style.width > 0 ? options.style.width : textSize.x;
    float height = options.style.height > 0 ? options.style.height : textSize.y;
    
    // Allocate bounds
    Rect bounds = allocateWidgetBounds(options.style, width, height);
    
    // Draw text
    dl.addText(font, bounds.pos, text, textColor);
}

//=============================================================================
// Label Variants
//=============================================================================

/**
 * @brief Renders text with secondary/muted styling.
 */
void LabelSecondary(std::string_view text) {
    auto wc = getWidgetContext();
    if (!wc.valid()) return;
    
    LabelOptions options;
    options.color = wc.theme->colors.textSecondary;
    Label(text, options);
}

/**
 * @brief Renders text as a heading (larger/emphasized).
 */
void LabelHeading(std::string_view text) {
    Label(text);
}

} // namespace fst
