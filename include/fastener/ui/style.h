#pragma once

#include "fastener/core/types.h"

namespace fst {

//=============================================================================
// Style Properties - Per-widget styling
//=============================================================================
struct Style {
    // Size
    float width = 0.0f;       // 0 = auto
    float height = 0.0f;      // 0 = auto
    float minWidth = 0.0f;
    float minHeight = 0.0f;
    float x = -1.0f;          // Absolute position (if layout not used, -1 = auto)
    float y = -1.0f;
    float maxWidth = 0.0f;    // 0 = no limit
    float maxHeight = 0.0f;   // 0 = no limit
    
    // Spacing
    Vec4 padding = Vec4(0.0f);    // top, right, bottom, left
    Vec4 margin = Vec4(0.0f);
    
    // Flex
    float flexGrow = 0.0f;
    float flexShrink = 1.0f;
    
    // Colors (Color::transparent() = use theme)
    Color backgroundColor = Color::transparent();
    Color textColor = Color::transparent();
    Color borderColor = Color::transparent();
    
    // Border
    float borderWidth = 0.0f;
    float borderRadius = 0.0f;
    
    // Shadow
    bool hasShadow = false;
    float shadowSize = 0.0f;
    Color shadowColor = Color::transparent();
    
    // Alignment
    Alignment horizontalAlign = Alignment::Start;
    Alignment verticalAlign = Alignment::Start;
    
    // Static constructors for common styles
    static Style fixed(float width, float height) {
        Style s;
        s.width = width;
        s.height = height;
        return s;
    }
    
    static Style flexible(float grow = 1.0f) {
        Style s;
        s.flexGrow = grow;
        return s;
    }
    
    static Style padded(float all) {
        Style s;
        s.padding = Vec4(all);
        return s;
    }
    
    static Style padded(float vertical, float horizontal) {
        Style s;
        s.padding = Vec4(vertical, horizontal, vertical, horizontal);
        return s;
    }
    
    // Chainable modifiers
    Style& withWidth(float w) { width = w; return *this; }
    Style& withHeight(float h) { height = h; return *this; }
    Style& withSize(float w, float h) { width = w; height = h; return *this; }
    Style& withPos(float _x, float _y) { x = _x; y = _y; return *this; }
    Style& withPadding(float all) { padding = Vec4(all); return *this; }
    Style& withPadding(float v, float h) { padding = Vec4(v, h, v, h); return *this; }
    Style& withMargin(float all) { margin = Vec4(all); return *this; }
    Style& withMargin(float v, float h) { margin = Vec4(v, h, v, h); return *this; }
    Style& withBackground(Color c) { backgroundColor = c; return *this; }
    Style& withTextColor(Color c) { textColor = c; return *this; }
    Style& withBorder(float w, Color c = Color::transparent()) { 
        borderWidth = w; 
        borderColor = c; 
        return *this; 
    }
    Style& withBorderRadius(float r) { borderRadius = r; return *this; }
    Style& withShadow(float size = 8.0f, Color color = Color(0, 0, 0, 80)) {
        hasShadow = true;
        shadowSize = size;
        shadowColor = color;
        return *this;
    }
    Style& withFlex(float grow = 1.0f, float shrink = 1.0f) {
        flexGrow = grow;
        flexShrink = shrink;
        return *this;
    }
    Style& withAlignment(Alignment h, Alignment v) {
        horizontalAlign = h;
        verticalAlign = v;
        return *this;
    }
};

} // namespace fst
