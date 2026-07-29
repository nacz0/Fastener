#include <gtest/gtest.h>

#include <fastener/core/context.h>
#include <fastener/widgets/combo_box.h>
#include <fastener/widgets/listbox.h>

#include "TestContext.h"

using namespace fst;

TEST(ComboBoxContextTest, OpenDropdownDoesNotLeakToMatchingIdInAnotherContext) {
    fst::testing::StubWindow firstWindow;
    fst::testing::StubWindow secondWindow;
    Context firstContext(false);
    Context secondContext(false);
    const std::vector<std::string> items = {"Zero", "One", "Two"};
    ComboBoxOptions options;
    options.style = Style().withPos(0, 0).withSize(150, 28);

    int firstSelection = 1;

    firstWindow.input().beginFrame();
    firstContext.beginFrame(firstWindow);
    firstWindow.input().onMouseMove(10, 10);
    firstWindow.input().onMouseDown(MouseButton::Left);
    EXPECT_FALSE(ComboBox(firstContext, "context-shared-combo", firstSelection, items, options));
    firstContext.endFrame();

    firstWindow.input().beginFrame();
    firstContext.beginFrame(firstWindow);
    firstWindow.input().onMouseMove(10, 10);
    firstWindow.input().onMouseUp(MouseButton::Left);
    EXPECT_FALSE(ComboBox(firstContext, "context-shared-combo", firstSelection, items, options));
    firstContext.endFrame();

    int secondSelection = 1;
    secondWindow.input().beginFrame();
    secondContext.beginFrame(secondWindow);
    secondWindow.input().onMouseMove(10, 35);
    secondWindow.input().onMouseDown(MouseButton::Left);
    EXPECT_FALSE(ComboBox(secondContext, "context-shared-combo", secondSelection, items, options));
    secondContext.endFrame();

    EXPECT_EQ(secondSelection, 1);
}

TEST(ListboxContextTest, ScrollOffsetDoesNotChangeHitTestingInAnotherContext) {
    fst::testing::StubWindow firstWindow;
    fst::testing::StubWindow secondWindow;
    Context firstContext(false);
    Context secondContext(false);
    const std::vector<std::string> items = {
        "Zero", "One", "Two", "Three", "Four",
    };
    ListboxOptions options;
    options.style = Style().withPos(0, 0).withSize(100, 48);
    options.itemHeight = 24.0f;

    int firstSelection = -1;
    firstWindow.input().beginFrame();
    firstContext.beginFrame(firstWindow);
    firstWindow.input().onMouseMove(10, 12);
    firstWindow.input().onMouseScroll(0, -1);
    EXPECT_FALSE(Listbox(firstContext, "context-shared-listbox", firstSelection, items, options));
    firstContext.endFrame();

    int secondSelection = -1;
    secondWindow.input().beginFrame();
    secondContext.beginFrame(secondWindow);
    secondWindow.input().onMouseMove(10, 12);
    secondWindow.input().onMouseDown(MouseButton::Left);
    EXPECT_TRUE(Listbox(secondContext, "context-shared-listbox", secondSelection, items, options));
    secondContext.endFrame();

    EXPECT_EQ(secondSelection, 0);
}
