/**
 * @file breadcrumb.cpp
 * @brief Breadcrumb widget implementation.
 */

#include "fastener/widgets/breadcrumb.h"
#include "fastener/core/context.h"
#include "fastener/graphics/draw_list.h"
#include "fastener/graphics/font.h"
#include "fastener/ui/widget.h"
#include "fastener/ui/widget_utils.h"
#include "fastener/ui/theme.h"
#include "fastener/ui/layout.h"
#include <algorithm>

namespace fst {

//=============================================================================
// Breadcrumb Implementation
//=============================================================================

int Breadcrumb(Context& ctx, const std::vector<std::string>& items, const BreadcrumbOptions& options) {
    if (items.empty()) return -1;
    
    auto wc = WidgetContext::make(ctx);
    
    const Theme& theme = *wc.theme;
    IDrawList& dl = *wc.dl;
    Font* font = wc.font;
    
    if (!font) return -1;
    
    int clickedIndex = -1;
    
    // Calculate total width needed
    float totalWidth = 0;
    Vec2 separatorSize = font->measureText(options.separator);
    float spacing = theme.metrics.paddingSmall;
    
    for (size_t i = 0; i < items.size(); ++i) {
        totalWidth += font->measureText(items[i]).x;
        if (i < items.size() - 1) {
            totalWidth += spacing + separatorSize.x + spacing;
        }
    }
    
    float height = font->lineHeight();
    
    // Allocate bounds
    Rect bounds = allocateWidgetBounds(ctx, options.style, totalWidth, height);
    
    // Draw each item
    float currentX = bounds.x();
    float textY = bounds.y();
    
    for (size_t i = 0; i < items.size(); ++i) {
        const std::string& item = items[i];
        Vec2 itemSize = font->measureText(item);
        bool isLast = (i == items.size() - 1);
        
        // Create item bounds for interaction
        Rect itemBounds(currentX, bounds.y(), itemSize.x, height);
        
        // Determine color based on state
        Color textColor;
        
        if (isLast) {
            // Last item is current location - not clickable
            textColor = theme.colors.text;
        } else {
            // Clickable items
            ctx.pushId(static_cast<int>(i));
            WidgetId id = ctx.makeId(item);
            WidgetInteraction interaction = handleWidgetInteraction(ctx, id, itemBounds, true);
            WidgetState state = getWidgetState(ctx, id);
            ctx.popId();
            
            if (interaction.clicked) {
                clickedIndex = static_cast<int>(i);
            }
            
            if (state.hovered) {
                textColor = theme.colors.primaryHover;
            } else {
                textColor = theme.colors.primary;
            }
        }
        
        // Draw item text
        dl.addText(font, Vec2(currentX, textY), item, textColor);
        
        // Draw underline for clickable items on hover
        if (!isLast) {
            ctx.pushId(static_cast<int>(i));
            WidgetId id = ctx.makeId(item);
            WidgetState state = getWidgetState(ctx, id);
            ctx.popId();
            
            if (state.hovered) {
                dl.addLine(
                    Vec2(currentX, textY + itemSize.y),
                    Vec2(currentX + itemSize.x, textY + itemSize.y),
                    theme.colors.primaryHover,
                    1.0f
                );
            }
        }
        
        currentX += itemSize.x;
        
        // Draw separator (except after last item)
        if (!isLast) {
            currentX += spacing;
            dl.addText(font, Vec2(currentX, textY), options.separator, theme.colors.textSecondary);
            currentX += separatorSize.x + spacing;
        }
    }
    
    return clickedIndex;
}

} // namespace fst
