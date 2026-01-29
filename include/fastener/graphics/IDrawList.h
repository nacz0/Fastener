#pragma once

/**
 * @file IDrawList.h
 * @brief Pure virtual interface for draw list operations.
 * 
 * @ai_hint This interface enables mocking of DrawList for unit testing.
 *          Use MockDrawList in tests to verify widget rendering calls.
 */

#include "fastener/core/types.h"
#include <string_view>

namespace fst {

// Forward declarations
class Texture;
class Font;

//=============================================================================
// Draw Layers
//=============================================================================
enum class DrawLayer {
    Default,    // Docked windows, background
    Floating,   // Floating windows
    Overlay,    // Tooltips, drag previews, menus
    Count
};

//=============================================================================
// IDrawList - Abstract interface for draw operations
//=============================================================================
class IDrawList {
public:
    virtual ~IDrawList() = default;
    
    // Clipping
    virtual void pushClipRect(const Rect& rect) = 0;
    virtual void popClipRect() = 0;
    virtual Rect currentClipRect() const = 0;
    
    // Color Stack
    virtual void pushColor(Color color) = 0;
    virtual void popColor() = 0;
    virtual Color currentColor() const = 0;
    
    // Primitives
    virtual void addRect(const Rect& rect, Color color = Color::none(), float rounding = 0.0f) = 0;
    virtual void addRectFilled(const Rect& rect, Color color = Color::none(), float rounding = 0.0f) = 0;
    virtual void addRectFilledMultiColor(const Rect& rect, Color topLeft, Color topRight, 
                                          Color bottomRight, Color bottomLeft) = 0;
    
    virtual void addLine(const Vec2& p1, const Vec2& p2, Color color = Color::none(), float thickness = 1.0f) = 0;
    virtual void addCircle(const Vec2& center, float radius, Color color = Color::none(), int segments = 0) = 0;
    virtual void addCircleFilled(const Vec2& center, float radius, Color color = Color::none(), int segments = 0) = 0;
    
    virtual void addTriangle(const Vec2& p1, const Vec2& p2, const Vec2& p3, Color color = Color::none()) = 0;
    virtual void addTriangleFilled(const Vec2& p1, const Vec2& p2, const Vec2& p3, Color color = Color::none()) = 0;
    
    // Text
    virtual void addText(Font* font, const Vec2& pos, std::string_view text, Color color = Color::none()) = 0;
    
    // Images
    virtual void addImage(const Texture* texture, const Rect& rect, Color tint = Color::white()) = 0;
    virtual void addImage(const Texture* texture, const Rect& rect, const Vec2& uv0, const Vec2& uv1, 
                          Color tint = Color::white()) = 0;
    virtual void addImageRounded(const Texture* texture, const Rect& rect, float rounding, 
                                 Color tint = Color::white()) = 0;
    
    // Blur
    virtual void addBlurRect(const Rect& rect, float blurRadius, float rounding = 0.0f, 
                             Color tint = Color::none()) = 0;
    
    // Shadow
    virtual void addShadow(const Rect& rect, Color color, float size, float rounding = 0.0f) = 0;
    
    // Texture batching
    virtual void setTexture(uint32_t textureId) = 0;
    
    // Layers
    virtual void setLayer(DrawLayer layer) = 0;
    virtual DrawLayer currentLayer() const = 0;
    
    // Color resolution helper
    virtual Color resolveColor(Color color) const = 0;
};

} // namespace fst
