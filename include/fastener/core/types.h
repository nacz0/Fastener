#pragma once

#include <cstdint>
#include <cmath>
#include <algorithm>
#include <string>

namespace fst {

// Forward declarations
struct Vec2;
struct Vec4;
struct Rect;
struct Color;

//=============================================================================
// Vec2 - 2D Vector
//=============================================================================
struct Vec2 {
    float x = 0.0f;
    float y = 0.0f;
    
    constexpr Vec2() = default;
    constexpr Vec2(float x, float y) : x(x), y(y) {}
    constexpr explicit Vec2(float v) : x(v), y(v) {}
    
    // Operators
    constexpr Vec2 operator+(const Vec2& other) const { return {x + other.x, y + other.y}; }
    constexpr Vec2 operator-(const Vec2& other) const { return {x - other.x, y - other.y}; }
    constexpr Vec2 operator*(float scalar) const { return {x * scalar, y * scalar}; }
    constexpr Vec2 operator/(float scalar) const { return {x / scalar, y / scalar}; }
    constexpr Vec2 operator*(const Vec2& other) const { return {x * other.x, y * other.y}; }
    constexpr Vec2 operator/(const Vec2& other) const { return {x / other.x, y / other.y}; }
    
    Vec2& operator+=(const Vec2& other) { x += other.x; y += other.y; return *this; }
    Vec2& operator-=(const Vec2& other) { x -= other.x; y -= other.y; return *this; }
    Vec2& operator*=(float scalar) { x *= scalar; y *= scalar; return *this; }
    Vec2& operator/=(float scalar) { x /= scalar; y /= scalar; return *this; }
    
    constexpr bool operator==(const Vec2& other) const { return x == other.x && y == other.y; }
    constexpr bool operator!=(const Vec2& other) const { return !(*this == other); }
    
    // Utility
    float length() const { return std::sqrt(x * x + y * y); }
    float lengthSquared() const { return x * x + y * y; }
    Vec2 normalized() const { 
        float len = length();
        return len > 0.0f ? *this / len : Vec2(0.0f);
    }
    constexpr float dot(const Vec2& other) const { return x * other.x + y * other.y; }
    
    // Static helpers
    static constexpr Vec2 zero() { return {0.0f, 0.0f}; }
    static constexpr Vec2 one() { return {1.0f, 1.0f}; }
};

inline constexpr Vec2 operator*(float scalar, const Vec2& v) { return v * scalar; }

//=============================================================================
// Vec4 - 4D Vector (also used for padding/margins)
//=============================================================================
struct Vec4 {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    float w = 0.0f;
    
    constexpr Vec4() = default;
    constexpr Vec4(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}
    constexpr explicit Vec4(float v) : x(v), y(v), z(v), w(v) {}
    
    // For padding/margins: top, right, bottom, left
    constexpr float top() const { return x; }
    constexpr float right() const { return y; }
    constexpr float bottom() const { return z; }
    constexpr float left() const { return w; }
    
    constexpr Vec2 topLeft() const { return {w, x}; }
    constexpr Vec2 bottomRight() const { return {y, z}; }
    constexpr Vec2 size() const { return {w + y, x + z}; }
};

//=============================================================================
// Rect - Rectangle (position + size)
//=============================================================================
struct Rect {
    Vec2 pos;   // Top-left position
    Vec2 size;  // Width and height
    
    constexpr Rect() = default;
    constexpr Rect(float x, float y, float w, float h) : pos(x, y), size(w, h) {}
    constexpr Rect(const Vec2& pos, const Vec2& size) : pos(pos), size(size) {}
    
    // Accessors
    constexpr float x() const { return pos.x; }
    constexpr float y() const { return pos.y; }
    constexpr float width() const { return size.x; }
    constexpr float height() const { return size.y; }
    
    constexpr float left() const { return pos.x; }
    constexpr float top() const { return pos.y; }
    constexpr float right() const { return pos.x + size.x; }
    constexpr float bottom() const { return pos.y + size.y; }
    
