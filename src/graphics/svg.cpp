#include "fastener/graphics/svg.h"
#include "fastener/core/log.h"

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <unordered_map>

namespace fst {

namespace {

std::string toLower(std::string_view value) {
    std::string out;
    out.reserve(value.size());
    for (char c : value) {
        out.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    }
    return out;
}

std::string trimCopy(std::string_view value) {
    size_t start = 0;
    while (start < value.size() && std::isspace(static_cast<unsigned char>(value[start]))) {
        ++start;
    }
    size_t end = value.size();
    while (end > start && std::isspace(static_cast<unsigned char>(value[end - 1]))) {
        --end;
    }
    return std::string(value.substr(start, end - start));
}

float parseFloat(std::string_view value, float fallback = 0.0f) {
    std::string s = trimCopy(value);
    if (s.empty()) return fallback;
    char* end = nullptr;
    float v = std::strtof(s.c_str(), &end);
    if (end == s.c_str()) return fallback;
    return v;
}

float parseOpacity(std::string_view value, float fallback = 1.0f) {
    std::string s = trimCopy(value);
    if (s.empty()) return fallback;
    char* end = nullptr;
    float v = std::strtof(s.c_str(), &end);
    if (end == s.c_str()) return fallback;
    if (end && *end == '%') {
        v = v / 100.0f;
    }
    return std::clamp(v, 0.0f, 1.0f);
}

bool parseHexColor(std::string_view value, Color& out) {
    if (value.empty() || value[0] != '#') return false;
    std::string s = toLower(value.substr(1));
    if (s.size() == 3) {
        unsigned int r = 0, g = 0, b = 0;
        r = std::strtoul(s.substr(0, 1).c_str(), nullptr, 16);
        g = std::strtoul(s.substr(1, 1).c_str(), nullptr, 16);
        b = std::strtoul(s.substr(2, 1).c_str(), nullptr, 16);
        out = Color(static_cast<uint8_t>(r * 17), static_cast<uint8_t>(g * 17), static_cast<uint8_t>(b * 17), 255);
        return true;
    }
    if (s.size() == 6 || s.size() == 8) {
        unsigned int hex = std::strtoul(s.c_str(), nullptr, 16);
        if (s.size() == 6) {
            out = Color::fromHex(hex);
        } else {
            out = Color::fromHex(hex, true);
        }
        return true;
    }
    return false;
}

bool parseRgbFunc(std::string_view value, Color& out) {
    std::string s = toLower(value);
    bool hasAlpha = false;
    if (s.rfind("rgb(", 0) == 0) {
        s = s.substr(4);
    } else if (s.rfind("rgba(", 0) == 0) {
        s = s.substr(5);
        hasAlpha = true;
    } else {
        return false;
    }
    if (!s.empty() && s.back() == ')') {
        s.pop_back();
    }

    float components[4] = {0, 0, 0, 1};
    int count = 0;
    const char* ptr = s.c_str();
    while (*ptr && count < 4) {
        while (*ptr && (std::isspace(static_cast<unsigned char>(*ptr)) || *ptr == ',')) {
            ++ptr;
        }
        if (!*ptr) break;
        char* end = nullptr;
        float v = std::strtof(ptr, &end);
        if (end == ptr) break;
        bool percent = end && *end == '%';
        ptr = end;
        if (percent) {
            v = (v / 100.0f) * 255.0f;
        }
        components[count++] = v;
    }
    if (count < 3) return false;

    auto clampByte = [](float v) {
        return static_cast<uint8_t>(std::clamp(v, 0.0f, 255.0f));
    };

    uint8_t a = 255;
    if (hasAlpha && count >= 4) {
        float alpha = components[3];
        if (alpha <= 1.0f) {
            alpha *= 255.0f;
        }
        a = clampByte(alpha);
    }

    out = Color(clampByte(components[0]), clampByte(components[1]), clampByte(components[2]), a);
    return true;
}

bool parseNamedColor(std::string_view value, Color& out) {
    std::string name = toLower(value);
    if (name == "black") { out = Color::black(); return true; }
    if (name == "white") { out = Color::white(); return true; }
    if (name == "red") { out = Color::red(); return true; }
    if (name == "green") { out = Color::green(); return true; }
    if (name == "blue") { out = Color::blue(); return true; }
    if (name == "yellow") { out = Color::yellow(); return true; }
    if (name == "cyan") { out = Color::cyan(); return true; }
    if (name == "magenta") { out = Color::magenta(); return true; }
    if (name == "gray" || name == "grey") { out = Color::gray(); return true; }
    return false;
}

bool parseColor(std::string_view value, Color& out) {
    std::string s = trimCopy(value);
    if (s.empty()) return false;
    std::string lower = toLower(s);
    if (lower == "none") return false;
    if (parseHexColor(lower, out)) return true;
    if (parseRgbFunc(lower, out)) return true;
    if (parseNamedColor(lower, out)) return true;
    return false;
}

Color applyTint(Color color, Color tint) {
    auto mul = [](uint8_t a, uint8_t b) {
        return static_cast<uint8_t>((static_cast<int>(a) * static_cast<int>(b)) / 255);
    };
    return Color(mul(color.r, tint.r), mul(color.g, tint.g), mul(color.b, tint.b), mul(color.a, tint.a));
}

struct Transform {
    float a = 1.0f;
    float b = 0.0f;
    float c = 0.0f;
    float d = 1.0f;
    float e = 0.0f;
    float f = 0.0f;
};

Transform identityTransform() {
    return Transform{};
}

Transform multiply(const Transform& lhs, const Transform& rhs) {
    Transform out;
    out.a = lhs.a * rhs.a + lhs.c * rhs.b;
    out.b = lhs.b * rhs.a + lhs.d * rhs.b;
    out.c = lhs.a * rhs.c + lhs.c * rhs.d;
    out.d = lhs.b * rhs.c + lhs.d * rhs.d;
    out.e = lhs.a * rhs.e + lhs.c * rhs.f + lhs.e;
    out.f = lhs.b * rhs.e + lhs.d * rhs.f + lhs.f;
    return out;
}

Vec2 applyTransform(const Transform& t, const Vec2& p) {
    return Vec2(
        t.a * p.x + t.c * p.y + t.e,
        t.b * p.x + t.d * p.y + t.f
    );
}

bool isAxisAligned(const Transform& t) {
    return std::abs(t.b) < 1e-6f && std::abs(t.c) < 1e-6f;
}

float transformScale(const Transform& t) {
    float sx = std::sqrt(t.a * t.a + t.b * t.b);
    float sy = std::sqrt(t.c * t.c + t.d * t.d);
    return 0.5f * (sx + sy);
}

std::vector<float> parseNumberList(std::string_view text) {
    std::vector<float> values;
    std::string s(text);
    const char* ptr = s.c_str();
    while (*ptr) {
        while (*ptr && (std::isspace(static_cast<unsigned char>(*ptr)) || *ptr == ',')) {
            ++ptr;
        }
        if (!*ptr) break;
        char* end = nullptr;
        float v = std::strtof(ptr, &end);
        if (end == ptr) break;
        values.push_back(v);
        ptr = end;
    }
    return values;
}

Transform parseTransform(std::string_view value) {
    std::string s = trimCopy(value);
    Transform t = identityTransform();
    size_t pos = 0;
    while (pos < s.size()) {
        while (pos < s.size() && std::isspace(static_cast<unsigned char>(s[pos]))) {
            ++pos;
        }
        size_t nameStart = pos;
        while (pos < s.size() && std::isalpha(static_cast<unsigned char>(s[pos]))) {
            ++pos;
        }
        if (pos == nameStart) break;
        std::string name = toLower(std::string_view(s).substr(nameStart, pos - nameStart));
        while (pos < s.size() && std::isspace(static_cast<unsigned char>(s[pos]))) {
            ++pos;
        }
        if (pos >= s.size() || s[pos] != '(') {
            break;
        }
        ++pos;
        size_t argStart = pos;
        int depth = 1;
        while (pos < s.size() && depth > 0) {
            if (s[pos] == '(') depth++;
            else if (s[pos] == ')') depth--;
            if (depth > 0) ++pos;
        }
        std::string_view argText = std::string_view(s).substr(argStart, pos - argStart);
        if (pos < s.size() && s[pos] == ')') ++pos;

        std::vector<float> args = parseNumberList(argText);
        Transform local = identityTransform();
        if (name == "translate") {
            float tx = args.size() > 0 ? args[0] : 0.0f;
            float ty = args.size() > 1 ? args[1] : 0.0f;
            local.e = tx;
            local.f = ty;
        } else if (name == "scale") {
            float sx = args.size() > 0 ? args[0] : 1.0f;
            float sy = args.size() > 1 ? args[1] : sx;
            local.a = sx;
            local.d = sy;
        } else if (name == "rotate") {
            float angle = args.size() > 0 ? args[0] : 0.0f;
            float radians = angle * 3.14159265f / 180.0f;
            float cosA = std::cos(radians);
            float sinA = std::sin(radians);
            local.a = cosA;
            local.b = sinA;
            local.c = -sinA;
            local.d = cosA;
            if (args.size() > 2) {
                float cx = args[1];
                float cy = args[2];
                Transform translateTo = identityTransform();
                translateTo.e = cx;
                translateTo.f = cy;
                Transform translateBack = identityTransform();
                translateBack.e = -cx;
                translateBack.f = -cy;
                local = multiply(translateTo, multiply(local, translateBack));
            }
        } else if (name == "matrix" && args.size() >= 6) {
            local.a = args[0];
            local.b = args[1];
            local.c = args[2];
            local.d = args[3];
            local.e = args[4];
            local.f = args[5];
        }

        // Pre-multiply to preserve SVG transform order
        t = multiply(local, t);
    }
    return t;
}

std::unordered_map<std::string, std::string> parseAttributes(std::string_view text) {
    std::unordered_map<std::string, std::string> attrs;
    size_t i = 0;
    while (i < text.size()) {
        while (i < text.size() && std::isspace(static_cast<unsigned char>(text[i]))) {
            ++i;
        }
        if (i >= text.size()) break;
        if (text[i] == '/') break;

        size_t keyStart = i;
        while (i < text.size() && !std::isspace(static_cast<unsigned char>(text[i])) && text[i] != '=' && text[i] != '/') {
            ++i;
        }
        if (i == keyStart) break;
        std::string key = toLower(text.substr(keyStart, i - keyStart));

        while (i < text.size() && std::isspace(static_cast<unsigned char>(text[i]))) {
            ++i;
        }
        std::string value;
        if (i < text.size() && text[i] == '=') {
            ++i;
            while (i < text.size() && std::isspace(static_cast<unsigned char>(text[i]))) {
                ++i;
            }
            if (i < text.size() && (text[i] == '"' || text[i] == '\'')) {
                char quote = text[i++];
                size_t valStart = i;
                while (i < text.size() && text[i] != quote) {
                    ++i;
                }
                value = std::string(text.substr(valStart, i - valStart));
                if (i < text.size() && text[i] == quote) ++i;
            } else {
                size_t valStart = i;
                while (i < text.size() && !std::isspace(static_cast<unsigned char>(text[i])) && text[i] != '/') {
                    ++i;
                }
                value = std::string(text.substr(valStart, i - valStart));
            }
        }
        attrs[key] = value;
    }
    return attrs;
}

void applyStyleAttributes(std::unordered_map<std::string, std::string>& attrs) {
    auto it = attrs.find("style");
    if (it == attrs.end()) return;

    std::string_view style = it->second;
    size_t i = 0;
    while (i < style.size()) {
        size_t keyStart = i;
        while (i < style.size() && style[i] != ':' && style[i] != ';') {
            ++i;
        }
        std::string key = trimCopy(style.substr(keyStart, i - keyStart));
        if (i >= style.size() || style[i] != ':') {
            if (i < style.size()) ++i;
            continue;
        }
        ++i;
        size_t valStart = i;
        while (i < style.size() && style[i] != ';') {
            ++i;
        }
        std::string value = trimCopy(style.substr(valStart, i - valStart));
        if (!key.empty()) {
            attrs[toLower(key)] = value;
        }
        if (i < style.size() && style[i] == ';') ++i;
    }
}

SvgDocument::Paint parsePaint(const std::unordered_map<std::string, std::string>& attrs) {
    SvgDocument::Paint paint;

    auto fillIt = attrs.find("fill");
    if (fillIt != attrs.end()) {
        Color color;
        if (parseColor(fillIt->second, color)) {
            paint.fill = color;
            paint.hasFill = true;
        } else {
            paint.hasFill = false;
        }
    }

    auto strokeIt = attrs.find("stroke");
    if (strokeIt != attrs.end()) {
        Color color;
        if (parseColor(strokeIt->second, color)) {
            paint.stroke = color;
            paint.hasStroke = true;
        } else {
            paint.hasStroke = false;
        }
    }

    auto strokeWidthIt = attrs.find("stroke-width");
    if (strokeWidthIt != attrs.end()) {
        paint.strokeWidth = parseFloat(strokeWidthIt->second, paint.strokeWidth);
    }

    auto fillRuleIt = attrs.find("fill-rule");
    if (fillRuleIt != attrs.end()) {
        std::string rule = toLower(fillRuleIt->second);
        if (rule == "evenodd") {
            paint.fillRule = SvgDocument::Paint::FillRule::EvenOdd;
        } else {
            paint.fillRule = SvgDocument::Paint::FillRule::NonZero;
        }
    }

    auto dashArrayIt = attrs.find("stroke-dasharray");
    if (dashArrayIt != attrs.end()) {
        std::string dash = trimCopy(dashArrayIt->second);
        if (dash == "none") {
            paint.dashArray.clear();
        } else {
            paint.dashArray = parseNumberList(dashArrayIt->second);
        }
    }

    auto dashOffsetIt = attrs.find("stroke-dashoffset");
    if (dashOffsetIt != attrs.end()) {
        paint.dashOffset = parseFloat(dashOffsetIt->second, paint.dashOffset);
    }

    auto lineCapIt = attrs.find("stroke-linecap");
    if (lineCapIt != attrs.end()) {
        std::string cap = toLower(lineCapIt->second);
        if (cap == "round") {
            paint.lineCap = SvgDocument::Paint::LineCap::Round;
        } else if (cap == "square") {
            paint.lineCap = SvgDocument::Paint::LineCap::Square;
        } else {
            paint.lineCap = SvgDocument::Paint::LineCap::Butt;
        }
    }

    auto lineJoinIt = attrs.find("stroke-linejoin");
    if (lineJoinIt != attrs.end()) {
        std::string join = toLower(lineJoinIt->second);
        if (join == "round") {
            paint.lineJoin = SvgDocument::Paint::LineJoin::Round;
        } else if (join == "bevel") {
            paint.lineJoin = SvgDocument::Paint::LineJoin::Bevel;
        } else {
            paint.lineJoin = SvgDocument::Paint::LineJoin::Miter;
        }
    }

    float opacity = 1.0f;
    auto opacityIt = attrs.find("opacity");
    if (opacityIt != attrs.end()) {
        opacity = parseOpacity(opacityIt->second, 1.0f);
    }

    float fillOpacity = 1.0f;
    auto fillOpacityIt = attrs.find("fill-opacity");
    if (fillOpacityIt != attrs.end()) {
        fillOpacity = parseOpacity(fillOpacityIt->second, 1.0f);
    }

    float strokeOpacity = 1.0f;
    auto strokeOpacityIt = attrs.find("stroke-opacity");
    if (strokeOpacityIt != attrs.end()) {
        strokeOpacity = parseOpacity(strokeOpacityIt->second, 1.0f);
    }

    paint.fill.a = static_cast<uint8_t>(paint.fill.a * opacity * fillOpacity);
    paint.stroke.a = static_cast<uint8_t>(paint.stroke.a * opacity * strokeOpacity);

    return paint;
}

SvgDocument::Paint applyTransformScale(const SvgDocument::Paint& paint, const Transform& transform) {
    SvgDocument::Paint out = paint;
    float scale = transformScale(transform);
    out.strokeWidth *= scale;
    out.dashOffset *= scale;
    if (!out.dashArray.empty()) {
        for (float& v : out.dashArray) {
            v *= scale;
        }
    }
    return out;
}

std::vector<Vec2> parsePointsList(std::string_view text) {
    std::vector<Vec2> points;
    std::string s(text);
    const char* ptr = s.c_str();
    std::vector<float> values;
    while (*ptr) {
        while (*ptr && (std::isspace(static_cast<unsigned char>(*ptr)) || *ptr == ',')) {
            ++ptr;
        }
        if (!*ptr) break;
        char* end = nullptr;
        float v = std::strtof(ptr, &end);
        if (end == ptr) break;
        values.push_back(v);
        ptr = end;
    }
    for (size_t i = 0; i + 1 < values.size(); i += 2) {
        points.emplace_back(values[i], values[i + 1]);
    }
    return points;
}

struct PathResult {
    std::vector<Vec2> points;
    bool closed = false;
};

float vectorAngle(const Vec2& u, const Vec2& v) {
    float dot = u.dot(v);
    float len = std::sqrt(u.lengthSquared() * v.lengthSquared());
    if (len <= 0.0f) return 0.0f;
    float ang = std::acos(std::clamp(dot / len, -1.0f, 1.0f));
    float cross = u.x * v.y - u.y * v.x;
    return cross < 0.0f ? -ang : ang;
}

void addCubicPoints(const Vec2& p0, const Vec2& p1, const Vec2& p2, const Vec2& p3,
                    std::vector<Vec2>& out) {
    float approx = (p0 - p1).length() + (p1 - p2).length() + (p2 - p3).length();
    int segments = std::max(6, static_cast<int>(approx / 2.0f));
    segments = std::min(segments, 256);
    for (int i = 1; i <= segments; ++i) {
        float t = static_cast<float>(i) / static_cast<float>(segments);
        float it = 1.0f - t;
        Vec2 p = p0 * (it * it * it)
               + p1 * (3.0f * it * it * t)
               + p2 * (3.0f * it * t * t)
               + p3 * (t * t * t);
        out.push_back(p);
    }
}

void addQuadraticPoints(const Vec2& p0, const Vec2& p1, const Vec2& p2,
                        std::vector<Vec2>& out) {
    float approx = (p0 - p1).length() + (p1 - p2).length();
    int segments = std::max(6, static_cast<int>(approx / 2.0f));
    segments = std::min(segments, 256);
    for (int i = 1; i <= segments; ++i) {
        float t = static_cast<float>(i) / static_cast<float>(segments);
        float it = 1.0f - t;
        Vec2 p = p0 * (it * it) + p1 * (2.0f * it * t) + p2 * (t * t);
        out.push_back(p);
    }
}

void addArcPoints(const Vec2& p0, float rx, float ry, float xAxisRotation, bool largeArc,
                  bool sweep, const Vec2& p1, std::vector<Vec2>& out) {
    if (rx == 0.0f || ry == 0.0f) {
        out.push_back(p1);
        return;
    }

    rx = std::abs(rx);
    ry = std::abs(ry);

    float phi = xAxisRotation * 3.14159265f / 180.0f;
    float cosPhi = std::cos(phi);
    float sinPhi = std::sin(phi);

    float dx = (p0.x - p1.x) * 0.5f;
    float dy = (p0.y - p1.y) * 0.5f;
    float x1p = cosPhi * dx + sinPhi * dy;
    float y1p = -sinPhi * dx + cosPhi * dy;

    float rx2 = rx * rx;
    float ry2 = ry * ry;
    float x1p2 = x1p * x1p;
    float y1p2 = y1p * y1p;

    float lambda = x1p2 / rx2 + y1p2 / ry2;
    if (lambda > 1.0f) {
        float scale = std::sqrt(lambda);
        rx *= scale;
        ry *= scale;
        rx2 = rx * rx;
        ry2 = ry * ry;
    }

    float numerator = rx2 * ry2 - rx2 * y1p2 - ry2 * x1p2;
    float denom = rx2 * y1p2 + ry2 * x1p2;
    float factor = denom <= 0.0f ? 0.0f : std::sqrt(std::max(0.0f, numerator / denom));
    if (largeArc == sweep) factor = -factor;

    float cxp = factor * (rx * y1p / ry);
    float cyp = factor * (-ry * x1p / rx);

    float cx = cosPhi * cxp - sinPhi * cyp + (p0.x + p1.x) * 0.5f;
    float cy = sinPhi * cxp + cosPhi * cyp + (p0.y + p1.y) * 0.5f;

    Vec2 v1((x1p - cxp) / rx, (y1p - cyp) / ry);
    Vec2 v2((-x1p - cxp) / rx, (-y1p - cyp) / ry);

    float theta1 = vectorAngle(Vec2(1.0f, 0.0f), v1);
    float delta = vectorAngle(v1, v2);

    if (!sweep && delta > 0.0f) {
        delta -= 2.0f * 3.14159265f;
    } else if (sweep && delta < 0.0f) {
        delta += 2.0f * 3.14159265f;
    }

    float arcLen = std::abs(delta) * std::max(rx, ry);
    int segments = std::max(12, static_cast<int>(arcLen / 2.0f));
    segments = std::min(segments, 256);
    for (int i = 1; i <= segments; ++i) {
        float t = theta1 + delta * (static_cast<float>(i) / static_cast<float>(segments));
        float cosT = std::cos(t);
        float sinT = std::sin(t);
        Vec2 p(
            cx + rx * cosPhi * cosT - ry * sinPhi * sinT,
            cy + rx * sinPhi * cosT + ry * cosPhi * sinT
        );
        out.push_back(p);
    }
}

std::vector<PathResult> parsePath(std::string_view d) {
    std::vector<PathResult> results;
    std::string s(d);
    const char* ptr = s.c_str();
    char cmd = 0;
    Vec2 current(0.0f, 0.0f);
    Vec2 start(0.0f, 0.0f);
    Vec2 lastCubicCtrl(0.0f, 0.0f);
    Vec2 lastQuadCtrl(0.0f, 0.0f);
    bool hasCubicCtrl = false;
    bool hasQuadCtrl = false;
    PathResult currentPath;

    auto flushPath = [&]() {
        if (!currentPath.points.empty()) {
            results.push_back(currentPath);
        }
        currentPath = PathResult{};
    };

    auto skipSeparators = [&]() {
        while (*ptr && (std::isspace(static_cast<unsigned char>(*ptr)) || *ptr == ',')) {
            ++ptr;
        }
    };

    auto parseNumber = [&](float& out) {
        skipSeparators();
        if (!*ptr) return false;
        if (std::isalpha(static_cast<unsigned char>(*ptr))) return false;
        char* end = nullptr;
        out = std::strtof(ptr, &end);
        if (end == ptr) return false;
        ptr = end;
        return true;
    };

    while (*ptr) {
        skipSeparators();
        if (!*ptr) break;
        if (std::isalpha(static_cast<unsigned char>(*ptr))) {
            cmd = *ptr++;
            if (cmd == 'Z' || cmd == 'z') {
                currentPath.closed = true;
                current = start;
                flushPath();
            }
            continue;
        }
        if (!cmd) {
            ++ptr;
            continue;
        }

        bool relative = (cmd >= 'a' && cmd <= 'z');
        char upper = static_cast<char>(std::toupper(static_cast<unsigned char>(cmd)));

        if (upper == 'M') {
            float x = 0.0f, y = 0.0f;
            if (!parseNumber(x) || !parseNumber(y)) break;
            if (!currentPath.points.empty()) {
                flushPath();
            }
            if (relative) {
                current.x += x;
                current.y += y;
            } else {
                current.x = x;
                current.y = y;
            }
            start = current;
            currentPath.points.push_back(current);
            cmd = relative ? 'l' : 'L';
            hasCubicCtrl = false;
            hasQuadCtrl = false;
        } else if (upper == 'L') {
            float x = 0.0f, y = 0.0f;
            if (!parseNumber(x) || !parseNumber(y)) break;
            if (relative) {
                current.x += x;
                current.y += y;
            } else {
                current.x = x;
                current.y = y;
            }
            currentPath.points.push_back(current);
            hasCubicCtrl = false;
            hasQuadCtrl = false;
        } else if (upper == 'H') {
            float x = 0.0f;
            if (!parseNumber(x)) break;
            current.x = relative ? current.x + x : x;
            currentPath.points.push_back(current);
            hasCubicCtrl = false;
            hasQuadCtrl = false;
        } else if (upper == 'V') {
            float y = 0.0f;
            if (!parseNumber(y)) break;
            current.y = relative ? current.y + y : y;
            currentPath.points.push_back(current);
            hasCubicCtrl = false;
            hasQuadCtrl = false;
        } else if (upper == 'C') {
            float x1 = 0.0f, y1 = 0.0f, x2 = 0.0f, y2 = 0.0f, x = 0.0f, y = 0.0f;
            if (!parseNumber(x1) || !parseNumber(y1) || !parseNumber(x2) || !parseNumber(y2) ||
                !parseNumber(x) || !parseNumber(y)) break;
            Vec2 c1 = relative ? current + Vec2(x1, y1) : Vec2(x1, y1);
            Vec2 c2 = relative ? current + Vec2(x2, y2) : Vec2(x2, y2);
            Vec2 end = relative ? current + Vec2(x, y) : Vec2(x, y);
            addCubicPoints(current, c1, c2, end, currentPath.points);
            current = end;
            lastCubicCtrl = c2;
            hasCubicCtrl = true;
            hasQuadCtrl = false;
        } else if (upper == 'S') {
            float x2 = 0.0f, y2 = 0.0f, x = 0.0f, y = 0.0f;
            if (!parseNumber(x2) || !parseNumber(y2) || !parseNumber(x) || !parseNumber(y)) break;
            Vec2 c1 = hasCubicCtrl ? current + (current - lastCubicCtrl) : current;
            Vec2 c2 = relative ? current + Vec2(x2, y2) : Vec2(x2, y2);
            Vec2 end = relative ? current + Vec2(x, y) : Vec2(x, y);
            addCubicPoints(current, c1, c2, end, currentPath.points);
            current = end;
            lastCubicCtrl = c2;
            hasCubicCtrl = true;
            hasQuadCtrl = false;
        } else if (upper == 'Q') {
            float x1 = 0.0f, y1 = 0.0f, x = 0.0f, y = 0.0f;
            if (!parseNumber(x1) || !parseNumber(y1) || !parseNumber(x) || !parseNumber(y)) break;
            Vec2 c1 = relative ? current + Vec2(x1, y1) : Vec2(x1, y1);
            Vec2 end = relative ? current + Vec2(x, y) : Vec2(x, y);
            addQuadraticPoints(current, c1, end, currentPath.points);
            current = end;
            lastQuadCtrl = c1;
            hasQuadCtrl = true;
            hasCubicCtrl = false;
        } else if (upper == 'T') {
            float x = 0.0f, y = 0.0f;
            if (!parseNumber(x) || !parseNumber(y)) break;
            Vec2 c1 = hasQuadCtrl ? current + (current - lastQuadCtrl) : current;
            Vec2 end = relative ? current + Vec2(x, y) : Vec2(x, y);
            addQuadraticPoints(current, c1, end, currentPath.points);
            current = end;
            lastQuadCtrl = c1;
            hasQuadCtrl = true;
            hasCubicCtrl = false;
        } else if (upper == 'A') {
            float rx = 0.0f, ry = 0.0f, rot = 0.0f;
            float largeArc = 0.0f, sweep = 0.0f, x = 0.0f, y = 0.0f;
            if (!parseNumber(rx) || !parseNumber(ry) || !parseNumber(rot) ||
                !parseNumber(largeArc) || !parseNumber(sweep) || !parseNumber(x) || !parseNumber(y)) break;
            Vec2 end = relative ? current + Vec2(x, y) : Vec2(x, y);
            addArcPoints(current, rx, ry, rot, largeArc > 0.5f, sweep > 0.5f, end, currentPath.points);
            current = end;
            hasCubicCtrl = false;
            hasQuadCtrl = false;
        } else {
            float ignored = 0.0f;
            if (!parseNumber(ignored)) {
                ++ptr;
            }
        }
    }

    if (!currentPath.points.empty()) {
        results.push_back(currentPath);
    }

    return results;
}

Rect computeBounds(const std::vector<SvgDocument::Shape>& shapes) {
    bool hasBounds = false;
    Vec2 minP(0.0f, 0.0f);
    Vec2 maxP(0.0f, 0.0f);

    auto includePoint = [&](const Vec2& p) {
        if (!hasBounds) {
            minP = p;
            maxP = p;
            hasBounds = true;
        } else {
            minP.x = std::min(minP.x, p.x);
            minP.y = std::min(minP.y, p.y);
            maxP.x = std::max(maxP.x, p.x);
            maxP.y = std::max(maxP.y, p.y);
        }
    };

    for (const auto& shape : shapes) {
        switch (shape.type) {
        case SvgDocument::Shape::Type::Rect:
            includePoint(shape.rect.topLeft());
            includePoint(shape.rect.bottomRight());
            break;
        case SvgDocument::Shape::Type::Circle:
            includePoint(shape.center - Vec2(shape.radius));
            includePoint(shape.center + Vec2(shape.radius));
            break;
        case SvgDocument::Shape::Type::Ellipse:
            includePoint(shape.center - shape.radii);
            includePoint(shape.center + shape.radii);
            break;
        case SvgDocument::Shape::Type::Line:
            includePoint(shape.p1);
            includePoint(shape.p2);
            break;
        case SvgDocument::Shape::Type::Polygon:
        case SvgDocument::Shape::Type::Polyline:
            for (const auto& p : shape.points) {
                includePoint(p);
            }
            break;
        }
    }

    if (!hasBounds) {
        return Rect(0, 0, 1, 1);
    }

    return Rect::fromMinMax(minP, maxP);
}

void drawRectStroke(IDrawList& dl, const Rect& rect, float thickness, Color color,
                    SvgDocument::Paint::LineJoin lineJoin) {
    if (thickness <= 0.0f) return;
    float t = thickness;
    if (rect.width() <= 0.0f || rect.height() <= 0.0f) return;

    float innerW = std::max(0.0f, rect.width() - 2.0f * t);
    float innerH = std::max(0.0f, rect.height() - 2.0f * t);

    dl.addRectFilled(Rect(rect.x(), rect.y(), rect.width(), t), color);
    dl.addRectFilled(Rect(rect.x(), rect.bottom() - t, rect.width(), t), color);
    if (innerH > 0.0f) {
        dl.addRectFilled(Rect(rect.x(), rect.y() + t, t, innerH), color);
        dl.addRectFilled(Rect(rect.right() - t, rect.y() + t, t, innerH), color);
    }

    if (lineJoin == SvgDocument::Paint::LineJoin::Round) {
        float r = t * 0.5f;
        dl.addCircleFilled(rect.topLeft(), r, color);
        dl.addCircleFilled(rect.topRight(), r, color);
        dl.addCircleFilled(rect.bottomLeft(), r, color);
        dl.addCircleFilled(rect.bottomRight(), r, color);
    }
}

void drawPolygonFill(IDrawList& dl, const std::vector<Vec2>& points, Color color) {
    if (points.size() < 3) return;

    std::vector<Vec2> poly = points;
    if (poly.size() > 2 && poly.front() == poly.back()) {
        poly.pop_back();
    }
    if (poly.size() < 3) return;

    auto polygonArea = [&](const std::vector<Vec2>& pts) {
        float area = 0.0f;
        for (size_t i = 0; i < pts.size(); ++i) {
            const Vec2& a = pts[i];
            const Vec2& b = pts[(i + 1) % pts.size()];
            area += a.x * b.y - b.x * a.y;
        }
        return area * 0.5f;
    };

    auto isConvex = [&](const Vec2& a, const Vec2& b, const Vec2& c, float orientation) {
        float cross = (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
        return orientation > 0.0f ? (cross > 0.0f) : (cross < 0.0f);
    };

    auto pointInTriangle = [&](const Vec2& p, const Vec2& a, const Vec2& b, const Vec2& c) {
        float area = std::abs((b - a).x * (c - a).y - (b - a).y * (c - a).x);
        float area1 = std::abs((a - p).x * (b - p).y - (a - p).y * (b - p).x);
        float area2 = std::abs((b - p).x * (c - p).y - (b - p).y * (c - p).x);
        float area3 = std::abs((c - p).x * (a - p).y - (c - p).y * (a - p).x);
        float sum = area1 + area2 + area3;
        return std::abs(sum - area) <= 0.01f;
    };

    float orientation = polygonArea(poly);
    std::vector<int> indices(poly.size());
    for (size_t i = 0; i < poly.size(); ++i) {
        indices[i] = static_cast<int>(i);
    }

    int guard = 0;
    while (indices.size() >= 3 && guard < 10000) {
        bool earFound = false;
        for (size_t i = 0; i < indices.size(); ++i) {
            int prev = indices[(i + indices.size() - 1) % indices.size()];
            int curr = indices[i];
            int next = indices[(i + 1) % indices.size()];

            const Vec2& a = poly[prev];
            const Vec2& b = poly[curr];
            const Vec2& c = poly[next];

            if (!isConvex(a, b, c, orientation)) continue;

            bool hasPointInside = false;
            for (size_t j = 0; j < indices.size(); ++j) {
                int idx = indices[j];
                if (idx == prev || idx == curr || idx == next) continue;
                if (pointInTriangle(poly[idx], a, b, c)) {
                    hasPointInside = true;
                    break;
                }
            }
            if (hasPointInside) continue;

            dl.addTriangleFilled(a, b, c, color);
            indices.erase(indices.begin() + static_cast<long long>(i));
            earFound = true;
            break;
        }

        if (!earFound) {
            break;
        }
        ++guard;
    }

    if (indices.size() >= 3) {
        const Vec2& p0 = poly[indices[0]];
        for (size_t i = 1; i + 1 < indices.size(); ++i) {
            dl.addTriangleFilled(p0, poly[indices[i]], poly[indices[i + 1]], color);
        }
    }
}

int ellipseSegments(float rx, float ry) {
    float r = std::max(rx, ry);
    int segments = static_cast<int>(r * 0.5f);
    segments = std::max(12, segments);
    return std::min(segments, 160);
}

std::vector<Vec2> buildEllipsePoints(const Vec2& center, float rx, float ry, int segments) {
    std::vector<Vec2> points;
    points.reserve(static_cast<size_t>(segments));
    const float twoPi = 6.28318530718f;
    for (int i = 0; i < segments; ++i) {
        float angle = twoPi * static_cast<float>(i) / static_cast<float>(segments);
        points.emplace_back(center.x + std::cos(angle) * rx, center.y + std::sin(angle) * ry);
    }
    return points;
}

std::vector<float> normalizedDashArray(const std::vector<float>& dashArray) {
    std::vector<float> dash = dashArray;
    dash.erase(std::remove_if(dash.begin(), dash.end(), [](float v) { return v <= 0.0f; }), dash.end());
    if (dash.empty()) return dash;
    if (dash.size() % 2 == 1) {
        size_t originalSize = dash.size();
        dash.reserve(originalSize * 2);
        for (size_t i = 0; i < originalSize; ++i) {
            dash.push_back(dash[i]);
        }
    }
    return dash;
}

void drawLineSegment(IDrawList& dl, const Vec2& p0, const Vec2& p1, Color color, float thickness,
                     SvgDocument::Paint::LineCap lineCap) {
    Vec2 start = p0;
    Vec2 end = p1;
    if (lineCap == SvgDocument::Paint::LineCap::Square && thickness > 0.0f) {
        Vec2 dir = (p1 - p0).normalized();
        float extend = thickness * 0.5f;
        start = p0 - dir * extend;
        end = p1 + dir * extend;
    }

    dl.addLine(start, end, color, thickness);

    if (lineCap == SvgDocument::Paint::LineCap::Round && thickness > 0.0f) {
        float r = thickness * 0.5f;
        dl.addCircleFilled(p0, r, color);
        dl.addCircleFilled(p1, r, color);
    }
}

void drawDashedPolyline(IDrawList& dl, const std::vector<Vec2>& points, bool closed, Color color, float thickness,
                        const std::vector<float>& dashArray, float dashOffset,
                        SvgDocument::Paint::LineCap lineCap) {
    if (points.size() < 2) return;
    std::vector<float> pattern = normalizedDashArray(dashArray);
    if (pattern.empty()) {
        drawLineSegment(dl, points.front(), points.back(), color, thickness, lineCap);
        return;
    }

    float total = 0.0f;
    for (float v : pattern) total += v;
    if (total <= 0.0f) {
        drawLineSegment(dl, points.front(), points.back(), color, thickness, lineCap);
        return;
    }

    float offset = std::fmod(dashOffset, total);
    if (offset < 0.0f) offset += total;

    size_t dashIndex = 0;
    float dashRemaining = pattern[0];
    while (offset > dashRemaining && dashRemaining > 0.0f) {
        offset -= dashRemaining;
        dashIndex = (dashIndex + 1) % pattern.size();
        dashRemaining = pattern[dashIndex];
    }
    dashRemaining -= offset;
    bool draw = (dashIndex % 2 == 0);

    auto advanceDash = [&]() {
        dashIndex = (dashIndex + 1) % pattern.size();
        dashRemaining = pattern[dashIndex];
        draw = (dashIndex % 2 == 0);
    };

    std::vector<Vec2> poly = points;
    if (closed && points.front() != points.back()) {
        poly.push_back(points.front());
    }

    for (size_t i = 0; i + 1 < poly.size(); ++i) {
        Vec2 start = poly[i];
        Vec2 end = poly[i + 1];
        Vec2 dir = end - start;
        float segLen = dir.length();
        if (segLen <= 0.0f) continue;
        Vec2 dirNorm = dir / segLen;

        float cursor = 0.0f;
        while (cursor < segLen) {
            float step = std::min(dashRemaining, segLen - cursor);
            if (draw && step > 0.0f) {
                Vec2 s = start + dirNorm * cursor;
                Vec2 e = start + dirNorm * (cursor + step);
                drawLineSegment(dl, s, e, color, thickness, lineCap);
            }
            cursor += step;
            dashRemaining -= step;
            if (dashRemaining <= 0.0f) {
                advanceDash();
            }
        }
    }
}

void drawPolyline(IDrawList& dl, const std::vector<Vec2>& points, bool closed, Color color, float thickness,
                  SvgDocument::Paint::LineCap lineCap, SvgDocument::Paint::LineJoin lineJoin,
                  const std::vector<float>& dashArray, float dashOffset) {
    if (points.size() < 2) return;
    if (!dashArray.empty()) {
        drawDashedPolyline(dl, points, closed, color, thickness, dashArray, dashOffset, lineCap);
        return;
    }

    std::vector<Vec2> strokePoints = points;

    Vec2 startPoint = points.front();
    Vec2 endPoint = points.back();

    if (!closed && lineCap == SvgDocument::Paint::LineCap::Square && thickness > 0.0f) {
        Vec2 dirStart = (points[1] - points[0]).normalized();
        Vec2 dirEnd = (points.back() - points[points.size() - 2]).normalized();
        float extend = thickness * 0.5f;
        strokePoints.front() = points.front() - dirStart * extend;
        strokePoints.back() = points.back() + dirEnd * extend;
    }

    for (size_t i = 0; i + 1 < strokePoints.size(); ++i) {
        dl.addLine(strokePoints[i], strokePoints[i + 1], color, thickness);
    }
    if (closed) {
        dl.addLine(strokePoints.back(), strokePoints.front(), color, thickness);
    }

    if (!closed && lineCap == SvgDocument::Paint::LineCap::Round && thickness > 0.0f) {
        float r = thickness * 0.5f;
        dl.addCircleFilled(startPoint, r, color);
        dl.addCircleFilled(endPoint, r, color);
    }

    if (lineJoin == SvgDocument::Paint::LineJoin::Round && thickness > 0.0f) {
        float r = thickness * 0.5f;
        if (closed) {
            for (const auto& p : points) {
                dl.addCircleFilled(p, r, color);
            }
        } else {
            for (size_t i = 1; i + 1 < points.size(); ++i) {
                dl.addCircleFilled(points[i], r, color);
            }
        }
    }
}

float polygonSignedAreaSimple(const std::vector<Vec2>& pts) {
    if (pts.size() < 3) return 0.0f;
    float area = 0.0f;
    for (size_t i = 0; i < pts.size(); ++i) {
        const Vec2& a = pts[i];
        const Vec2& b = pts[(i + 1) % pts.size()];
        area += a.x * b.y - b.x * a.y;
    }
    return area * 0.5f;
}

bool pointInPolygon(const Vec2& p, const std::vector<Vec2>& poly) {
    if (poly.size() < 3) return false;
    bool inside = false;
    for (size_t i = 0, j = poly.size() - 1; i < poly.size(); j = i++) {
        const Vec2& pi = poly[i];
        const Vec2& pj = poly[j];
        bool intersect = ((pi.y > p.y) != (pj.y > p.y)) &&
                         (p.x < (pj.x - pi.x) * (p.y - pi.y) / (pj.y - pi.y + 1e-6f) + pi.x);
        if (intersect) inside = !inside;
    }
    return inside;
}

} // namespace

SvgDocument::SvgDocument() = default;

void SvgDocument::clear() {
    m_shapes.clear();
    m_viewBox = Rect();
    m_width = 0.0f;
    m_height = 0.0f;
    m_hasViewBox = false;
    m_valid = false;
}

bool SvgDocument::loadFromFile(const std::string& path) {
#ifdef _WIN32
    int wlen = MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1, nullptr, 0);
    std::wstring wpath(wlen, 0);
    MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1, wpath.data(), wlen);
    FILE* f = _wfopen(wpath.c_str(), L"rb");
#else
    FILE* f = fopen(path.c_str(), "rb");
#endif
    if (!f) {
        FST_LOG_ERROR("Failed to open SVG file");
        return false;
    }

