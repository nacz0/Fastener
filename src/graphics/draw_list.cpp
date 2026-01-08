#include "fastener/graphics/draw_list.h"
#include "fastener/graphics/texture.h"
#include "fastener/graphics/font.h"
#include "fastener/core/constants.h"
#include <cmath>
#include <cstring>

namespace fst {

DrawList::DrawList() {
    for (int i = 0; i < static_cast<int>(Layer::Count); ++i) {
        m_layers[i].vertices.reserve(constants::DEFAULT_VERTEX_RESERVE / static_cast<int>(Layer::Count));
        m_layers[i].indices.reserve(constants::DEFAULT_INDEX_RESERVE / static_cast<int>(Layer::Count));
        m_layers[i].commands.reserve(constants::DEFAULT_COMMAND_RESERVE / static_cast<int>(Layer::Count));
    }
}

DrawList::~DrawList() = default;

void DrawList::clear() {
    for (int i = 0; i < static_cast<int>(Layer::Count); ++i) {
        m_layers[i].vertices.clear();
        m_layers[i].indices.clear();
        m_layers[i].commands.clear();
        m_layers[i].clipRectStack.clear();
        m_layers[i].currentTexture = 0;
    }
    m_currentLayer = Layer::Default;
    m_mergedVertices.clear();
    m_mergedIndices.clear();
    m_mergedCommands.clear();
}

void DrawList::setLayer(Layer layer) {
    m_currentLayer = layer;
}

DrawList::Layer DrawList::currentLayer() const {
    return m_currentLayer;
}

void DrawList::mergeLayers() {
    m_mergedVertices.clear();
    m_mergedIndices.clear();
    m_mergedCommands.clear();

    uint32_t vertexOffset = 0;
    uint32_t indexOffset = 0;

    for (int i = 0; i < static_cast<int>(Layer::Count); ++i) {
        auto& layer = m_layers[i];
        if (layer.commands.empty()) continue;

        // Copy vertices
        m_mergedVertices.insert(m_mergedVertices.end(), layer.vertices.begin(), layer.vertices.end());

        // Copy indices with offset
        for (auto idx : layer.indices) {
            m_mergedIndices.push_back(idx + vertexOffset);
        }

        // Copy commands with offset
        for (auto cmd : layer.commands) {
            cmd.indexOffset += indexOffset;
            m_mergedCommands.push_back(cmd);
        }

        vertexOffset += static_cast<uint32_t>(layer.vertices.size());
        indexOffset += static_cast<uint32_t>(layer.indices.size());
    }
}

void DrawList::pushClipRect(const Rect& rect) {
    auto& data = currentData();
    if (data.clipRectStack.empty()) {
        data.clipRectStack.push_back(rect);
    } else {
        data.clipRectStack.push_back(rect.clipped(data.clipRectStack.back()));
    }
}

void DrawList::pushClipRectFullScreen(const Vec2& screenSize) {
    currentData().clipRectStack.push_back(Rect(0, 0, screenSize.x, screenSize.y));
}

void DrawList::popClipRect() {
    auto& data = currentData();
    if (!data.clipRectStack.empty()) {
        data.clipRectStack.pop_back();
    }
}

Rect DrawList::currentClipRect() const {
    auto& data = currentData();
    if (data.clipRectStack.empty()) {
        return Rect(0, 0, 10000, 10000);
    }
    return data.clipRectStack.back();
}

void DrawList::setTexture(uint32_t textureId) {
    auto& data = currentData();
    if (data.currentTexture != textureId) {
        data.currentTexture = textureId;
    }
}

void DrawList::updateCommand() {
    auto& data = currentData();
    if (data.commands.empty() || 
        data.commands.back().textureId != data.currentTexture ||
        data.commands.back().clipRect != currentClipRect()) {
        
        DrawCommand cmd;
        cmd.textureId = data.currentTexture;
        cmd.indexOffset = static_cast<uint32_t>(data.indices.size());
        cmd.indexCount = 0;
        cmd.clipRect = currentClipRect();
        data.commands.push_back(cmd);
    }
}

void DrawList::addVertex(const Vec2& pos, const Vec2& uv, Color color) {
    DrawVertex v;
    v.pos = pos;
    v.uv = uv;
    v.color = color.toABGR(); // Use common ABGR helper (implied to be what we want)
    currentData().vertices.push_back(v);
}

void DrawList::addIndex(uint32_t idx) {
    auto& data = currentData();
    data.indices.push_back(idx);
    if (!data.commands.empty()) {
        data.commands.back().indexCount++;
    }
}

void DrawList::addQuad(const Vec2& p0, const Vec2& p1, const Vec2& p2, const Vec2& p3,
                       const Vec2& uv0, const Vec2& uv1, const Vec2& uv2, const Vec2& uv3,
                       Color color) {
    updateCommand();
    
    uint32_t idx = static_cast<uint32_t>(currentData().vertices.size());
    
    addVertex(p0, uv0, color);
    addVertex(p1, uv1, color);
    addVertex(p2, uv2, color);
    addVertex(p3, uv3, color);
    
    addIndex(idx + 0);
    addIndex(idx + 1);
    addIndex(idx + 2);
    addIndex(idx + 0);
    addIndex(idx + 2);
    addIndex(idx + 3);
}

void DrawList::addQuadFilled(const Rect& rect, Color color) {
    setTexture(0);
    addQuad(
        rect.topLeft(), rect.topRight(), rect.bottomRight(), rect.bottomLeft(),
        {0, 0}, {1, 0}, {1, 1}, {0, 1},
        color
    );
}

void DrawList::addRectFilled(const Rect& rect, Color color, float rounding) {
    setTexture(0);
    if (rounding <= 0.0f) {
        addQuadFilled(rect, color);
    } else {
        primRectFilled(rect, color, rounding);
    }
}

void DrawList::addRect(const Rect& rect, Color color, float rounding) {
    setTexture(0);
    if (rounding <= 0.0f) {
        float thickness = 1.0f;
        addRectFilled(Rect(rect.x(), rect.y(), rect.width(), thickness), color);
        addRectFilled(Rect(rect.x(), rect.bottom() - thickness, rect.width(), thickness), color);
        addRectFilled(Rect(rect.x(), rect.y(), thickness, rect.height()), color);
        addRectFilled(Rect(rect.right() - thickness, rect.y(), thickness, rect.height()), color);
    } else {
        primRect(rect, color, rounding);
    }
}

void DrawList::addRectFilledMultiColor(const Rect& rect, Color topLeft, Color topRight,
                                        Color bottomRight, Color bottomLeft) {
    setTexture(0);
    updateCommand();
    
    uint32_t idx = static_cast<uint32_t>(currentData().vertices.size());
    
    addVertex(rect.topLeft(), {0, 0}, topLeft);
    addVertex(rect.topRight(), {1, 0}, topRight);
    addVertex(rect.bottomRight(), {1, 1}, bottomRight);
    addVertex(rect.bottomLeft(), {0, 1}, bottomLeft);
    
    addIndex(idx + 0);
    addIndex(idx + 1);
    addIndex(idx + 2);
    addIndex(idx + 0);
    addIndex(idx + 2);
    addIndex(idx + 3);
}

void DrawList::primRectFilled(const Rect& rect, Color color, float rounding) {
    // Clamp rounding
    rounding = std::min(rounding, std::min(rect.width(), rect.height()) * 0.5f);
    
    if (rounding < 0.5f) {
        addQuadFilled(rect, color);
        return;
    }
    
    const int segments = 8;
    
    // Draw center rect
    addQuadFilled(Rect(rect.x() + rounding, rect.y(), 
                       rect.width() - rounding * 2, rect.height()), color);
    
    // Draw left rect
    addQuadFilled(Rect(rect.x(), rect.y() + rounding,
                       rounding, rect.height() - rounding * 2), color);
    
    // Draw right rect
    addQuadFilled(Rect(rect.right() - rounding, rect.y() + rounding,
                       rounding, rect.height() - rounding * 2), color);
    
    // Draw corners as triangle fans
    auto drawCorner = [&](Vec2 center, float startAngle) {
        updateCommand();
        uint32_t centerIdx = static_cast<uint32_t>(currentData().vertices.size());
        addVertex(center, {0.5f, 0.5f}, color);
        
        for (int i = 0; i <= segments; ++i) {
            float angle = startAngle + (3.14159f / 2.0f) * i / segments;
            Vec2 p = center + Vec2(std::cos(angle), std::sin(angle)) * rounding;
            addVertex(p, {0.5f, 0.5f}, color);
            
            if (i > 0) {
                addIndex(centerIdx);
                addIndex(centerIdx + i);
                addIndex(centerIdx + i + 1);
            }
        }
    };
    
    // Top-left corner
    drawCorner({rect.x() + rounding, rect.y() + rounding}, 3.14159f);
    // Top-right corner  
    drawCorner({rect.right() - rounding, rect.y() + rounding}, -3.14159f / 2);
    // Bottom-right corner
    drawCorner({rect.right() - rounding, rect.bottom() - rounding}, 0);
    // Bottom-left corner
    drawCorner({rect.x() + rounding, rect.bottom() - rounding}, 3.14159f / 2);
}

void DrawList::primRect(const Rect& rect, Color color, float rounding) {
    rounding = std::min(rounding, std::min(rect.width(), rect.height()) * 0.5f);
    
    float thickness = 1.0f;
    if (rounding < 0.5f) {
        addRectFilled(Rect(rect.x(), rect.y(), rect.width(), thickness), color);
        addRectFilled(Rect(rect.x(), rect.bottom() - thickness, rect.width(), thickness), color);
        addRectFilled(Rect(rect.x(), rect.y() + thickness, thickness, rect.height() - thickness * 2), color);
        addRectFilled(Rect(rect.right() - thickness, rect.y() + thickness, thickness, rect.height() - thickness * 2), color);
        return;
    }
    
    // Straight segments
    // Top
    addLine({rect.x() + rounding, rect.y() + thickness * 0.5f}, 
            {rect.right() - rounding, rect.y() + thickness * 0.5f}, color, thickness);
    // Bottom
    addLine({rect.x() + rounding, rect.bottom() - thickness * 0.5f}, 
            {rect.right() - rounding, rect.bottom() - thickness * 0.5f}, color, thickness);
    // Left
    addLine({rect.x() + thickness * 0.5f, rect.y() + rounding}, 
            {rect.x() + thickness * 0.5f, rect.bottom() - rounding}, color, thickness);
    // Right
    addLine({rect.right() - thickness * 0.5f, rect.y() + rounding}, 
            {rect.right() - thickness * 0.5f, rect.bottom() - rounding}, color, thickness);
            
    // Arcs for corners
    auto drawCornerArc = [&](Vec2 center, float startAngle) {
        const int segments = 8;
        float r = rounding - thickness * 0.5f;
        for (int i = 0; i < segments; ++i) {
            float a1 = startAngle + (3.14159265f / 2.0f) * (float)i / segments;
            float a2 = startAngle + (3.14159265f / 2.0f) * (float)(i + 1) / segments;
            addLine(center + Vec2(std::cos(a1), std::sin(a1)) * r, 
                    center + Vec2(std::cos(a2), std::sin(a2)) * r, color, thickness);
        }
    };
    
    const float PI = 3.14159265f;
    drawCornerArc({rect.x() + rounding, rect.y() + rounding}, PI);           // Top-left
    drawCornerArc({rect.right() - rounding, rect.y() + rounding}, -PI / 2.0f); // Top-right
    drawCornerArc({rect.right() - rounding, rect.bottom() - rounding}, 0.0f);   // Bottom-right
    drawCornerArc({rect.x() + rounding, rect.bottom() - rounding}, PI / 2.0f);  // Bottom-left
}

void DrawList::addLine(const Vec2& p1, const Vec2& p2, Color color, float thickness) {
    setTexture(0);
    Vec2 dir = (p2 - p1).normalized();
    Vec2 normal = {-dir.y, dir.x};
    Vec2 offset = normal * (thickness * 0.5f);
    
    addQuad(
        p1 - offset, p1 + offset, p2 + offset, p2 - offset,
        {0, 0}, {1, 0}, {1, 1}, {0, 1},
        color
    );
}

void DrawList::addCircle(const Vec2& center, float radius, Color color, int segments) {
    if (segments <= 0) {
        segments = std::max(12, static_cast<int>(radius * 0.5f));
    }
    
    float thickness = 1.0f;
    
    for (int i = 0; i < segments; ++i) {
        float a1 = 2.0f * 3.14159f * i / segments;
        float a2 = 2.0f * 3.14159f * (i + 1) / segments;
        
        Vec2 p1 = center + Vec2(std::cos(a1), std::sin(a1)) * radius;
        Vec2 p2 = center + Vec2(std::cos(a2), std::sin(a2)) * radius;
        
        addLine(p1, p2, color, thickness);
    }
}

void DrawList::addCircleFilled(const Vec2& center, float radius, Color color, int segments) {
    setTexture(0);
    if (segments <= 0) {
        segments = std::max(12, static_cast<int>(radius * 0.5f));
    }
    
    updateCommand();
    
    uint32_t centerIdx = static_cast<uint32_t>(currentData().vertices.size());
    addVertex(center, {0.5f, 0.5f}, color);
    
    for (int i = 0; i <= segments; ++i) {
        float angle = 2.0f * 3.14159f * i / segments;
        Vec2 p = center + Vec2(std::cos(angle), std::sin(angle)) * radius;
        addVertex(p, {0.5f, 0.5f}, color);
        
        if (i > 0) {
            addIndex(centerIdx);
            addIndex(centerIdx + i);
            addIndex(centerIdx + i + 1);
        }
    }
}

void DrawList::addTriangle(const Vec2& p1, const Vec2& p2, const Vec2& p3, Color color) {
    addLine(p1, p2, color);
    addLine(p2, p3, color);
    addLine(p3, p1, color);
}

void DrawList::addTriangleFilled(const Vec2& p1, const Vec2& p2, const Vec2& p3, Color color) {
    setTexture(0);
    updateCommand();
    
    uint32_t idx = static_cast<uint32_t>(currentData().vertices.size());
    
    addVertex(p1, {0, 0}, color);
    addVertex(p2, {0.5f, 1}, color);
    addVertex(p3, {1, 0}, color);
    
    addIndex(idx + 0);
    addIndex(idx + 1);
    addIndex(idx + 2);
}

void DrawList::addText(const Font* font, const Vec2& pos, const std::string& text, Color color) {
    addText(font, pos, text.c_str(), text.c_str() + text.size(), color);
}

void DrawList::addText(const Font* font, const Vec2& pos, const char* text, const char* textEnd, Color color) {
    if (!font || !text || !font->isValid()) return;
    
    if (!textEnd) {
        textEnd = text + std::strlen(text);
    }
    
    // Use font atlas texture
    setTexture(font->atlasTexture().handle());
    
    float x = pos.x;
    float y = pos.y;
    uint32_t prevCodepoint = 0;
    
    Font* mutableFont = const_cast<Font*>(font);  // For lazy glyph loading
    
    const char* s = text;
    while (s < textEnd) {
        // Decode UTF-8
        uint32_t codepoint;
        unsigned char c = *s++;
        
        if (c < 0x80) {
            codepoint = c;
        } else if (c < 0xE0) {
            codepoint = (c & 0x1F) << 6;
            if (s < textEnd) codepoint |= (*s++ & 0x3F);
        } else if (c < 0xF0) {
            codepoint = (c & 0x0F) << 12;
            if (s < textEnd) codepoint |= (*s++ & 0x3F) << 6;
            if (s < textEnd) codepoint |= (*s++ & 0x3F);
        } else {
            codepoint = (c & 0x07) << 18;
            if (s < textEnd) codepoint |= (*s++ & 0x3F) << 12;
            if (s < textEnd) codepoint |= (*s++ & 0x3F) << 6;
            if (s < textEnd) codepoint |= (*s++ & 0x3F);
        }
        
        if (codepoint == '\n') {
            x = pos.x;
            y += font->lineHeight();
            prevCodepoint = 0;
            continue;
        }
        
        if (codepoint == '\r') {
            prevCodepoint = 0;
            continue;
        }
        
        const GlyphInfo* glyph = mutableFont->getGlyph(codepoint);
        if (!glyph) continue;
        
        // Apply kerning
        if (prevCodepoint != 0) {
            x += font->getKerning(prevCodepoint, codepoint);
        }
        
        // Draw glyph quad
        if (glyph->atlasW > 0 && glyph->atlasH > 0) {
            Rect glyphRect(
                x + glyph->xOffset,
                y + glyph->yOffset,
                static_cast<float>(glyph->atlasW),
                static_cast<float>(glyph->atlasH)
            );
            
            addQuad(
                glyphRect.topLeft(), glyphRect.topRight(), 
                glyphRect.bottomRight(), glyphRect.bottomLeft(),
                {glyph->uvX0, glyph->uvY0}, {glyph->uvX1, glyph->uvY0},
                {glyph->uvX1, glyph->uvY1}, {glyph->uvX0, glyph->uvY1},
                color
            );
        }
        
        x += glyph->xAdvance;
        prevCodepoint = codepoint;
    }
}

void DrawList::addImage(const Texture* texture, const Rect& rect, Color tint) {
    addImage(texture, rect, {0, 0}, {1, 1}, tint);
}

void DrawList::addImage(const Texture* texture, const Rect& rect, 
                        const Vec2& uv0, const Vec2& uv1, Color tint) {
    if (!texture || !texture->isValid()) return;
    
    setTexture(texture->handle());
    
    addQuad(
        rect.topLeft(), rect.topRight(), rect.bottomRight(), rect.bottomLeft(),
        {uv0.x, uv0.y}, {uv1.x, uv0.y}, {uv1.x, uv1.y}, {uv0.x, uv1.y},
        tint
    );
}

void DrawList::addImageRounded(const Texture* texture, const Rect& rect, 
                                float rounding, Color tint) {
    // TODO: Implement rounded image with masking
    addImage(texture, rect, tint);
}

void DrawList::addShadow(const Rect& rect, Color color, float size, float rounding) {
    // Draw gradient shadow using multiple layers
    int layers = static_cast<int>(size / 2);
    if (layers < 1) layers = 1;
    
    for (int i = layers; i > 0; --i) {
        float t = static_cast<float>(i) / layers;
        float expand = size * t;
        uint8_t alpha = static_cast<uint8_t>(color.a * (1.0f - t) * 0.5f);
        
        Rect shadowRect = rect.expanded(expand);
        addRectFilled(shadowRect, color.withAlpha(alpha), rounding + expand);
    }
}

} // namespace fst
