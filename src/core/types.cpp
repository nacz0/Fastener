#include "fastener/core/types.h"
#include "fastener/core/constants.h"
#include <cmath>
#include <algorithm>

namespace fst {

Color Color::fromHSL(float h, float s, float l, float a) {
    // Normalize hue to 0-1
    h = h - std::floor(h);
    
    auto hueToRgb = [](float p, float q, float t) {
        if (t < 0.0f) t += 1.0f;
        if (t > 1.0f) t -= 1.0f;
        if (t < 1.0f/6.0f) return p + (q - p) * 6.0f * t;
        if (t < 1.0f/2.0f) return q;
        if (t < 2.0f/3.0f) return p + (q - p) * (2.0f/3.0f - t) * 6.0f;
        return p;
    };
    
    float r, g, b;
    
    if (s == 0.0f) {
        r = g = b = l;
    } else {
        float q = l < 0.5f ? l * (1.0f + s) : l + s - l * s;
        float p = 2.0f * l - q;
        r = hueToRgb(p, q, h + 1.0f/3.0f);
        g = hueToRgb(p, q, h);
        b = hueToRgb(p, q, h - 1.0f/3.0f);
    }
    
    return Color::fromFloat(r, g, b, a);
}

Color Color::fromHSV(float h, float s, float v, float a) {
    h = h - std::floor(h); // Normalize to 0-1
    float r = 0, g = 0, b = 0;
    if (s <= 0) {
        r = g = b = v;
    } else {
        float hh = h * 6.0f;
        int i = (int)hh;
        float ff = hh - i;
        float p = v * (1.0f - s);
        float q = v * (1.0f - (s * ff));
        float t = v * (1.0f - (s * (1.0f - ff)));
        switch(i % 6) {
            case 0: r = v; g = t; b = p; break;
            case 1: r = q; g = v; b = p; break;
            case 2: r = p; g = v; b = t; break;
            case 3: r = p; g = q; b = v; break;
            case 4: r = t; g = p; b = v; break;
            case 5: r = v; g = p; b = q; break;
        }
    }
    return Color::fromFloat(r, g, b, a);
}

void Color::toHSV(float& h, float& s, float& v) const {
    float r = rf(), g = gf(), b = bf();
    float K = 0.0f;
    if (g < b) {
        std::swap(g, b);
        K = -1.0f;
    }
    if (r < g) {
        std::swap(r, g);
        K = -2.0f / 6.0f - K;
    }
    float chroma = r - std::min(g, b);
    h = std::abs(K + (g - b) / (6.0f * chroma + constants::EPSILON));
    s = chroma / (r + constants::EPSILON);
    v = r;
}

Color Color::lighter(float amount) const {
    float factor = 1.0f + amount;
    return Color::fromFloat(
        std::min(rf() * factor, 1.0f),
        std::min(gf() * factor, 1.0f),
        std::min(bf() * factor, 1.0f),
        af()
    );
}

Color Color::darker(float amount) const {
    float factor = 1.0f - amount;
    return Color::fromFloat(
        rf() * factor,
        gf() * factor,
        bf() * factor,
        af()
    );
}

Color Color::lerp(const Color& a, const Color& b, float t) {
    t = std::clamp(t, 0.0f, 1.0f);
    return {
        static_cast<uint8_t>(a.r + (b.r - a.r) * t),
        static_cast<uint8_t>(a.g + (b.g - a.g) * t),
        static_cast<uint8_t>(a.b + (b.b - a.b) * t),
        static_cast<uint8_t>(a.a + (b.a - a.a) * t)
    };
}

} // namespace fst