    fseek(f, 0, SEEK_END);
    size_t size = ftell(f);
    fseek(f, 0, SEEK_SET);

    std::string data;
    data.resize(size);
    fread(data.data(), 1, size, f);
    fclose(f);

    return loadFromMemory(data);
}

bool SvgDocument::loadFromMemory(std::string_view svg) {
    clear();
    if (svg.empty()) return false;

    std::string data(svg);
    std::vector<Transform> transformStack;
    transformStack.push_back(identityTransform());
    size_t pos = 0;
    while (true) {
        pos = data.find('<', pos);
        if (pos == std::string::npos) break;
        if (pos + 1 >= data.size()) break;

        char next = data[pos + 1];
        if (next == '/') {
            size_t end = data.find('>', pos + 1);
            if (end == std::string::npos) break;
            std::string_view closeTag = std::string_view(data).substr(pos + 2, end - pos - 2);
            size_t nameEnd = 0;
            while (nameEnd < closeTag.size() && !std::isspace(static_cast<unsigned char>(closeTag[nameEnd]))) {
                ++nameEnd;
            }
            std::string name = toLower(closeTag.substr(0, nameEnd));
            if ((name == "g" || name == "svg") && transformStack.size() > 1) {
                transformStack.pop_back();
            }
            pos = end + 1;
            continue;
        }
        if (next == '!' || next == '?') {
            size_t end = data.find('>', pos + 1);
            if (end == std::string::npos) break;
            pos = end + 1;
            continue;
        }

        size_t end = data.find('>', pos + 1);
        if (end == std::string::npos) break;

        std::string_view tag = std::string_view(data).substr(pos + 1, end - pos - 1);
        bool selfClosing = false;
        if (!tag.empty() && tag.back() == '/') {
            selfClosing = true;
            tag = tag.substr(0, tag.size() - 1);
        }

        size_t nameEnd = 0;
        while (nameEnd < tag.size() && !std::isspace(static_cast<unsigned char>(tag[nameEnd]))) {
            ++nameEnd;
        }
        std::string name = toLower(tag.substr(0, nameEnd));
        std::string_view attrsText = (nameEnd < tag.size()) ? tag.substr(nameEnd) : std::string_view();

        auto attrs = parseAttributes(attrsText);
        applyStyleAttributes(attrs);

        Transform currentTransform = transformStack.empty() ? identityTransform() : transformStack.back();
        Transform elementTransform = identityTransform();
        auto transformIt = attrs.find("transform");
        if (transformIt != attrs.end()) {
            elementTransform = parseTransform(transformIt->second);
        }
        Transform combinedTransform = multiply(currentTransform, elementTransform);

        if (name == "svg") {
            auto widthIt = attrs.find("width");
            if (widthIt != attrs.end()) {
                m_width = parseFloat(widthIt->second, m_width);
            }
            auto heightIt = attrs.find("height");
            if (heightIt != attrs.end()) {
                m_height = parseFloat(heightIt->second, m_height);
            }
            auto viewBoxIt = attrs.find("viewbox");
            if (viewBoxIt != attrs.end()) {
                std::vector<Vec2> values = parsePointsList(viewBoxIt->second);
                if (values.size() >= 2) {
                    m_viewBox = Rect(values[0].x, values[0].y, values[1].x, values[1].y);
                    m_hasViewBox = true;
                } else {
                    std::string vb = viewBoxIt->second;
                    std::vector<float> numbers;
                    const char* ptr = vb.c_str();
                    while (*ptr) {
                        while (*ptr && (std::isspace(static_cast<unsigned char>(*ptr)) || *ptr == ',')) {
                            ++ptr;
                        }
                        if (!*ptr) break;
                        char* endNum = nullptr;
                        float v = std::strtof(ptr, &endNum);
                        if (endNum == ptr) break;
                        numbers.push_back(v);
                        ptr = endNum;
                    }
                    if (numbers.size() >= 4) {
                        m_viewBox = Rect(numbers[0], numbers[1], numbers[2], numbers[3]);
                        m_hasViewBox = true;
                    }
                }
            }
            if (!selfClosing) {
                transformStack.push_back(combinedTransform);
            }
        } else if (name == "g") {
            if (!selfClosing) {
                transformStack.push_back(combinedTransform);
            }
        } else if (name == "rect") {
            float x = parseFloat(attrs["x"], 0.0f);
            float y = parseFloat(attrs["y"], 0.0f);
            float w = parseFloat(attrs["width"], 0.0f);
            float h = parseFloat(attrs["height"], 0.0f);
            if (w > 0.0f && h > 0.0f) {
                Shape shape;
                Rect rect(x, y, w, h);
                float rx = parseFloat(attrs["rx"], 0.0f);
                float ry = parseFloat(attrs["ry"], 0.0f);
                float rounding = std::min(rx > 0.0f ? rx : ry, ry > 0.0f ? ry : rx);
                Paint paint = applyTransformScale(parsePaint(attrs), combinedTransform);
                if (isAxisAligned(combinedTransform)) {
                    Vec2 p0 = applyTransform(combinedTransform, rect.topLeft());
                    Vec2 p1 = applyTransform(combinedTransform, rect.bottomRight());
                    shape.type = Shape::Type::Rect;
                    shape.rect = Rect::fromMinMax(p0, p1);
                    shape.rounding = rounding * transformScale(combinedTransform);
                    shape.paint = paint;
                    m_shapes.push_back(std::move(shape));
                } else {
                    std::vector<Vec2> pts = {
                        rect.topLeft(), rect.topRight(), rect.bottomRight(), rect.bottomLeft()
                    };
                    for (auto& p : pts) {
                        p = applyTransform(combinedTransform, p);
                    }
                    shape.type = Shape::Type::Polygon;
                    shape.points = std::move(pts);
                    shape.paint = paint;
                    m_shapes.push_back(std::move(shape));
                }
            }
        } else if (name == "circle") {
            float cx = parseFloat(attrs["cx"], 0.0f);
            float cy = parseFloat(attrs["cy"], 0.0f);
            float r = parseFloat(attrs["r"], 0.0f);
            if (r > 0.0f) {
                Shape shape;
                Paint paint = applyTransformScale(parsePaint(attrs), combinedTransform);
                if (isAxisAligned(combinedTransform)) {
                    Vec2 center = applyTransform(combinedTransform, Vec2(cx, cy));
                    float rx = r * std::abs(combinedTransform.a);
                    float ry = r * std::abs(combinedTransform.d);
                    if (std::abs(rx - ry) < 1e-3f) {
                        shape.type = Shape::Type::Circle;
                        shape.center = center;
                        shape.radius = (rx + ry) * 0.5f;
                    } else {
                        shape.type = Shape::Type::Ellipse;
                        shape.center = center;
                        shape.radii = Vec2(rx, ry);
                    }
                    shape.paint = paint;
                    m_shapes.push_back(std::move(shape));
                } else {
                    int segments = ellipseSegments(r, r);
                    std::vector<Vec2> pts = buildEllipsePoints(Vec2(cx, cy), r, r, segments);
                    for (auto& p : pts) {
                        p = applyTransform(combinedTransform, p);
                    }
                    shape.type = Shape::Type::Polygon;
                    shape.points = std::move(pts);
                    shape.paint = paint;
                    m_shapes.push_back(std::move(shape));
                }
            }
        } else if (name == "ellipse") {
            float cx = parseFloat(attrs["cx"], 0.0f);
            float cy = parseFloat(attrs["cy"], 0.0f);
            float rx = parseFloat(attrs["rx"], 0.0f);
            float ry = parseFloat(attrs["ry"], 0.0f);
            if (rx > 0.0f && ry > 0.0f) {
                Shape shape;
                Paint paint = applyTransformScale(parsePaint(attrs), combinedTransform);
                if (isAxisAligned(combinedTransform)) {
                    Vec2 center = applyTransform(combinedTransform, Vec2(cx, cy));
                    shape.type = Shape::Type::Ellipse;
                    shape.center = center;
                    shape.radii = Vec2(rx * std::abs(combinedTransform.a), ry * std::abs(combinedTransform.d));
                    shape.paint = paint;
                    m_shapes.push_back(std::move(shape));
                } else {
                    int segments = ellipseSegments(rx, ry);
                    std::vector<Vec2> pts = buildEllipsePoints(Vec2(cx, cy), rx, ry, segments);
                    for (auto& p : pts) {
                        p = applyTransform(combinedTransform, p);
                    }
                    shape.type = Shape::Type::Polygon;
                    shape.points = std::move(pts);
                    shape.paint = paint;
                    m_shapes.push_back(std::move(shape));
                }
            }
        } else if (name == "line") {
            Shape shape;
            shape.type = Shape::Type::Line;
            shape.p1 = applyTransform(combinedTransform,
                                      Vec2(parseFloat(attrs["x1"], 0.0f), parseFloat(attrs["y1"], 0.0f)));
            shape.p2 = applyTransform(combinedTransform,
                                      Vec2(parseFloat(attrs["x2"], 0.0f), parseFloat(attrs["y2"], 0.0f)));
            shape.paint = applyTransformScale(parsePaint(attrs), combinedTransform);
            m_shapes.push_back(std::move(shape));
        } else if (name == "polygon") {
            auto pointsIt = attrs.find("points");
            if (pointsIt != attrs.end()) {
                Shape shape;
                shape.type = Shape::Type::Polygon;
                shape.points = parsePointsList(pointsIt->second);
                for (auto& p : shape.points) {
                    p = applyTransform(combinedTransform, p);
                }
                shape.paint = applyTransformScale(parsePaint(attrs), combinedTransform);
                if (shape.points.size() >= 3) {
                    m_shapes.push_back(std::move(shape));
                }
            }
        } else if (name == "polyline") {
            auto pointsIt = attrs.find("points");
            if (pointsIt != attrs.end()) {
                Shape shape;
                shape.type = Shape::Type::Polyline;
                shape.points = parsePointsList(pointsIt->second);
                for (auto& p : shape.points) {
                    p = applyTransform(combinedTransform, p);
                }
                shape.paint = applyTransformScale(parsePaint(attrs), combinedTransform);
                if (shape.points.size() >= 2) {
                    m_shapes.push_back(std::move(shape));
                }
            }
        } else if (name == "path") {
            auto dIt = attrs.find("d");
            if (dIt != attrs.end()) {
                Paint paint = applyTransformScale(parsePaint(attrs), combinedTransform);
                std::vector<PathResult> paths = parsePath(dIt->second);
                std::vector<Shape> pathShapes;
                pathShapes.reserve(paths.size());
                for (auto& path : paths) {
                    Shape shape;
                    shape.type = path.closed ? Shape::Type::Polygon : Shape::Type::Polyline;
                    shape.points = std::move(path.points);
                    for (auto& p : shape.points) {
                        p = applyTransform(combinedTransform, p);
                    }
                    shape.paint = paint;
                    if (shape.points.size() >= (path.closed ? 3u : 2u)) {
                        pathShapes.push_back(std::move(shape));
                    }
                }

                if (paint.fillRule == Paint::FillRule::EvenOdd) {
                    struct PathInfo {
                        size_t index;
                        float area;
                        Vec2 point;
                    };
                    std::vector<PathInfo> infos;
                    infos.reserve(pathShapes.size());
                    for (size_t i = 0; i < pathShapes.size(); ++i) {
                        if (pathShapes[i].type != Shape::Type::Polygon || !pathShapes[i].paint.hasFill) continue;
                        float area = std::abs(polygonSignedAreaSimple(pathShapes[i].points));
                        Vec2 avg(0.0f, 0.0f);
                        for (const auto& p : pathShapes[i].points) {
                            avg += p;
                        }
                        avg = avg / static_cast<float>(pathShapes[i].points.size());
                        infos.push_back({i, area, avg});
                    }

                    for (const auto& info : infos) {
                        int containCount = 0;
                        for (const auto& other : infos) {
                            if (other.index == info.index) continue;
                            if (other.area <= info.area) continue;
                            if (pointInPolygon(info.point, pathShapes[other.index].points)) {
                                containCount++;
                            }
                        }
                        if (containCount % 2 == 1) {
                            pathShapes[info.index].paint.hasFill = false;
                        }
                    }
                }

                for (auto& shape : pathShapes) {
                    m_shapes.push_back(std::move(shape));
                }
            }
        }

        pos = end + 1;
    }

    if (m_width <= 0.0f || m_height <= 0.0f) {
        if (m_hasViewBox) {
            m_width = m_viewBox.width();
            m_height = m_viewBox.height();
        }
    }

    m_valid = !m_shapes.empty();
    return m_valid;
}

