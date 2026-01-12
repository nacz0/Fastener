/**
 * @file collapsing_header.cpp
 * @brief CollapsingHeader widget implementation for expandable sections.
 */

#include "fastener/widgets/collapsing_header.h"
#include "fastener/core/context.h"
#include "fastener/graphics/draw_list.h"
#include "fastener/graphics/font.h"
#include "fastener/ui/widget.h"
#include "fastener/ui/widget_utils.h"
#include "fastener/ui/theme.h"
#include "fastener/ui/layout.h"

namespace fst {

//=============================================================================
// CollapsingHeader Implementation
//=============================================================================

/**
 * @brief Renders a collapsible header that toggles visibility of content.
 * 
 * @param label Text displayed on the header
 * @param isOpen Reference to the open/closed state
 * @param options Styling and behavior options
 * @return true if the header is currently open (for conditional child rendering)
 */
bool CollapsingHeader(Context& ctx, std::string_view label, bool& isOpen,
                      const CollapsingHeaderOptions& options) {
    auto wc = WidgetContext::make(ctx);
    
    const Theme& theme = *wc.theme;
    IDrawList& dl = *wc.dl;
    Font* font = wc.font;
    
    // Generate unique ID
    WidgetId id = ctx.makeId(label);
    
    // Calculate dimensions
    float width = options.style.width > 0 ? options.style.width : 0; // 0 = auto-expand
    float height = options.style.height > 0 ? options.style.height : theme.metrics.inputHeight;
    
    // Calculate arrow and text sizes
    float arrowSize = height * 0.4f;
    Vec2 textSize = font ? font->measureText(label) : Vec2(0, height);
    
    if (width <= 0) {
        // Auto-expand to content width
        width = arrowSize + theme.metrics.paddingSmall + textSize.x + theme.metrics.paddingMedium * 2;
    }
    
    // Allocate bounds
    Rect bounds = allocateWidgetBounds(ctx, options.style, width, height);
    
    // Handle interaction
    WidgetInteraction interaction = handleWidgetInteraction(ctx, id, bounds, true);
    WidgetState state = getWidgetState(ctx, id);
    
    // Toggle on click
    if (interaction.clicked) {
        isOpen = !isOpen;
    }
    
    // Draw background
    Color bgColor = getStateColor(
        theme.colors.panelBackground.lighter(0.02f),
        theme.colors.panelBackground.lighter(0.06f),
        theme.colors.panelBackground.lighter(0.1f),
        state
    );
    float radius = theme.metrics.borderRadiusSmall;
    dl.addRectFilled(bounds, bgColor, radius);
    
    // Draw arrow indicator
    float arrowX = bounds.x() + theme.metrics.paddingMedium;
    Vec2 arrowCenter(arrowX + arrowSize * 0.5f, bounds.center().y);
    
    Color arrowColor = theme.colors.textSecondary;
    
    if (isOpen) {
        // Down arrow (▼)
        Vec2 p1(arrowCenter.x - arrowSize * 0.35f, arrowCenter.y - arrowSize * 0.2f);
        Vec2 p2(arrowCenter.x + arrowSize * 0.35f, arrowCenter.y - arrowSize * 0.2f);
        Vec2 p3(arrowCenter.x, arrowCenter.y + arrowSize * 0.3f);
        dl.addTriangleFilled(p1, p2, p3, arrowColor);
    } else {
        // Right arrow (▶)
        Vec2 p1(arrowCenter.x - arrowSize * 0.2f, arrowCenter.y - arrowSize * 0.35f);
        Vec2 p2(arrowCenter.x + arrowSize * 0.3f, arrowCenter.y);
        Vec2 p3(arrowCenter.x - arrowSize * 0.2f, arrowCenter.y + arrowSize * 0.35f);
        dl.addTriangleFilled(p1, p2, p3, arrowColor);
    }
    
    // Draw label text
    if (font && !label.empty()) {
        float textY = layout_utils::verticalCenterY(bounds.y(), height, textSize.y);
        float textX = arrowX + arrowSize + theme.metrics.paddingSmall;
        dl.addText(font, Vec2(textX, textY), label, theme.colors.text);
    }
    
    return isOpen;
}

} // namespace fst
