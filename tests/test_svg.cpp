#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "MockDrawList.h"
#include <fastener/graphics/svg.h>

using namespace fst;
using namespace fst::testing;
using ::testing::_;
using ::testing::AtLeast;

TEST(SvgRenderTest, RendersBasicShapes) {
    SvgDocument doc;
    const char* svg = R"svg(
        <svg width="100" height="50" viewBox="0 0 100 50">
            <rect x="10" y="5" width="20" height="10" fill="#ff0000" />
            <circle cx="60" cy="25" r="5" fill="rgb(0,255,0)" />
            <line x1="0" y1="0" x2="100" y2="50" stroke="#0000ff" stroke-width="2" />
        </svg>
    )svg";

    ASSERT_TRUE(doc.loadFromMemory(svg));

    MockDrawList dl;
    EXPECT_CALL(dl, addRectFilled(Rect(10, 5, 20, 10), Color(255, 0, 0, 255), 0.0f)).Times(1);
    EXPECT_CALL(dl, addCircleFilled(Vec2(60, 25), 5.0f, Color(0, 255, 0, 255), _)).Times(1);
    EXPECT_CALL(dl, addLine(Vec2(0, 0), Vec2(100, 50), Color(0, 0, 255, 255), 2.0f)).Times(1);

    EXPECT_TRUE(doc.render(dl, Rect(0, 0, 100, 50)));
}

TEST(SvgRenderTest, ScalesWithoutPreservingAspectRatio) {
    SvgDocument doc;
    const char* svg = R"svg(
        <svg viewBox="0 0 10 10">
            <rect x="0" y="0" width="10" height="10" fill="#000000" />
        </svg>
    )svg";

    ASSERT_TRUE(doc.loadFromMemory(svg));

    MockDrawList dl;
    SvgRenderOptions options;
    options.preserveAspectRatio = false;

    EXPECT_CALL(dl, addRectFilled(Rect(0, 0, 20, 10), Color(0, 0, 0, 255), 0.0f)).Times(1);
    EXPECT_TRUE(doc.render(dl, Rect(0, 0, 20, 10), options));
}

TEST(SvgRenderTest, SupportsEllipse) {
    SvgDocument doc;
    const char* svg = R"svg(
        <svg viewBox="0 0 40 20">
            <ellipse cx="20" cy="10" rx="12" ry="6" fill="#00ff00" stroke="#ff0000" stroke-width="2" />
        </svg>
    )svg";

    ASSERT_TRUE(doc.loadFromMemory(svg));

    MockDrawList dl;
    EXPECT_CALL(dl, addTriangleFilled(_, _, _, _)).Times(AtLeast(1));
    EXPECT_CALL(dl, addLine(_, _, _, _)).Times(AtLeast(1));

    EXPECT_TRUE(doc.render(dl, Rect(0, 0, 40, 20)));
}

TEST(SvgRenderTest, SupportsStrokeLinecapRound) {
    SvgDocument doc;
    const char* svg = R"svg(
        <svg width="10" height="10" viewBox="0 0 10 10">
            <line x1="1" y1="1" x2="9" y2="9" stroke="#000000" stroke-width="4" stroke-linecap="round" />
        </svg>
    )svg";

    ASSERT_TRUE(doc.loadFromMemory(svg));

    MockDrawList dl;
    EXPECT_CALL(dl, addCircleFilled(Vec2(1, 1), 2.0f, Color(0, 0, 0, 255), _)).Times(1);
    EXPECT_CALL(dl, addCircleFilled(Vec2(9, 9), 2.0f, Color(0, 0, 0, 255), _)).Times(1);

    EXPECT_TRUE(doc.render(dl, Rect(0, 0, 10, 10)));
}

TEST(SvgRenderTest, SupportsStrokeLinejoinRound) {
    SvgDocument doc;
    const char* svg = R"svg(
        <svg width="20" height="10" viewBox="0 0 20 10">
            <polyline points="2,2 10,8 18,2" stroke="#000000" stroke-width="4" stroke-linejoin="round" fill="none" />
        </svg>
    )svg";

    ASSERT_TRUE(doc.loadFromMemory(svg));

    MockDrawList dl;
    EXPECT_CALL(dl, addCircleFilled(Vec2(10, 8), 2.0f, Color(0, 0, 0, 255), _)).Times(1);

    EXPECT_TRUE(doc.render(dl, Rect(0, 0, 20, 10)));
}

TEST(SvgRenderTest, SupportsCubicCurvePath) {
    SvgDocument doc;
    const char* svg = R"svg(
        <svg width="20" height="20" viewBox="0 0 20 20">
            <path d="M2 2 C 6 2 14 18 18 18" stroke="#000000" stroke-width="2" fill="none" />
        </svg>
    )svg";

    ASSERT_TRUE(doc.loadFromMemory(svg));

    MockDrawList dl;
    EXPECT_CALL(dl, addLine(_, _, _, _)).Times(AtLeast(2));

    EXPECT_TRUE(doc.render(dl, Rect(0, 0, 20, 20)));
}

TEST(SvgRenderTest, SupportsArcPath) {
    SvgDocument doc;
    const char* svg = R"svg(
        <svg width="20" height="20" viewBox="0 0 20 20">
            <path d="M2 10 A 8 8 0 0 1 18 10" stroke="#000000" stroke-width="2" fill="none" />
        </svg>
    )svg";

    ASSERT_TRUE(doc.loadFromMemory(svg));

    MockDrawList dl;
    EXPECT_CALL(dl, addLine(_, _, _, _)).Times(AtLeast(2));

    EXPECT_TRUE(doc.render(dl, Rect(0, 0, 20, 20)));
}

TEST(SvgRenderTest, SupportsTransformTranslate) {
    SvgDocument doc;
    const char* svg = R"svg(
        <svg width="20" height="20" viewBox="0 0 20 20">
            <rect x="0" y="0" width="10" height="10" fill="#000000" transform="translate(5 4)" />
        </svg>
    )svg";

    ASSERT_TRUE(doc.loadFromMemory(svg));

    MockDrawList dl;
    EXPECT_CALL(dl, addRectFilled(Rect(5, 4, 10, 10), Color(0, 0, 0, 255), 0.0f)).Times(1);

    EXPECT_TRUE(doc.render(dl, Rect(0, 0, 20, 20)));
}

TEST(SvgRenderTest, SupportsStrokeDasharray) {
    SvgDocument doc;
    const char* svg = R"svg(
        <svg width="20" height="20" viewBox="0 0 20 20">
            <line x1="2" y1="2" x2="18" y2="2" stroke="#000000" stroke-width="2" stroke-dasharray="4 2" />
        </svg>
    )svg";

    ASSERT_TRUE(doc.loadFromMemory(svg));

    MockDrawList dl;
    EXPECT_CALL(dl, addLine(_, _, _, _)).Times(AtLeast(2));

    EXPECT_TRUE(doc.render(dl, Rect(0, 0, 20, 20)));
}

TEST(SvgRenderTest, SupportsFillRuleEvenOdd) {
    SvgDocument doc;
    const char* svg = R"svg(
        <svg width="20" height="20" viewBox="0 0 20 20">
            <path d="M0 0 L20 0 L20 20 L0 20 Z M6 6 L14 6 L14 14 L6 14 Z" fill="#ff0000" fill-rule="evenodd" />
        </svg>
    )svg";

    ASSERT_TRUE(doc.loadFromMemory(svg));

    MockDrawList dl;
    EXPECT_CALL(dl, addTriangleFilled(_, _, _, _)).Times(2);

    EXPECT_TRUE(doc.render(dl, Rect(0, 0, 20, 20)));
}
