#pragma once

/**
 * @file MockDrawList.h
 * @brief Google Mock implementation of IDrawList for unit testing.
 * 
 * @ai_hint Use this mock in tests to verify widget rendering:
 *   - EXPECT_CALL to verify specific draw calls
 *   - Use ::testing::_ for any-value matching
 *   - Use AtLeast(1) for required calls
 */

#include <gmock/gmock.h>
#include <fastener/graphics/IDrawList.h>

namespace fst {
namespace testing {

class MockDrawList : public IDrawList {
public:
    // Clipping
    MOCK_METHOD(void, pushClipRect, (const Rect& rect), (override));
    MOCK_METHOD(void, popClipRect, (), (override));
    MOCK_METHOD(Rect, currentClipRect, (), (const, override));
    
    // Color Stack
    MOCK_METHOD(void, pushColor, (Color color), (override));
    MOCK_METHOD(void, popColor, (), (override));
    MOCK_METHOD(Color, currentColor, (), (const, override));
    
    // Primitives
    MOCK_METHOD(void, addRect, (const Rect& rect, Color color, float rounding), (override));
    MOCK_METHOD(void, addRectFilled, (const Rect& rect, Color color, float rounding), (override));
    MOCK_METHOD(void, addRectFilledMultiColor, 
                (const Rect& rect, Color topLeft, Color topRight, Color bottomRight, Color bottomLeft), 
                (override));
    
    MOCK_METHOD(void, addLine, (const Vec2& p1, const Vec2& p2, Color color, float thickness), (override));
    MOCK_METHOD(void, addCircle, (const Vec2& center, float radius, Color color, int segments), (override));
    MOCK_METHOD(void, addCircleFilled, (const Vec2& center, float radius, Color color, int segments), (override));
    
    MOCK_METHOD(void, addTriangle, (const Vec2& p1, const Vec2& p2, const Vec2& p3, Color color), (override));
    MOCK_METHOD(void, addTriangleFilled, (const Vec2& p1, const Vec2& p2, const Vec2& p3, Color color), (override));
    
    // Text
    MOCK_METHOD(void, addText, (Font* font, const Vec2& pos, std::string_view text, Color color), (override));
    
    // Images
    MOCK_METHOD(void, addImage, (const Texture* texture, const Rect& rect, Color tint), (override));
    MOCK_METHOD(void, addImage, 
                (const Texture* texture, const Rect& rect, const Vec2& uv0, const Vec2& uv1, Color tint), 
                (override));
    MOCK_METHOD(void, addImageRounded, 
                (const Texture* texture, const Rect& rect, float rounding, Color tint), (override));
    
    // Blur
    MOCK_METHOD(void, addBlurRect, 
                (const Rect& rect, float blurRadius, float rounding, Color tint), (override));
    
    // Shadow
    MOCK_METHOD(void, addShadow, (const Rect& rect, Color color, float size, float rounding), (override));
    
    // Texture batching
    MOCK_METHOD(void, setTexture, (uint32_t textureId), (override));
    
    // Layers
    MOCK_METHOD(void, setLayer, (DrawLayer layer), (override));
    MOCK_METHOD(DrawLayer, currentLayer, (), (const, override));
    
    // Color resolution
    MOCK_METHOD(Color, resolveColor, (Color color), (const, override));
};

} // namespace testing
} // namespace fst
