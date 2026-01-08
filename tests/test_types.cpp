#include <gtest/gtest.h>
#include <fastener/core/types.h>

using namespace fst;

//=============================================================================
// Vec2 Tests
//=============================================================================

TEST(Vec2Test, DefaultConstructor) {
    Vec2 v;
    EXPECT_FLOAT_EQ(v.x, 0.0f);
    EXPECT_FLOAT_EQ(v.y, 0.0f);
}

TEST(Vec2Test, ParameterizedConstructor) {
    Vec2 v(3.0f, 4.0f);
    EXPECT_FLOAT_EQ(v.x, 3.0f);
    EXPECT_FLOAT_EQ(v.y, 4.0f);
}

TEST(Vec2Test, SingleValueConstructor) {
    Vec2 v(5.0f);
    EXPECT_FLOAT_EQ(v.x, 5.0f);
    EXPECT_FLOAT_EQ(v.y, 5.0f);
}

TEST(Vec2Test, Addition) {
    Vec2 a(1.0f, 2.0f);
    Vec2 b(3.0f, 4.0f);
    Vec2 result = a + b;
    EXPECT_FLOAT_EQ(result.x, 4.0f);
    EXPECT_FLOAT_EQ(result.y, 6.0f);
}

TEST(Vec2Test, Subtraction) {
    Vec2 a(5.0f, 7.0f);
    Vec2 b(2.0f, 3.0f);
    Vec2 result = a - b;
    EXPECT_FLOAT_EQ(result.x, 3.0f);
    EXPECT_FLOAT_EQ(result.y, 4.0f);
}

TEST(Vec2Test, ScalarMultiplication) {
    Vec2 v(2.0f, 3.0f);
    Vec2 result = v * 2.0f;
    EXPECT_FLOAT_EQ(result.x, 4.0f);
    EXPECT_FLOAT_EQ(result.y, 6.0f);
    
    // Test commutative property
    Vec2 result2 = 2.0f * v;
    EXPECT_FLOAT_EQ(result2.x, 4.0f);
    EXPECT_FLOAT_EQ(result2.y, 6.0f);
}

TEST(Vec2Test, ScalarDivision) {
    Vec2 v(6.0f, 8.0f);
    Vec2 result = v / 2.0f;
    EXPECT_FLOAT_EQ(result.x, 3.0f);
    EXPECT_FLOAT_EQ(result.y, 4.0f);
}

TEST(Vec2Test, Length) {
    Vec2 v(3.0f, 4.0f);
    EXPECT_FLOAT_EQ(v.length(), 5.0f);  // 3-4-5 triangle
}

TEST(Vec2Test, LengthSquared) {
    Vec2 v(3.0f, 4.0f);
    EXPECT_FLOAT_EQ(v.lengthSquared(), 25.0f);
}

TEST(Vec2Test, Normalized) {
    Vec2 v(3.0f, 4.0f);
    Vec2 n = v.normalized();
    EXPECT_FLOAT_EQ(n.length(), 1.0f);
    EXPECT_FLOAT_EQ(n.x, 0.6f);
    EXPECT_FLOAT_EQ(n.y, 0.8f);
}

TEST(Vec2Test, NormalizedZeroVector) {
    Vec2 v(0.0f, 0.0f);
    Vec2 n = v.normalized();
    EXPECT_FLOAT_EQ(n.x, 0.0f);
    EXPECT_FLOAT_EQ(n.y, 0.0f);
}

TEST(Vec2Test, DotProduct) {
    Vec2 a(1.0f, 2.0f);
    Vec2 b(3.0f, 4.0f);
    EXPECT_FLOAT_EQ(a.dot(b), 11.0f);  // 1*3 + 2*4 = 11
}

TEST(Vec2Test, Equality) {
    Vec2 a(1.0f, 2.0f);
    Vec2 b(1.0f, 2.0f);
    Vec2 c(1.0f, 3.0f);
    EXPECT_TRUE(a == b);
    EXPECT_FALSE(a == c);
    EXPECT_TRUE(a != c);
}