bool SvgDocument::render(IDrawList& drawList, const Rect& bounds, const SvgRenderOptions& options) const {
    if (m_shapes.empty()) return false;
    if (bounds.width() <= 0.0f || bounds.height() <= 0.0f) return false;

    Rect viewBox = m_hasViewBox ? m_viewBox : Rect(0.0f, 0.0f, m_width, m_height);
    if (viewBox.width() <= 0.0f || viewBox.height() <= 0.0f) {
        viewBox = computeBounds(m_shapes);
    }
    if (viewBox.width() <= 0.0f || viewBox.height() <= 0.0f) {
        return false;
    }

    float scaleX = bounds.width() / viewBox.width();
    float scaleY = bounds.height() / viewBox.height();

    Vec2 offset(bounds.x(), bounds.y());
    if (options.preserveAspectRatio) {
        float scale = std::min(scaleX, scaleY);
        scaleX = scaleY = scale;
        float contentW = viewBox.width() * scale;
        float contentH = viewBox.height() * scale;
        offset.x += (bounds.width() - contentW) * 0.5f;
        offset.y += (bounds.height() - contentH) * 0.5f;
    }

    offset.x -= viewBox.x() * scaleX;
    offset.y -= viewBox.y() * scaleY;

    auto mapPoint = [&](const Vec2& p) {
        return Vec2(offset.x + p.x * scaleX, offset.y + p.y * scaleY);
    };

    float strokeScale = options.preserveAspectRatio ? scaleX : std::min(std::abs(scaleX), std::abs(scaleY));

    for (const auto& shape : m_shapes) {
        Paint paint = shape.paint;
        if (paint.hasFill) {
            paint.fill = applyTint(paint.fill, options.tint);
        }
        if (paint.hasStroke) {
            paint.stroke = applyTint(paint.stroke, options.tint);
        }
        std::vector<float> dashArray = paint.dashArray;
        if (!dashArray.empty()) {
            for (float& v : dashArray) {
                v *= strokeScale;
            }
        }
        float dashOffset = paint.dashOffset * strokeScale;

        switch (shape.type) {
        case Shape::Type::Rect: {
            Rect rect = Rect::fromMinMax(mapPoint(shape.rect.topLeft()), mapPoint(shape.rect.bottomRight()));
            float rounding = shape.rounding * strokeScale;
            if (paint.hasFill && paint.fill.a > 0) {
                drawList.addRectFilled(rect, paint.fill, rounding);
            }
            if (paint.hasStroke && paint.stroke.a > 0 && paint.strokeWidth > 0.0f) {
                float thickness = paint.strokeWidth * strokeScale;
                if (!dashArray.empty()) {
                    std::vector<Vec2> outline = {
                        rect.topLeft(), rect.topRight(), rect.bottomRight(), rect.bottomLeft()
                    };
                    drawPolyline(drawList, outline, true, paint.stroke, thickness, paint.lineCap, paint.lineJoin,
                                 dashArray, dashOffset);
                } else {
                    drawRectStroke(drawList, rect, thickness, paint.stroke, paint.lineJoin);
                }
            }
            break;
        }
        case Shape::Type::Circle: {
            Vec2 center = mapPoint(shape.center);
            float radius = shape.radius * strokeScale;
            if (paint.hasFill && paint.fill.a > 0) {
                drawList.addCircleFilled(center, radius, paint.fill);
            }
            if (paint.hasStroke && paint.stroke.a > 0 && paint.strokeWidth > 0.0f) {
                float thickness = paint.strokeWidth * strokeScale;
                int segments = ellipseSegments(radius, radius);
                std::vector<Vec2> circlePoints = buildEllipsePoints(center, radius, radius, segments);
                drawPolyline(drawList, circlePoints, true, paint.stroke, thickness, paint.lineCap, paint.lineJoin,
                             dashArray, dashOffset);
            }
            break;
        }
        case Shape::Type::Ellipse: {
            Vec2 center = mapPoint(shape.center);
            float rx = shape.radii.x * std::abs(scaleX);
            float ry = shape.radii.y * std::abs(scaleY);
            if (paint.hasFill && paint.fill.a > 0) {
                int segments = ellipseSegments(rx, ry);
                std::vector<Vec2> ellipsePoints = buildEllipsePoints(center, rx, ry, segments);
                drawPolygonFill(drawList, ellipsePoints, paint.fill);
            }
            if (paint.hasStroke && paint.stroke.a > 0 && paint.strokeWidth > 0.0f) {
                float thickness = paint.strokeWidth * strokeScale;
                int segments = ellipseSegments(rx, ry);
                std::vector<Vec2> ellipsePoints = buildEllipsePoints(center, rx, ry, segments);
                drawPolyline(drawList, ellipsePoints, true, paint.stroke, thickness, paint.lineCap, paint.lineJoin,
                             dashArray, dashOffset);
            }
            break;
        }
        case Shape::Type::Line: {
            if (paint.hasStroke && paint.stroke.a > 0 && paint.strokeWidth > 0.0f) {
                std::vector<Vec2> linePoints = {mapPoint(shape.p1), mapPoint(shape.p2)};
                drawPolyline(drawList, linePoints, false, paint.stroke, paint.strokeWidth * strokeScale,
                             paint.lineCap, paint.lineJoin, dashArray, dashOffset);
            }
            break;
        }
        case Shape::Type::Polygon: {
            std::vector<Vec2> mapped;
            mapped.reserve(shape.points.size());
            for (const auto& p : shape.points) {
                mapped.push_back(mapPoint(p));
            }
            if (paint.hasFill && paint.fill.a > 0) {
                drawPolygonFill(drawList, mapped, paint.fill);
            }
            if (paint.hasStroke && paint.stroke.a > 0 && paint.strokeWidth > 0.0f) {
                drawPolyline(drawList, mapped, true, paint.stroke, paint.strokeWidth * strokeScale,
                             paint.lineCap, paint.lineJoin, dashArray, dashOffset);
            }
            break;
        }
        case Shape::Type::Polyline: {
            std::vector<Vec2> mapped;
            mapped.reserve(shape.points.size());
            for (const auto& p : shape.points) {
                mapped.push_back(mapPoint(p));
            }
            if (paint.hasStroke && paint.stroke.a > 0 && paint.strokeWidth > 0.0f) {
                drawPolyline(drawList, mapped, false, paint.stroke, paint.strokeWidth * strokeScale,
                             paint.lineCap, paint.lineJoin, dashArray, dashOffset);
            }
            break;
        }
        }
    }

    return true;
}

} // namespace fst
