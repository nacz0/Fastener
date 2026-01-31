#pragma once

#include "fastener/core/types.h"
#include "fastener/graphics/IDrawList.h"
#include <string>
#include <string_view>
#include <vector>

namespace fst {

struct SvgRenderOptions {
    bool preserveAspectRatio = true;
    Color tint = Color::white();
};

class SvgDocument {
public:
    struct Paint {
        enum class FillRule {
            NonZero,
            EvenOdd
        };

        enum class LineCap {
            Butt,
            Round,
            Square
        };

        enum class LineJoin {
            Miter,
            Round,
            Bevel
        };

        bool hasFill = true;
        bool hasStroke = false;
        Color fill = Color::black();
        Color stroke = Color::black();
        float strokeWidth = 1.0f;
        FillRule fillRule = FillRule::NonZero;
        LineCap lineCap = LineCap::Butt;
        LineJoin lineJoin = LineJoin::Miter;
        std::vector<float> dashArray;
        float dashOffset = 0.0f;
    };

    struct Shape {
        enum class Type {
            Rect,
            Circle,
            Ellipse,
            Line,
            Polygon,
            Polyline
        } type = Type::Rect;

        Rect rect;
        float rounding = 0.0f;

        Vec2 center;
        float radius = 0.0f;
        Vec2 radii;

        Vec2 p1;
        Vec2 p2;

        std::vector<Vec2> points;

        Paint paint;
    };

    SvgDocument();

    bool loadFromMemory(std::string_view svg);
    bool loadFromFile(const std::string& path);
    void clear();

    bool isValid() const { return m_valid; }
    Vec2 size() const { return {m_width, m_height}; }

    bool render(IDrawList& drawList, const Rect& bounds, const SvgRenderOptions& options = {}) const;

private:
    std::vector<Shape> m_shapes;
    Rect m_viewBox;
    float m_width = 0.0f;
    float m_height = 0.0f;
    bool m_hasViewBox = false;
    bool m_valid = false;
};

} // namespace fst