TEST(Vec2Test, StaticHelpers) {
    Vec2 zero = Vec2::zero();
    EXPECT_FLOAT_EQ(zero.x, 0.0f);
    EXPECT_FLOAT_EQ(zero.y, 0.0f);
    
    Vec2 one = Vec2::one();
    EXPECT_FLOAT_EQ(one.x, 1.0f);
    EXPECT_FLOAT_EQ(one.y, 1.0f);
}

//=============================================================================
// Rect Tests
//=============================================================================

TEST(RectTest, DefaultConstructor) {
    Rect r;
    EXPECT_FLOAT_EQ(r.x(), 0.0f);
    EXPECT_FLOAT_EQ(r.y(), 0.0f);
    EXPECT_FLOAT_EQ(r.width(), 0.0f);
    EXPECT_FLOAT_EQ(r.height(), 0.0f);
}

TEST(RectTest, ParameterizedConstructor) {
    Rect r(10.0f, 20.0f, 100.0f, 50.0f);
    EXPECT_FLOAT_EQ(r.x(), 10.0f);
    EXPECT_FLOAT_EQ(r.y(), 20.0f);
    EXPECT_FLOAT_EQ(r.width(), 100.0f);
    EXPECT_FLOAT_EQ(r.height(), 50.0f);
}

TEST(RectTest, Edges) {
    Rect r(10.0f, 20.0f, 100.0f, 50.0f);
    EXPECT_FLOAT_EQ(r.left(), 10.0f);
    EXPECT_FLOAT_EQ(r.top(), 20.0f);
    EXPECT_FLOAT_EQ(r.right(), 110.0f);
    EXPECT_FLOAT_EQ(r.bottom(), 70.0f);
}

TEST(RectTest, Corners) {
    Rect r(10.0f, 20.0f, 100.0f, 50.0f);
    
    Vec2 tl = r.topLeft();
    EXPECT_FLOAT_EQ(tl.x, 10.0f);
    EXPECT_FLOAT_EQ(tl.y, 20.0f);
    
    Vec2 br = r.bottomRight();
    EXPECT_FLOAT_EQ(br.x, 110.0f);
    EXPECT_FLOAT_EQ(br.y, 70.0f);
}

TEST(RectTest, Center) {
    Rect r(0.0f, 0.0f, 100.0f, 50.0f);
    Vec2 c = r.center();
    EXPECT_FLOAT_EQ(c.x, 50.0f);
    EXPECT_FLOAT_EQ(c.y, 25.0f);
}

TEST(RectTest, ContainsPoint) {
    Rect r(10.0f, 10.0f, 100.0f, 100.0f);
    
    // Inside
    EXPECT_TRUE(r.contains(Vec2(50.0f, 50.0f)));
    EXPECT_TRUE(r.contains(50.0f, 50.0f));
    
    // Edges (left and top are inclusive)
    EXPECT_TRUE(r.contains(10.0f, 10.0f));
    
    // Right and bottom are exclusive
    EXPECT_FALSE(r.contains(110.0f, 50.0f));
    EXPECT_FALSE(r.contains(50.0f, 110.0f));
    
    // Outside
    EXPECT_FALSE(r.contains(0.0f, 0.0f));
    EXPECT_FALSE(r.contains(200.0f, 200.0f));
}

TEST(RectTest, Intersects) {
    Rect a(0.0f, 0.0f, 100.0f, 100.0f);
    
    // Overlapping
    Rect b(50.0f, 50.0f, 100.0f, 100.0f);
    EXPECT_TRUE(a.intersects(b));
    EXPECT_TRUE(b.intersects(a));
    
    // Touching edges (no intersection)
    Rect c(100.0f, 0.0f, 100.0f, 100.0f);
    EXPECT_FALSE(a.intersects(c));
    
    // Completely separate
    Rect d(200.0f, 200.0f, 50.0f, 50.0f);
    EXPECT_FALSE(a.intersects(d));
    
    // One inside another
    Rect e(25.0f, 25.0f, 50.0f, 50.0f);
    EXPECT_TRUE(a.intersects(e));
}

