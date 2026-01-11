#pragma once

#include "fastener/graphics/IDrawList.h"
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
class DrawList : public IDrawList {
public:
    DrawList();
    ~DrawList();
    
    // Clear for new frame
    void clear();
    
    // Clipping
    void pushClipRect(const Rect& rect) override;
    void pushClipRectFullScreen(const Vec2& screenSize);
    void popClipRect() override;
    Rect currentClipRect() const override;
    
    // Layers
    void setLayer(DrawLayer layer) override;
    DrawLayer currentLayer() const override;
    
    // Color Stack
    void pushColor(Color color) override;
    void popColor() override;
    Color currentColor() const override;
    
    // Primitives
    void addRect(const Rect& rect, Color color = Color::none(), float rounding = 0.0f) override;
    void addRectFilled(const Rect& rect, Color color = Color::none(), float rounding = 0.0f) override;
    void addRectFilledMultiColor(const Rect& rect, Color topLeft, Color topRight, 
                                  Color bottomRight, Color bottomLeft) override;
    
    void addLine(const Vec2& p1, const Vec2& p2, Color color = Color::none(), float thickness = 1.0f) override;
    void addCircle(const Vec2& center, float radius, Color color = Color::none(), int segments = 0) override;
    void addCircleFilled(const Vec2& center, float radius, Color color = Color::none(), int segments = 0) override;
    
    void addTriangle(const Vec2& p1, const Vec2& p2, const Vec2& p3, Color color = Color::none()) override;
    void addTriangleFilled(const Vec2& p1, const Vec2& p2, const Vec2& p3, Color color = Color::none()) override;
    
    // Text
    void addText(const Font* font, const Vec2& pos, std::string_view text, Color color = Color::none()) override;
    
    // Images
    void addImage(const Texture* texture, const Rect& rect, Color tint = Color::white()) override;
    void addImage(const Texture* texture, const Rect& rect, const Vec2& uv0, const Vec2& uv1, 
                  Color tint = Color::white()) override;
    void addImageRounded(const Texture* texture, const Rect& rect, float rounding, 
                         Color tint = Color::white()) override;
    
    // Shadow (soft rectangle)
    void addShadow(const Rect& rect, Color color, float size, float rounding = 0.0f) override;
    
    // Final merged data for rendering (merged by mergeLayers())
    const std::vector<DrawVertex>& vertices() const { return m_mergedVertices; }
    const std::vector<uint32_t>& indices() const { return m_mergedIndices; }
    const std::vector<DrawCommand>& commands() const { return m_mergedCommands; }
    
    // Consolidate all layers into the merged buffers
    void mergeLayers();
    
    // Current texture (for batching)
    void setTexture(uint32_t textureId) override;
    
    // Color resolution helper
    Color resolveColor(Color color) const override;
    
private:
    struct LayerData {
        std::vector<DrawVertex> vertices;
        std::vector<uint32_t> indices;
        std::vector<DrawCommand> commands;
        std::vector<Rect> clipRectStack;
        std::vector<Color> colorStack;
        uint32_t currentTexture = 0;
    };

    LayerData m_layers[static_cast<int>(DrawLayer::Count)];
    DrawLayer m_currentLayer = DrawLayer::Default;

    // Merged results for the renderer
    std::vector<DrawVertex> m_mergedVertices;
    std::vector<uint32_t> m_mergedIndices;
    std::vector<DrawCommand> m_mergedCommands;
    
    // Helpers (these now work on the current layer)
    void addVertex(const Vec2& pos, const Vec2& uv, Color color);
    void addIndex(uint32_t idx);
    void addQuad(const Vec2& p0, const Vec2& p1, const Vec2& p2, const Vec2& p3,
                 const Vec2& uv0, const Vec2& uv1, const Vec2& uv2, const Vec2& uv3,
                 Color color);
    void addQuadFilled(const Rect& rect, Color color);
    void primRect(const Rect& rect, Color color, float rounding);
    void primRectFilled(const Rect& rect, Color color, float rounding);
    
    void updateCommand();
    LayerData& currentData() { return m_layers[static_cast<int>(m_currentLayer)]; }
    const LayerData& currentData() const { return m_layers[static_cast<int>(m_currentLayer)]; }
};

} // namespace fst
