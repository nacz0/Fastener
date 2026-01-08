#pragma once

#include "fastener/core/types.h"
#include "fastener/ui/style.h"
#include "fastener/graphics/texture.h"

namespace fst {

//=============================================================================
// Image
//=============================================================================

/**
 * @brief Options for Image widget styling.
 */
struct ImageOptions {
    Style style;
    Color tint = Color(255, 255, 255, 255);  // White = no tint
    float borderRadius = 0.0f;
};

/**
 * @brief Renders an image from a texture.
 * 
 * @param texture Pointer to the texture to display
 * @param options Styling options including tint and border radius
 */
void Image(Texture* texture, const ImageOptions& options = {});

/**
 * @brief Renders an image with explicit size.
 * 
 * @param texture Pointer to the texture to display
 * @param size Explicit size for the image
 * @param options Styling options
 */
void Image(Texture* texture, Vec2 size, const ImageOptions& options = {});

} // namespace fst
