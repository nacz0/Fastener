#pragma once

#include "fastener/core/types.h"
#include <vector>

namespace fst {

// Forward declarations
class Texture;
class Font;

//=============================================================================
// Draw Vertex
//=============================================================================
struct DrawVertex {
    Vec2 pos;
    Vec2 uv;
    uint32_t color;  // ABGR packed
};

//=============================================================================
// Draw Command
//=============================================================================
struct DrawCommand {
    uint32_t textureId = 0;
    uint32_t indexOffset = 0;
    uint32_t indexCount = 0;
    Rect clipRect;
};

//=============================================================================
// Draw List - Collects draw commands for rendering
//=============================================================================
class DrawList {
public:
    DrawList();
    ~DrawList();
    
    // Clear for new frame
    void clear();
    
    // Clipping
    void pushClipRect(const Rect& rect);
    void pushClipRectFullScreen(const Vec2& screenSize);
    void popClipRect();
    Rect currentClipRect() const;
    
    // Primitives
    void addRect(const Rect& rect, Color color, float rounding = 0.0f);
    void addRectFilled(const Rect& rect, Color color, float rounding = 0.0f);
    void addRectFilledMultiColor(const Rect& rect, Color topLeft, Color topRight, 
                                  Color bottomRight, Color bottomLeft);
    
    void addLine(const Vec2& p1, const Vec2& p2, Color color, float thickness = 1.0f);
    void addCircle(const Vec2& center, float radius, Color color, int segments = 0);
    void addCircleFilled(const Vec2& center, float radius, Color color, int segments = 0);
    
    void addTriangle(const Vec2& p1, const Vec2& p2, const Vec2& p3, Color color);
    void addTriangleFilled(const Vec2& p1, const Vec2& p2, const Vec2& p3, Color color);
    
    // Text
    void addText(const Font* font, const Vec2& pos, const std::string& text, Color color);
    void addText(const Font* font, const Vec2& pos, const char* text, const char* textEnd, Color color);
    
    // Images
    void addImage(const Texture* texture, const Rect& rect, Color tint = Color::white());
    void addImage(const Texture* texture, const Rect& rect, const Vec2& uv0, const Vec2& uv1, 
                  Color tint = Color::white());
    void addImageRounded(const Texture* texture, const Rect& rect, float rounding, 
                         Color tint = Color::white());
    
    // Shadow (soft rectangle)
    void addShadow(const Rect& rect, Color color, float size, float rounding = 0.0f);
    
    // Access to raw data for rendering
    const std::vector<DrawVertex>& vertices() const { return m_vertices; }
    const std::vector<uint32_t>& indices() const { return m_indices; }
    const std::vector<DrawCommand>& commands() const { return m_commands; }
    
    // Current texture (for batching)
    void setTexture(uint32_t textureId);
    
private:
    std::vector<DrawVertex> m_vertices;
    std::vector<uint32_t> m_indices;
    std::vector<DrawCommand> m_commands;
    std::vector<Rect> m_clipRectStack;
    
    uint32_t m_currentTexture = 0;
    
    // Helpers
    void addVertex(const Vec2& pos, const Vec2& uv, Color color);
    void addIndex(uint32_t idx);
    void addQuad(const Vec2& p0, const Vec2& p1, const Vec2& p2, const Vec2& p3,
                 const Vec2& uv0, const Vec2& uv1, const Vec2& uv2, const Vec2& uv3,
                 Color color);
    void addQuadFilled(const Rect& rect, Color color);
    void primRect(const Rect& rect, Color color, float rounding);
    void primRectFilled(const Rect& rect, Color color, float rounding);
    
    void updateCommand();
};

} // namespace fst
