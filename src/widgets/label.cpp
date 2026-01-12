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
// Label Implementation (Explicit DI)
//=============================================================================

void Label(Context& ctx, std::string_view text, const LabelOptions& options) {
    auto wc = WidgetContext::make(ctx);
    if (!wc.font) return;
    
    const Theme& theme = *wc.theme;
    IDrawList& dl = *wc.dl;
    Font* font = wc.font;
    
    // Determine text color
    Color textColor = options.color.a > 0 ? options.color : theme.colors.text;
    
    // Measure text dimensions
    Vec2 textSize = font->measureText(text);
    
    // Calculate widget bounds
    float width = options.style.width > 0 ? options.style.width : textSize.x;
    float height = options.style.height > 0 ? options.style.height : textSize.y;
    
    // Allocate bounds
    Rect bounds = allocateWidgetBounds(ctx, options.style, width, height);
    
    // Draw text
    dl.addText(font, bounds.pos, text, textColor);
}



//=============================================================================
// Label Variants
//=============================================================================

void LabelSecondary(Context& ctx, std::string_view text) {
    LabelOptions options;
    options.color = ctx.theme().colors.textSecondary;
    Label(ctx, text, options);
}

void LabelHeading(Context& ctx, std::string_view text) {
    Label(ctx, text);
}

} // namespace fst

