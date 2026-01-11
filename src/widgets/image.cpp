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
// Image Implementation
//=============================================================================

/**
 * @brief Renders an image from a texture.
 * 
 * @param texture Pointer to the texture to display
 * @param options Styling options including tint and border radius
 */
void Image(Texture* texture, const ImageOptions& options) {
    if (!texture && options.style.width <= 0 && options.style.height <= 0) return;
    
    Vec2 size(0, 0);
    if (texture) {
        size = Vec2(static_cast<float>(texture->width()), 
                    static_cast<float>(texture->height()));
    }
    Image(texture, size, options);
}

/**
 * @brief Renders an image with explicit size.
 * 
 * @param texture Pointer to the texture to display
 * @param size Explicit size for the image
 * @param options Styling options
 */
void Image(Texture* texture, Vec2 size, const ImageOptions& options) {
    // Get widget context
    auto wc = getWidgetContext();
    if (!wc.valid()) return;
    
    IDrawList& dl = *wc.dl;
    
    // Use explicit size or style size
    float width = options.style.width > 0 ? options.style.width : size.x;
    float height = options.style.height > 0 ? options.style.height : size.y;
    
    // Allocate bounds
    Rect bounds = allocateWidgetBounds(options.style, width, height);
    
    // Draw image with optional tint
    if (texture) {
        // UV coordinates for full texture
        Vec2 uv0(0.0f, 0.0f);
        Vec2 uv1(1.0f, 1.0f);
        
        if (options.borderRadius > 0.0f) {
            // For rounded images, we'd need clipping - for now draw normally
            dl.addImage(texture, bounds, uv0, uv1, options.tint);
        } else {
            dl.addImage(texture, bounds, uv0, uv1, options.tint);
        }
    } else {
        // Draw placeholder rectangle if no texture
        const Theme& theme = *wc.theme;
        dl.addRectFilled(bounds, theme.colors.secondary, options.borderRadius);
    }
}

} // namespace fst