TEST(RectTest, Expanded) {
    Rect r(10.0f, 10.0f, 100.0f, 100.0f);
    Rect expanded = r.expanded(5.0f);
    
    EXPECT_FLOAT_EQ(expanded.x(), 5.0f);
    EXPECT_FLOAT_EQ(expanded.y(), 5.0f);
    EXPECT_FLOAT_EQ(expanded.width(), 110.0f);
    EXPECT_FLOAT_EQ(expanded.height(), 110.0f);
}

TEST(RectTest, Shrunk) {
    Rect r(10.0f, 10.0f, 100.0f, 100.0f);
    Rect shrunk = r.shrunk(5.0f);
    
    EXPECT_FLOAT_EQ(shrunk.x(), 15.0f);
    EXPECT_FLOAT_EQ(shrunk.y(), 15.0f);
    EXPECT_FLOAT_EQ(shrunk.width(), 90.0f);
    EXPECT_FLOAT_EQ(shrunk.height(), 90.0f);
}

TEST(RectTest, Translated) {
    Rect r(10.0f, 20.0f, 100.0f, 50.0f);
    Rect translated = r.translated(Vec2(5.0f, -10.0f));
    
    EXPECT_FLOAT_EQ(translated.x(), 15.0f);
    EXPECT_FLOAT_EQ(translated.y(), 10.0f);
    EXPECT_FLOAT_EQ(translated.width(), 100.0f);
    EXPECT_FLOAT_EQ(translated.height(), 50.0f);
}

TEST(RectTest, Clipped) {
    Rect r(0.0f, 0.0f, 100.0f, 100.0f);
    Rect clipRect(25.0f, 25.0f, 50.0f, 50.0f);
    Rect clipped = r.clipped(clipRect);
    
    EXPECT_FLOAT_EQ(clipped.x(), 25.0f);
    EXPECT_FLOAT_EQ(clipped.y(), 25.0f);
    EXPECT_FLOAT_EQ(clipped.width(), 50.0f);
    EXPECT_FLOAT_EQ(clipped.height(), 50.0f);
}

TEST(RectTest, ClippedNoIntersection) {
    Rect r(0.0f, 0.0f, 50.0f, 50.0f);
    Rect clipRect(100.0f, 100.0f, 50.0f, 50.0f);
    Rect clipped = r.clipped(clipRect);
    
    EXPECT_FLOAT_EQ(clipped.width(), 0.0f);
    EXPECT_FLOAT_EQ(clipped.height(), 0.0f);
}

TEST(RectTest, FromMinMax) {
    Rect r = Rect::fromMinMax(Vec2(10.0f, 20.0f), Vec2(110.0f, 70.0f));
    EXPECT_FLOAT_EQ(r.x(), 10.0f);
    EXPECT_FLOAT_EQ(r.y(), 20.0f);
    EXPECT_FLOAT_EQ(r.width(), 100.0f);
    EXPECT_FLOAT_EQ(r.height(), 50.0f);
}

//=============================================================================
// Color Tests
//=============================================================================

TEST(ColorTest, DefaultConstructor) {
    Color c;
    EXPECT_EQ(c.r, 255);
    EXPECT_EQ(c.g, 255);
    EXPECT_EQ(c.b, 255);
    EXPECT_EQ(c.a, 255);
}

TEST(ColorTest, ParameterizedConstructor) {
    Color c(100, 150, 200, 128);
    EXPECT_EQ(c.r, 100);
    EXPECT_EQ(c.g, 150);
    EXPECT_EQ(c.b, 200);
    EXPECT_EQ(c.a, 128);
}

TEST(ColorTest, DefaultAlpha) {
    Color c(100, 150, 200);
    EXPECT_EQ(c.a, 255);
}

TEST(ColorTest, FromHex) {
    Color c = Color::fromHex(0xFF5500);
    EXPECT_EQ(c.r, 255);
    EXPECT_EQ(c.g, 85);
    EXPECT_EQ(c.b, 0);
    EXPECT_EQ(c.a, 255);
}