    constexpr Vec2 topLeft() const { return pos; }
    constexpr Vec2 topRight() const { return {right(), top()}; }
    constexpr Vec2 bottomLeft() const { return {left(), bottom()}; }
    constexpr Vec2 bottomRight() const { return {right(), bottom()}; }
    constexpr Vec2 center() const { return pos + size * 0.5f; }
    
    // Hit testing
    constexpr bool contains(const Vec2& point) const {
        return point.x >= left() && point.x < right() &&
               point.y >= top() && point.y < bottom();
    }
    
    constexpr bool contains(float px, float py) const {
        return contains(Vec2(px, py));
    }
    
    constexpr bool intersects(const Rect& other) const {
        return left() < other.right() && right() > other.left() &&
               top() < other.bottom() && bottom() > other.top();
    }
    
    // Operations
    constexpr Rect expanded(float amount) const {
        return {pos.x - amount, pos.y - amount, 
                size.x + amount * 2, size.y + amount * 2};
    }
    
    constexpr Rect shrunk(float amount) const {
        return expanded(-amount);
    }
    
    constexpr Rect expanded(const Vec4& padding) const {
        return {pos.x - padding.left(), pos.y - padding.top(),
                size.x + padding.left() + padding.right(),
                size.y + padding.top() + padding.bottom()};
    }
    
    constexpr Rect shrunk(const Vec4& padding) const {
        return {pos.x + padding.left(), pos.y + padding.top(),
                size.x - padding.left() - padding.right(),
                size.y - padding.top() - padding.bottom()};
    }
    
    constexpr Rect translated(const Vec2& offset) const {
        return {pos + offset, size};
    }
    
    // Intersection (clip)
    Rect clipped(const Rect& clipRect) const {
        float newLeft = std::max(left(), clipRect.left());
        float newTop = std::max(top(), clipRect.top());
        float newRight = std::min(right(), clipRect.right());
        float newBottom = std::min(bottom(), clipRect.bottom());
        
        if (newRight <= newLeft || newBottom <= newTop) {
            return Rect(newLeft, newTop, 0, 0);
        }
        
        return Rect(newLeft, newTop, newRight - newLeft, newBottom - newTop);
    }
    
    // Comparison
    constexpr bool operator==(const Rect& other) const {
        return pos == other.pos && size == other.size;
    }
    
    constexpr bool operator!=(const Rect& other) const {
        return !(*this == other);
    }
    
    // Static helpers
    static constexpr Rect zero() { return {}; }
    static constexpr Rect fromMinMax(const Vec2& min, const Vec2& max) {
        return {min, max - min};
    }
};

//=============================================================================
// Color - RGBA Color (0-255 per channel)
//=============================================================================
struct Color {
    uint8_t r = 255;
    uint8_t g = 255;
    uint8_t b = 255;
    uint8_t a = 255;
    
    constexpr Color() = default;
    constexpr Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) 
        : r(r), g(g), b(b), a(a) {}
    
    // From float (0-1 range)
    static Color fromFloat(float r, float g, float b, float a = 1.0f) {
        return {
            static_cast<uint8_t>(std::clamp(r, 0.0f, 1.0f) * 255.0f),
            static_cast<uint8_t>(std::clamp(g, 0.0f, 1.0f) * 255.0f),
            static_cast<uint8_t>(std::clamp(b, 0.0f, 1.0f) * 255.0f),
            static_cast<uint8_t>(std::clamp(a, 0.0f, 1.0f) * 255.0f)
        };
    }
    
    // From hex (0xRRGGBB or 0xRRGGBBAA)
    static constexpr Color fromHex(uint32_t hex, bool hasAlpha = false) {
        if (hasAlpha) {
            return {
                static_cast<uint8_t>((hex >> 24) & 0xFF),
                static_cast<uint8_t>((hex >> 16) & 0xFF),
                static_cast<uint8_t>((hex >> 8) & 0xFF),
                static_cast<uint8_t>(hex & 0xFF)
            };
        } else {
            return {
                static_cast<uint8_t>((hex >> 16) & 0xFF),
                static_cast<uint8_t>((hex >> 8) & 0xFF),
                static_cast<uint8_t>(hex & 0xFF),
                255
            };
        }
    }
    
