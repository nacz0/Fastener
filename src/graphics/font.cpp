#define STB_TRUETYPE_IMPLEMENTATION

#include "stb_truetype.h"
#include "fastener/graphics/font.h"
#include "fastener/core/log.h"
#include <fstream>
#include <cstring>
#include <cmath>

namespace fst {

Font::Font() = default;

Font::~Font() {
    destroy();
}

Font::Font(Font&& other) noexcept 
    : m_fontData(std::move(other.m_fontData))
    , m_fontInfo(other.m_fontInfo)
    , m_atlas(std::move(other.m_atlas))
    , m_atlasWidth(other.m_atlasWidth)
    , m_atlasHeight(other.m_atlasHeight)
    , m_atlasData(std::move(other.m_atlasData))
    , m_glyphs(std::move(other.m_glyphs))
    , m_size(other.m_size)
    , m_scale(other.m_scale)
    , m_lineHeight(other.m_lineHeight)
    , m_ascent(other.m_ascent)
    , m_descent(other.m_descent)
    , m_isValid(other.m_isValid)
    , m_packX(other.m_packX)
    , m_packY(other.m_packY)
    , m_packRowHeight(other.m_packRowHeight)
{
    other.m_fontInfo = nullptr;
    other.m_isValid = false;
}

Font& Font::operator=(Font&& other) noexcept {
    if (this != &other) {
        destroy();
        m_fontData = std::move(other.m_fontData);
        m_fontInfo = other.m_fontInfo;
        m_atlas = std::move(other.m_atlas);
        m_atlasWidth = other.m_atlasWidth;
        m_atlasHeight = other.m_atlasHeight;
        m_atlasData = std::move(other.m_atlasData);
        m_glyphs = std::move(other.m_glyphs);
        m_size = other.m_size;
        m_scale = other.m_scale;
        m_lineHeight = other.m_lineHeight;
        m_ascent = other.m_ascent;
        m_descent = other.m_descent;
        m_isValid = other.m_isValid;
        m_packX = other.m_packX;
        m_packY = other.m_packY;
        m_packRowHeight = other.m_packRowHeight;
        
        other.m_fontInfo = nullptr;
        other.m_isValid = false;
    }
    return *this;
}

bool Font::loadFromFile(const std::string& path, float size) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        FST_LOG_ERROR("Failed to open font file");
        return false;
    }
    
    size_t fileSize = file.tellg();
    file.seekg(0, std::ios::beg);
    
    std::vector<uint8_t> data(fileSize);
    file.read(reinterpret_cast<char*>(data.data()), fileSize);
    
    return loadFromMemory(data.data(), data.size(), size);
}

bool Font::loadFromMemory(const void* data, size_t dataSize, float size) {
    destroy();
    
    // Copy font data
    m_fontData.resize(dataSize);
    std::memcpy(m_fontData.data(), data, dataSize);
    
    // Initialize font info
    m_fontInfo = new stbtt_fontinfo;
    stbtt_fontinfo* info = static_cast<stbtt_fontinfo*>(m_fontInfo);
    
    if (!stbtt_InitFont(info, m_fontData.data(), 0)) {
        FST_LOG_ERROR("Failed to initialize font - invalid font data");
        delete info;
        m_fontInfo = nullptr;
        m_fontData.clear();
        return false;
    }
    
    m_size = size;
    m_scale = stbtt_ScaleForPixelHeight(info, size);
    
    // Get font metrics
    int ascent, descent, lineGap;
    stbtt_GetFontVMetrics(info, &ascent, &descent, &lineGap);
    
    m_ascent = ascent * m_scale;
    m_descent = descent * m_scale;
    m_lineHeight = (ascent - descent + lineGap) * m_scale;
    
    // Initialize atlas
    m_atlasData.resize(m_atlasWidth * m_atlasHeight);
    std::memset(m_atlasData.data(), 0, m_atlasData.size());
    
    m_packX = 1;
    m_packY = 1;
    m_packRowHeight = 0;
    
    // Pre-bake ASCII characters
    for (uint32_t c = 32; c < 127; ++c) {
        bakeGlyph(c);
    }
    
    // Convert grayscale atlas to RGBA (white text with alpha from grayscale)
    std::vector<uint8_t> rgbaData(m_atlasWidth * m_atlasHeight * 4);
    for (int i = 0; i < m_atlasWidth * m_atlasHeight; ++i) {
        rgbaData[i * 4 + 0] = 255;              // R = white
        rgbaData[i * 4 + 1] = 255;              // G = white
        rgbaData[i * 4 + 2] = 255;              // B = white
        rgbaData[i * 4 + 3] = m_atlasData[i];   // A = grayscale value
    }
    
    // Create atlas texture as RGBA
    m_atlas.create(m_atlasWidth, m_atlasHeight, rgbaData.data(), 4);
    
    m_isValid = true;
    return true;
}

void Font::destroy() {
    if (m_fontInfo) {
        delete static_cast<stbtt_fontinfo*>(m_fontInfo);
        m_fontInfo = nullptr;
    }
    m_fontData.clear();
    m_atlas.destroy();
    m_atlasData.clear();
    m_glyphs.clear();
    m_isValid = false;
}