TEST(ColorTest, FromHexWithAlpha) {
    Color c = Color::fromHex(0xFF550080, true);
    EXPECT_EQ(c.r, 255);
    EXPECT_EQ(c.g, 85);
    EXPECT_EQ(c.b, 0);
    EXPECT_EQ(c.a, 128);
}

TEST(ColorTest, FromFloat) {
    Color c = Color::fromFloat(1.0f, 0.5f, 0.0f, 0.5f);
    EXPECT_EQ(c.r, 255);
    EXPECT_NEAR(c.g, 127, 1);
    EXPECT_EQ(c.b, 0);
    EXPECT_NEAR(c.a, 127, 1);
}

TEST(ColorTest, FromFloatClamped) {
    Color c = Color::fromFloat(2.0f, -1.0f, 0.5f);
    EXPECT_EQ(c.r, 255);  // Clamped from 2.0
    EXPECT_EQ(c.g, 0);    // Clamped from -1.0
}

TEST(ColorTest, ToFloat) {
    Color c(255, 127, 0, 255);
    EXPECT_FLOAT_EQ(c.rf(), 1.0f);
    EXPECT_NEAR(c.gf(), 0.498f, 0.01f);
    EXPECT_FLOAT_EQ(c.bf(), 0.0f);
    EXPECT_FLOAT_EQ(c.af(), 1.0f);
}

TEST(ColorTest, ToRGBA) {
    Color c(0x12, 0x34, 0x56, 0x78);
    uint32_t rgba = c.toRGBA();
    EXPECT_EQ(rgba, 0x12345678);
}

TEST(ColorTest, ToABGR) {
    Color c(0x12, 0x34, 0x56, 0x78);
    uint32_t abgr = c.toABGR();
    EXPECT_EQ(abgr, 0x78563412);
}

TEST(ColorTest, WithAlphaUint8) {
    Color c(100, 150, 200, 255);
    Color result = c.withAlpha(static_cast<uint8_t>(128));
    EXPECT_EQ(result.r, 100);
    EXPECT_EQ(result.g, 150);
    EXPECT_EQ(result.b, 200);
    EXPECT_EQ(result.a, 128);
}

TEST(ColorTest, WithAlphaFloat) {
    Color c(100, 150, 200, 200);
    Color result = c.withAlpha(0.5f);
    EXPECT_EQ(result.a, 100);
}

TEST(ColorTest, Lerp) {
    Color a = Color::black();
    Color b = Color::white();
    
    Color mid = Color::lerp(a, b, 0.5f);
    EXPECT_NEAR(mid.r, 127, 1);
    EXPECT_NEAR(mid.g, 127, 1);
    EXPECT_NEAR(mid.b, 127, 1);
    
    Color start = Color::lerp(a, b, 0.0f);
    EXPECT_EQ(start.r, 0);
    
    Color end = Color::lerp(a, b, 1.0f);
    EXPECT_EQ(end.r, 255);
}

TEST(ColorTest, Equality) {
    Color a(100, 150, 200, 255);
    Color b(100, 150, 200, 255);
    Color c(100, 150, 200, 128);
    
    EXPECT_TRUE(a == b);
    EXPECT_FALSE(a == c);
    EXPECT_TRUE(a != c);
}

TEST(ColorTest, StaticColors) {
    EXPECT_EQ(Color::white(), Color(255, 255, 255, 255));
    EXPECT_EQ(Color::black(), Color(0, 0, 0, 255));
    EXPECT_EQ(Color::transparent(), Color(0, 0, 0, 0));
    EXPECT_EQ(Color::red(), Color(255, 0, 0, 255));
    EXPECT_EQ(Color::green(), Color(0, 255, 0, 255));
    EXPECT_EQ(Color::blue(), Color(0, 0, 255, 255));
}

