/**
 * @file image.cpp
 * @brief Image widget implementation for texture display.
 */

#include "fastener/widgets/image.h"
#include "fastener/core/context.h"
#include "fastener/graphics/draw_list.h"
#include "fastener/ui/widget_utils.h"
#include "fastener/ui/theme.h"
#include "fastener/ui/layout.h"

namespace fst {

//=============================================================================
// Image Implementation (Explicit DI)
//=============================================================================

void Image(Context& ctx, Texture* texture, const ImageOptions& options) {
    if (!texture && options.style.width <= 0 && options.style.height <= 0) return;
    
    Vec2 size(0, 0);
    if (texture) {
        size = Vec2(static_cast<float>(texture->width()), 
                    static_cast<float>(texture->height()));
    }
    Image(ctx, texture, size, options);
}

void Image(Context& ctx, Texture* texture, Vec2 size, const ImageOptions& options) {
    auto wc = WidgetContext::make(ctx);
    
    IDrawList& dl = *wc.dl;
    
    // Use explicit size or style size
    float width = options.style.width > 0 ? options.style.width : size.x;
    float height = options.style.height > 0 ? options.style.height : size.y;
    
    // Allocate bounds
    Rect bounds = allocateWidgetBounds(options.style, width, height);
    
    // Draw image with optional tint
    if (texture) {
        Vec2 uv0(0.0f, 0.0f);
        Vec2 uv1(1.0f, 1.0f);
        dl.addImage(texture, bounds, uv0, uv1, options.tint);
    } else {
        const Theme& theme = *wc.theme;
        dl.addRectFilled(bounds, theme.colors.secondary, options.borderRadius);
    }
}

//=============================================================================
// Backward-compatible wrappers
//=============================================================================

void Image(Texture* texture, const ImageOptions& options) {
    auto wc = getWidgetContext();
    if (!wc.valid()) return;
    Image(*wc.ctx, texture, options);
}

void Image(Texture* texture, Vec2 size, const ImageOptions& options) {
    auto wc = getWidgetContext();
    if (!wc.valid()) return;
    Image(*wc.ctx, texture, size, options);
}

} // namespace fst

