#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "TestContext.h"
#include <fastener/widgets/svg_image.h>

using namespace fst;
using namespace fst::testing;
using ::testing::_;

TEST(SvgImageWidgetTest, DrawsSvgContent) {
    TestContext tc;
    tc.beginFrame();

    SvgDocument doc;
    ASSERT_TRUE(doc.loadFromMemory(R"(
        <svg viewBox="0 0 10 10">
            <rect x="0" y="0" width="10" height="10" fill="#ff0000" />
        </svg>
    )"));

    SvgImageOptions opts;
    opts.style = Style().withPos(0, 0).withSize(10, 10);

    EXPECT_CALL(tc.mockDrawList(), addRectFilled(Rect(0, 0, 10, 10), Color(255, 0, 0, 255), 0.0f)).Times(1);

    SvgImage(tc.context(), &doc, opts);

    tc.endFrame();
}

TEST(SvgImageWidgetTest, DrawsPlaceholderWhenInvalid) {
    TestContext tc;
    tc.beginFrame();

    SvgDocument doc;

    SvgImageOptions opts;
    opts.style = Style().withPos(0, 0).withSize(20, 20);

    EXPECT_CALL(tc.mockDrawList(), addRectFilled(Rect(0, 0, 20, 20), _, _)).Times(1);

    SvgImage(tc.context(), &doc, opts);

    tc.endFrame();
}