TEST(ColorTest, FromHSL) {
    // Red at 0 degrees
    Color red = Color::fromHSL(0.0f, 1.0f, 0.5f);
    EXPECT_NEAR(red.r, 255, 2);
    EXPECT_NEAR(red.g, 0, 2);
    EXPECT_NEAR(red.b, 0, 2);
    
    // Green at 120 degrees (1/3)
    Color green = Color::fromHSL(1.0f/3.0f, 1.0f, 0.5f);
    EXPECT_NEAR(green.r, 0, 2);
    EXPECT_NEAR(green.g, 255, 2);
    EXPECT_NEAR(green.b, 0, 2);
    
    // Gray (saturation 0)
    Color gray = Color::fromHSL(0.0f, 0.0f, 0.5f);
    EXPECT_NEAR(gray.r, 127, 2);
    EXPECT_NEAR(gray.g, 127, 2);
    EXPECT_NEAR(gray.b, 127, 2);
}

TEST(ColorTest, FromHSV) {
    // Red at 0 degrees, full saturation, full value
    Color red = Color::fromHSV(0.0f, 1.0f, 1.0f);
    EXPECT_NEAR(red.r, 255, 2);
    EXPECT_NEAR(red.g, 0, 2);
    EXPECT_NEAR(red.b, 0, 2);
    
    // Blue at 240 degrees (2/3)
    Color blue = Color::fromHSV(2.0f/3.0f, 1.0f, 1.0f);
    EXPECT_NEAR(blue.r, 0, 2);
    EXPECT_NEAR(blue.g, 0, 2);
    EXPECT_NEAR(blue.b, 255, 2);
}

TEST(ColorTest, ToHSV) {
    Color red(255, 0, 0);
    float h, s, v;
    red.toHSV(h, s, v);
    
    EXPECT_NEAR(h, 0.0f, 0.01f);  // Red is at 0 degrees
    EXPECT_NEAR(s, 1.0f, 0.01f);  // Full saturation
    EXPECT_NEAR(v, 1.0f, 0.01f);  // Full value
}

TEST(ColorTest, HsvRoundTrip) {
    // Test that fromHSV -> toHSV gives back original values
    float origH = 0.3f, origS = 0.7f, origV = 0.9f;
    Color c = Color::fromHSV(origH, origS, origV);
    
    float h, s, v;
    c.toHSV(h, s, v);
    
    EXPECT_NEAR(h, origH, 0.02f);
    EXPECT_NEAR(s, origS, 0.02f);
    EXPECT_NEAR(v, origV, 0.02f);
}

TEST(ColorTest, Lighter) {
    Color c(100, 100, 100);
    Color lighter = c.lighter(0.5f);
    
    EXPECT_GT(lighter.r, c.r);
    EXPECT_GT(lighter.g, c.g);
    EXPECT_GT(lighter.b, c.b);
    EXPECT_EQ(lighter.a, c.a);  // Alpha unchanged
}

TEST(ColorTest, LighterClamp) {
    Color c(200, 200, 200);
    Color lighter = c.lighter(0.5f);  // Would exceed 255
    
    EXPECT_EQ(lighter.r, 255);  // Clamped
    EXPECT_EQ(lighter.g, 255);
    EXPECT_EQ(lighter.b, 255);
}

TEST(ColorTest, Darker) {
    Color c(100, 100, 100);
    Color darker = c.darker(0.5f);
    
    EXPECT_LT(darker.r, c.r);
    EXPECT_LT(darker.g, c.g);
    EXPECT_LT(darker.b, c.b);
    EXPECT_EQ(darker.a, c.a);  // Alpha unchanged
}

//=============================================================================
// WidgetId Tests
//=============================================================================

TEST(WidgetIdTest, HashString) {
    WidgetId id1 = hashString("button1");
    WidgetId id2 = hashString("button2");
    WidgetId id1_again = hashString("button1");
    
    EXPECT_NE(id1, id2);
    EXPECT_EQ(id1, id1_again);
    EXPECT_NE(id1, INVALID_WIDGET_ID);
}

TEST(WidgetIdTest, CombineIds) {
    WidgetId parent = hashString("panel");
    WidgetId child1 = hashString("button1");
    WidgetId child2 = hashString("button2");
    
    WidgetId combined1 = combineIds(parent, child1);
    WidgetId combined2 = combineIds(parent, child2);
    
    EXPECT_NE(combined1, combined2);
    EXPECT_NE(combined1, parent);
    EXPECT_NE(combined1, child1);
}
