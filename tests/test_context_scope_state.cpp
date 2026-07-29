#include <gtest/gtest.h>
#include <fastener/core/context.h>
#include <fastener/graphics/draw_list.h>
#include <fastener/ui/flex_layout.h>
#include <fastener/widgets/modal.h>
#include <fastener/widgets/panel.h>
#include <fastener/widgets/status_bar.h>
#include <fastener/widgets/tooltip.h>
#include "TestContext.h"
#include <filesystem>

using namespace fst;

namespace {

std::string testFontPath() {
    namespace fs = std::filesystem;
    fs::path root = fs::path(__FILE__).parent_path().parent_path();
    return (root / "assets" / "arial.ttf").string();
}

std::size_t renderTopLevelPanel(Context& ctx, fst::testing::StubWindow& window) {
    PanelOptions options;
    options.style.width = 200.0f;
    options.style.height = 100.0f;

    ctx.beginFrame(window);
    EXPECT_TRUE(BeginPanel(ctx, "Panel", options));
    EndPanel(ctx);
    ctx.endFrame();
    return ctx.drawList().vertices().size();
}

std::size_t renderStatusBarWithSection(Context& ctx, fst::testing::StubWindow& window) {
    ctx.beginFrame(window);
    EXPECT_TRUE(BeginStatusBar(ctx));
    StatusBarSection(ctx, "Ready");
    EndStatusBar(ctx);
    ctx.endFrame();
    return ctx.drawList().vertices().size();
}

} // namespace

TEST(PanelContextStateTest, OpenPanelInAnotherContextIsStillTopLevel) {
    Context first(false);
    Context second(false);
    Context baseline(false);
    fst::testing::StubWindow firstWindow;
    fst::testing::StubWindow secondWindow;
    fst::testing::StubWindow baselineWindow;

    PanelOptions options;
    options.style.width = 200.0f;
    options.style.height = 100.0f;

    first.beginFrame(firstWindow);
    ASSERT_TRUE(BeginPanel(first, "FirstPanel", options));

    second.beginFrame(secondWindow);
    ASSERT_TRUE(BeginPanel(second, "SecondPanel", options));
    EndPanel(second);
    second.endFrame();

    EndPanel(first);
    first.endFrame();

    const std::size_t baselineVertexCount = renderTopLevelPanel(baseline, baselineWindow);
    EXPECT_EQ(second.drawList().vertices().size(), baselineVertexCount);
}

TEST(ModalContextStateTest, ClosedModalInAnotherContextCannotPreventScopeCleanup) {
    Context first(false);
    Context second(false);
    fst::testing::StubWindow firstWindow;
    fst::testing::StubWindow secondWindow;
    bool firstOpen = true;
    bool secondOpen = false;

    first.beginFrame(firstWindow);
    ASSERT_TRUE(BeginModal(first, "FirstModal", firstOpen));
    ASSERT_NE(first.currentId(), WidgetId{0});

    second.beginFrame(secondWindow);
    EXPECT_FALSE(BeginModal(second, "SecondModal", secondOpen));
    second.endFrame();

    EndModal(first);
    EXPECT_EQ(first.currentId(), WidgetId{0});
    first.endFrame();
}

TEST(StatusBarContextStateTest, EndingAnotherContextDoesNotDisableActiveStatusBar) {
    Context first(false);
    Context second(false);
    Context baseline(false);
    fst::testing::StubWindow firstWindow;
    fst::testing::StubWindow secondWindow;
    fst::testing::StubWindow baselineWindow;
    ASSERT_TRUE(first.loadFont(testFontPath(), 16.0f));
    ASSERT_TRUE(baseline.loadFont(testFontPath(), 16.0f));

    first.beginFrame(firstWindow);
    ASSERT_TRUE(BeginStatusBar(first));

    second.beginFrame(secondWindow);
    ASSERT_TRUE(BeginStatusBar(second));
    EndStatusBar(second);
    second.endFrame();

    StatusBarSection(first, "Ready");
    EndStatusBar(first);
    first.endFrame();

    const std::size_t baselineVertexCount =
        renderStatusBarWithSection(baseline, baselineWindow);
    EXPECT_EQ(first.drawList().vertices().size(), baselineVertexCount);
}

TEST(GridContextStateTest, AllocationUsesTheCallingContextsActiveGrid) {
    Context first(false);
    Context second(false);
    fst::testing::StubWindow firstWindow;
    fst::testing::StubWindow secondWindow;
    GridOptions options;
    options.columns = 2;
    options.style.width = 100.0f;
    options.style.height = 100.0f;

    first.beginFrame(firstWindow);
    BeginGrid(first, options);
    const Rect firstItem = allocateWidgetBounds(first, Style{}, 10.0f, 10.0f);

    second.beginFrame(secondWindow);
    BeginGrid(second, options);

    const Rect secondItemInFirst = allocateWidgetBounds(first, Style{}, 10.0f, 10.0f);
    EXPECT_GT(secondItemInFirst.x(), firstItem.x());

    EndGrid(second);
    second.endFrame();
    EndGrid(first);
    first.endFrame();
}

TEST(TooltipContextStateTest, HoverTrackingIsIndependentForEachContext) {
    Context first(false);
    Context second(false);
    constexpr WidgetId sharedWidgetId = 42;

    first.setHoveredWidget(sharedWidgetId);
    Tooltip(first, "First tooltip");

    EXPECT_EQ(internal::getTooltipState(first).hoveredWidget, sharedWidgetId);
    EXPECT_EQ(internal::getTooltipState(second).hoveredWidget, INVALID_WIDGET_ID);
}
