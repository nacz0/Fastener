#pragma once

#include "fastener/core/types.h"
#include "fastener/graphics/texture.h"
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

namespace fst {

//=============================================================================
// Glyph Info
//=============================================================================
struct GlyphInfo {
    uint32_t codepoint = 0;
    
    // Position in atlas (pixels)
    int atlasX = 0;
    int atlasY = 0;
    int atlasW = 0;
    int atlasH = 0;
    
    // UV coordinates (normalized 0-1)
    float uvX0 = 0.0f;
    float uvY0 = 0.0f;
    float uvX1 = 0.0f;
    float uvY1 = 0.0f;
    
    // Layout info (pixels)
    float xOffset = 0.0f;
    float yOffset = 0.0f;
    float xAdvance = 0.0f;
};

//=============================================================================
// Font
//=============================================================================
class Font {
public:
    Font();
    ~Font();
    
    // Non-copyable, movable
    Font(const Font&) = delete;
    Font& operator=(const Font&) = delete;
    Font(Font&& other) noexcept;
    Font& operator=(Font&& other) noexcept;
    
    // Loading
    bool loadFromFile(const std::string& path, float size);
    bool loadFromMemory(const void* data, size_t dataSize, float size);
    void destroy();
    
    // Properties
    float size() const { return m_size; }
    float lineHeight() const { return m_lineHeight; }
    float ascent() const { return m_ascent; }
    float descent() const { return m_descent; }
    bool isValid() const { return m_isValid; }
    
    // Glyph access
    const GlyphInfo* getGlyph(uint32_t codepoint);
    float getKerning(uint32_t cp1, uint32_t cp2) const;
    
    // Atlas texture
    const Texture& atlasTexture() const { return m_atlas; }
    
    // Text measurement
    Vec2 measureText(std::string_view text) const;
    
    // Character iteration helper
    struct CharacterInfo {
        uint32_t codepoint;
        const GlyphInfo* glyph;
        float x;
        float y;
    };
    
private:
    // STB TrueType data
    std::vector<uint8_t> m_fontData;
    void* m_fontInfo = nullptr;  // stbtt_fontinfo*
    
    // Atlas
    Texture m_atlas;
    int m_atlasWidth = 1024;
    int m_atlasHeight = 1024;
    std::vector<uint8_t> m_atlasData;
    
    // Glyphs
    std::unordered_map<uint32_t, GlyphInfo> m_glyphs;
    
    // Metrics
    float m_size = 0.0f;
    float m_scale = 0.0f;
    float m_lineHeight = 0.0f;
    float m_ascent = 0.0f;
    float m_descent = 0.0f;
    
    bool m_isValid = false;
    
    // Atlas packing state
    int m_packX = 0;
    int m_packY = 0;
    int m_packRowHeight = 0;
    
    bool bakeGlyph(uint32_t codepoint);
    void updateAtlasTexture();
};

} // namespace fst
