#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "TestContext.h"
#include <fastener/widgets/pill.h>

using namespace fst;
using namespace fst::testing;
using ::testing::_;

TEST(PillWidgetTest, DrawsFilledPill) {
    TestContext tc;
    tc.beginFrame();

    PillOptions opts;
    opts.style = Style().withPos(0, 0).withSize(100, 24);
    opts.color = Color::fromHex(0x336699);

    EXPECT_CALL(tc.mockDrawList(), addRectFilled(Rect(0, 0, 100, 24), _, _)).Times(1);

    Pill(tc.context(), "Build", opts);

    tc.endFrame();
}

TEST(PillWidgetTest, DrawsOutlinedPill) {
    TestContext tc;
    tc.beginFrame();

    PillOptions opts;
    opts.style = Style().withPos(10, 5).withSize(120, 28);
    opts.outlined = true;

    EXPECT_CALL(tc.mockDrawList(), addRect(Rect(10, 5, 120, 28), _, _)).Times(1);

    Pill(tc.context(), "Outlined", opts);

    tc.endFrame();
}