const GlyphInfo* Font::getGlyph(uint32_t codepoint) {
    auto it = m_glyphs.find(codepoint);
    if (it != m_glyphs.end()) {
        return &it->second;
    }
    
    // Try to bake the glyph
    if (bakeGlyph(codepoint)) {
        updateAtlasTexture();
        return &m_glyphs[codepoint];
    }
    
    // Return space as fallback
    it = m_glyphs.find(' ');
    if (it != m_glyphs.end()) {
        return &it->second;
    }
    
    return nullptr;
}

float Font::getKerning(uint32_t cp1, uint32_t cp2) const {
    if (!m_fontInfo) return 0.0f;
    
    stbtt_fontinfo* info = static_cast<stbtt_fontinfo*>(m_fontInfo);
    return stbtt_GetCodepointKernAdvance(info, cp1, cp2) * m_scale;
}

bool Font::bakeGlyph(uint32_t codepoint) {
    if (!m_fontInfo) return false;
    
    stbtt_fontinfo* info = static_cast<stbtt_fontinfo*>(m_fontInfo);
    
    // Get glyph metrics
    int glyphIndex = stbtt_FindGlyphIndex(info, codepoint);
    if (glyphIndex == 0 && codepoint != 0) {
        // Unknown glyph
        return false;
    }
    
    int advanceWidth, leftSideBearing;
    stbtt_GetGlyphHMetrics(info, glyphIndex, &advanceWidth, &leftSideBearing);
    
    int x0, y0, x1, y1;
    stbtt_GetGlyphBitmapBox(info, glyphIndex, m_scale, m_scale, &x0, &y0, &x1, &y1);
    
    int glyphWidth = x1 - x0;
    int glyphHeight = y1 - y0;
    
    // Check if we need to move to next row
    if (m_packX + glyphWidth + 1 >= m_atlasWidth) {
        m_packX = 1;
        m_packY += m_packRowHeight + 1;
        m_packRowHeight = 0;
    }
    
    // Check if atlas is full
    if (m_packY + glyphHeight + 1 >= m_atlasHeight) {
        // TODO: Expand atlas or use multiple pages
        return false;
    }
    
    // Render glyph to atlas
    if (glyphWidth > 0 && glyphHeight > 0) {
        stbtt_MakeGlyphBitmap(
            info,
            m_atlasData.data() + m_packY * m_atlasWidth + m_packX,
            glyphWidth, glyphHeight,
            m_atlasWidth,
            m_scale, m_scale,
            glyphIndex
        );
    }
    
    // Create glyph info
    GlyphInfo glyph;
    glyph.codepoint = codepoint;
    glyph.atlasX = m_packX;
    glyph.atlasY = m_packY;
    glyph.atlasW = glyphWidth;
    glyph.atlasH = glyphHeight;
    glyph.uvX0 = static_cast<float>(m_packX) / m_atlasWidth;
    glyph.uvY0 = static_cast<float>(m_packY) / m_atlasHeight;
    glyph.uvX1 = static_cast<float>(m_packX + glyphWidth) / m_atlasWidth;
    glyph.uvY1 = static_cast<float>(m_packY + glyphHeight) / m_atlasHeight;
    glyph.xOffset = static_cast<float>(x0);
    glyph.yOffset = static_cast<float>(y0) + m_ascent;
    glyph.xAdvance = advanceWidth * m_scale;
    
    m_glyphs[codepoint] = glyph;
    
    // Advance pack position
    m_packX += glyphWidth + 1;
    m_packRowHeight = std::max(m_packRowHeight, glyphHeight);
    
    return true;
}

void Font::updateAtlasTexture() {
    if (m_atlas.isValid()) {
        // Convert grayscale to RGBA for update
        std::vector<uint8_t> rgbaData(m_atlasWidth * m_atlasHeight * 4);
        for (int i = 0; i < m_atlasWidth * m_atlasHeight; ++i) {
            rgbaData[i * 4 + 0] = 255;
            rgbaData[i * 4 + 1] = 255;
            rgbaData[i * 4 + 2] = 255;
            rgbaData[i * 4 + 3] = m_atlasData[i];
        }
        m_atlas.update(0, 0, m_atlasWidth, m_atlasHeight, rgbaData.data());
    }
}

Vec2 Font::measureText(std::string_view text) const {
    if (text.empty() || !m_isValid) return Vec2::zero();
    
    const char* textStart = text.data();
    const char* textEnd = text.data() + text.size();
    
    float x = 0.0f;
    float maxX = 0.0f;
    float y = m_lineHeight;
    
    uint32_t prevCodepoint = 0;
    
    const char* s = textStart;
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
            maxX = std::max(maxX, x);
            x = 0.0f;
            y += m_lineHeight;
            prevCodepoint = 0;
            continue;
        }
        
        if (codepoint == '\r') {
            prevCodepoint = 0;
            continue;
        }
        
        // Get glyph info (need const_cast for lazy loading)
        auto it = m_glyphs.find(codepoint);
        if (it != m_glyphs.end()) {
            const GlyphInfo& glyph = it->second;
            
            // Apply kerning
            if (prevCodepoint != 0) {
                x += getKerning(prevCodepoint, codepoint);
            }
            
            x += glyph.xAdvance;
        }
        
        prevCodepoint = codepoint;
    }
    
    maxX = std::max(maxX, x);
    return {maxX, y};
}

} // namespace fst