    // From HSL
    static Color fromHSL(float h, float s, float l, float a = 1.0f);
    
    // HSV conversion
    static Color fromHSV(float h, float s, float v, float a = 1.0f);
    void toHSV(float& h, float& s, float& v) const;
    
    // To float (0-1 range)
    float rf() const { return r / 255.0f; }
    float gf() const { return g / 255.0f; }
    float bf() const { return b / 255.0f; }
    float af() const { return a / 255.0f; }
    
    // Pack to uint32 (RGBA)
    constexpr uint32_t toRGBA() const {
        return (static_cast<uint32_t>(r) << 24) |
               (static_cast<uint32_t>(g) << 16) |
               (static_cast<uint32_t>(b) << 8) |
               static_cast<uint32_t>(a);
    }
    
    // Pack to uint32 (ABGR) - common for OpenGL
    constexpr uint32_t toABGR() const {
        return (static_cast<uint32_t>(a) << 24) |
               (static_cast<uint32_t>(b) << 16) |
               (static_cast<uint32_t>(g) << 8) |
               static_cast<uint32_t>(r);
    }
    
    // Operations
    Color withAlpha(uint8_t newAlpha) const {
        return {r, g, b, newAlpha};
    }
    
    Color withAlpha(float alphaMultiplier) const {
        return {r, g, b, static_cast<uint8_t>(a * alphaMultiplier)};
    }
    
    Color lighter(float amount = 0.1f) const;
    Color darker(float amount = 0.1f) const;
    
    static Color lerp(const Color& a, const Color& b, float t);
    
    // Comparison
    constexpr bool operator==(const Color& other) const {
        return r == other.r && g == other.g && b == other.b && a == other.a;
    }
    
    constexpr bool operator!=(const Color& other) const {
        return !(*this == other);
    }
    
    // Common colors
    static constexpr Color white() { return {255, 255, 255, 255}; }
    static constexpr Color black() { return {0, 0, 0, 255}; }
    static constexpr Color transparent() { return {0, 0, 0, 0}; }
    static constexpr Color red() { return {255, 0, 0, 255}; }
    static constexpr Color green() { return {0, 255, 0, 255}; }
    static constexpr Color blue() { return {0, 0, 255, 255}; }
    static constexpr Color yellow() { return {255, 255, 0, 255}; }
    static constexpr Color cyan() { return {0, 255, 255, 255}; }
    static constexpr Color magenta() { return {255, 0, 255, 255}; }
    static constexpr Color gray() { return {128, 128, 128, 255}; }
};

//=============================================================================
// ID System - Unique widget identifiers
//=============================================================================
using WidgetId = uint64_t;

constexpr WidgetId INVALID_WIDGET_ID = 0;

// Generate ID from string (compile-time if possible)
constexpr WidgetId hashString(const char* str) {
    WidgetId hash = 14695981039346656037ULL;
    while (*str) {
        hash ^= static_cast<WidgetId>(*str++);
        hash *= 1099511628211ULL;
    }
    return hash;
}

inline WidgetId hashString(const std::string& str) {
    return hashString(str.c_str());
}

// Combine IDs (for hierarchical widgets)
constexpr WidgetId combineIds(WidgetId parent, WidgetId child) {
    return parent ^ (child * 1099511628211ULL);
}

//=============================================================================
// Enums
//=============================================================================
enum class Alignment : uint8_t {
    Start,      // Left or Top
    Center,
    End,        // Right or Bottom
    Stretch     // Fill available space
};

enum class Direction : uint8_t {
    Horizontal,
    Vertical
};

enum class Cursor : uint8_t {
    Arrow,
    IBeam,
    Hand,
    ResizeH,
    ResizeV,
    ResizeNESW,
    ResizeNWSE,
    Move,
    NotAllowed,
    Wait
};

} // namespace fst
