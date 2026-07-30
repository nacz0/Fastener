#include <gtest/gtest.h>
#include <fastener/core/context.h>
#include <fastener/graphics/draw_list.h>
#include <fastener/graphics/font.h>
#include <fastener/ui/dock_context.h>
#include <fastener/ui/flex_layout.h>
#include <fastener/ui/theme.h>
#include <fastener/widgets/dock_preview.h>
#include <fastener/widgets/modal.h>
#include <fastener/widgets/panel.h>
#include <fastener/widgets/status_bar.h>
#include <fastener/widgets/toast.h>
#include <fastener/widgets/tooltip.h>
#include <fastener/widgets/tree_view.h>
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

TEST(TreeViewSimpleContextStateTest, MatchingIdsRetainIndependentScrollState) {
    Context first(false);
    Context second(false);
    fst::testing::StubWindow firstWindow;
    fst::testing::StubWindow secondWindow;
    ASSERT_TRUE(first.loadFont(testFontPath(), 16.0f));
    ASSERT_TRUE(second.loadFont(testFontPath(), 16.0f));

    TreeNode firstRoot("first-root", "First root");
    TreeNode secondRoot("second-root", "Second root");
    for (int index = 0; index < 5; ++index) {
        firstRoot.addChild(
            "first-" + std::to_string(index),
            "First " + std::to_string(index),
            true);
        secondRoot.addChild(
            "second-" + std::to_string(index),
            "Second " + std::to_string(index),
            true);
    }

    const Rect bounds(0.0f, 40.0f, 120.0f, 48.0f);
    TreeViewOptions options;
    options.rowHeight = 24.0f;
    options.showIcons = false;

    firstWindow.input().beginFrame();
    first.beginFrame(firstWindow);
    firstWindow.input().onMouseMove(50.0f, 52.0f);
    firstWindow.input().onMouseScroll(0.0f, -1.0f);
    TreeViewSimple(first, "shared-tree", &firstRoot, bounds, nullptr, options);
    first.endFrame();

    secondWindow.input().beginFrame();
    second.beginFrame(secondWindow);
    TreeViewSimple(second, "shared-tree", &secondRoot, bounds, nullptr, options);
    second.endFrame();

    std::string selectedId;
    firstWindow.input().beginFrame();
    first.beginFrame(firstWindow);
    firstWindow.input().onMouseMove(50.0f, 52.0f);
    firstWindow.input().onMouseDown(MouseButton::Left);
    TreeViewSimple(
        first,
        "shared-tree",
        &firstRoot,
        bounds,
        [&](TreeNode* node) { selectedId = node->id; },
        options);
    first.endFrame();

    EXPECT_EQ(selectedId, "first-3");
}

TEST(ContextFontLifetimeTest, FailedReloadPreservesTheCurrentFont) {
    Context ctx(false);
    ASSERT_TRUE(ctx.loadFont(testFontPath(), 16.0f));
    Font* originalFont = ctx.font();
    ASSERT_NE(originalFont, nullptr);

    EXPECT_FALSE(ctx.loadFont("this/font/does/not/exist.ttf", 18.0f));

    EXPECT_EQ(ctx.font(), originalFont);
    EXPECT_EQ(ctx.defaultFont(), originalFont);
}

TEST(FontLifetimeTest, InvalidMemoryReloadPreservesValidFont) {
    Font font;
    ASSERT_TRUE(font.loadFromFile(testFontPath(), 16.0f));
    const Vec2 originalMeasurement = font.measureText("Fastener");
    const std::uint8_t invalidData[] = {0x00, 0x01, 0x02, 0x03};

    EXPECT_FALSE(font.loadFromMemory(invalidData, sizeof(invalidData), 18.0f));

    EXPECT_TRUE(font.isValid());
    EXPECT_EQ(font.measureText("Fastener"), originalMeasurement);

    EXPECT_FALSE(font.loadFromMemory(nullptr, 4, 18.0f));
    EXPECT_TRUE(font.isValid());
    EXPECT_EQ(font.measureText("Fastener"), originalMeasurement);
}

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
    const WidgetId firstRootId = first.currentId();
    ASSERT_TRUE(BeginModal(first, "FirstModal", firstOpen));
    ASSERT_NE(first.currentId(), firstRootId);

    second.beginFrame(secondWindow);
    EXPECT_FALSE(BeginModal(second, "SecondModal", secondOpen));
    second.endFrame();

    EndModal(first);
    EXPECT_EQ(first.currentId(), firstRootId);
    first.endFrame();
}

TEST(ModalContextStateTest, NestedModalsRestoreIdsAndCallingDrawLayer) {
    Context ctx(false);
    fst::testing::StubWindow window;
    bool outerOpen = true;
    bool innerOpen = true;

    ctx.beginFrame(window);
    const WidgetId rootId = ctx.currentId();
    ctx.drawList().setLayer(DrawLayer::Floating);

    ASSERT_TRUE(BeginModal(ctx, "OuterModal", outerOpen));
    WidgetId outerId = ctx.currentId();
    ASSERT_NE(outerId, rootId);

    ASSERT_TRUE(BeginModal(ctx, "InnerModal", innerOpen));
    ASSERT_NE(ctx.currentId(), outerId);

    EndModal(ctx);
    EXPECT_EQ(ctx.currentId(), outerId);
    EXPECT_EQ(ctx.drawList().currentLayer(), DrawLayer::Overlay);

    EndModal(ctx);
    EXPECT_EQ(ctx.currentId(), rootId);
    EXPECT_EQ(ctx.drawList().currentLayer(), DrawLayer::Floating);
    ctx.endFrame();
}

TEST(ModalContextStateTest, BackdropCloseCleansPartiallyOpenedScope) {
    Context ctx(false);
    fst::testing::StubWindow window;
    bool isOpen = true;

    window.input().beginFrame();
    window.input().onMouseMove(0.0f, 0.0f);
    window.input().onMouseDown(MouseButton::Left);
    ctx.beginFrame(window);
    const WidgetId rootId = ctx.currentId();
    ctx.drawList().setLayer(DrawLayer::Floating);

    EXPECT_FALSE(BeginModal(ctx, "BackdropCloseModal", isOpen));
    EXPECT_FALSE(isOpen);
    EXPECT_EQ(ctx.currentId(), rootId);
    EXPECT_EQ(ctx.drawList().currentLayer(), DrawLayer::Floating);

    EndModal(ctx);
    ctx.endFrame();
}

TEST(ModalContextStateTest, CloseButtonCanCloseAndCleanItsScope) {
    Context ctx(false);
    fst::testing::StubWindow window;
    ASSERT_TRUE(ctx.loadFont(testFontPath(), 16.0f));
    bool isOpen = true;

    ModalOptions options;
    options.title = "Closable";
    Font* font = ctx.font();
    ASSERT_NE(font, nullptr);

    const float padding = ctx.theme().metrics.paddingMedium;
    const float titleHeight = font->lineHeight() + padding * 2.0f;
    const float modalX =
        (static_cast<float>(window.width()) - options.width) * 0.5f;
    const float modalY =
        (static_cast<float>(window.height()) - 200.0f) * 0.5f;
    const float closeSize = font->lineHeight();
    const float closeX =
        modalX + options.width - padding - closeSize * 0.5f;
    const float closeY = modalY + titleHeight * 0.5f;

    window.input().beginFrame();
    window.input().onMouseMove(closeX, closeY);
    window.input().onMouseDown(MouseButton::Left);
    window.input().onMouseUp(MouseButton::Left);
    ctx.beginFrame(window);
    const WidgetId rootId = ctx.currentId();
    ctx.drawList().setLayer(DrawLayer::Floating);

    EXPECT_FALSE(BeginModal(ctx, "CloseButtonModal", isOpen, options));
    EXPECT_FALSE(isOpen);
    EXPECT_EQ(ctx.currentId(), rootId);
    EXPECT_EQ(ctx.drawList().currentLayer(), DrawLayer::Floating);

    EndModal(ctx);
    ctx.endFrame();
}

TEST(ModalContextStateTest, ClosedRaiiModalDoesNotEndOuterModal) {
    Context ctx(false);
    fst::testing::StubWindow window;
    bool outerOpen = true;
    bool innerOpen = false;

    ctx.beginFrame(window);
    const WidgetId rootId = ctx.currentId();
    ASSERT_TRUE(BeginModal(ctx, "OuterModal", outerOpen));
    WidgetId outerId = ctx.currentId();

    {
        ModalScope inner(ctx, "ClosedInnerModal", innerOpen);
        EXPECT_FALSE(inner);
    }

    EXPECT_EQ(ctx.currentId(), outerId);
    EXPECT_EQ(ctx.drawList().currentLayer(), DrawLayer::Overlay);

    EndModal(ctx);
    EXPECT_EQ(ctx.currentId(), rootId);
    ctx.endFrame();
}

TEST(OverlayLayerStateTest, ToastRenderingRestoresCallingDrawLayer) {
    Context ctx(false);
    fst::testing::StubWindow window;
    ASSERT_TRUE(ctx.loadFont(testFontPath(), 16.0f));

    ctx.beginFrame(window);
    ctx.drawList().setLayer(DrawLayer::Floating);
    ShowToast(ctx, "Layer-safe toast");

    RenderToasts(ctx);

    EXPECT_EQ(ctx.drawList().currentLayer(), DrawLayer::Floating);
    DismissAllToasts(ctx);
    ctx.endFrame();
}

TEST(OverlayLayerStateTest, DockPreviewRestoresCallingDrawLayer) {
    Context ctx(false);
    fst::testing::StubWindow window;
    DockNode::Id rootId =
        ctx.docking().createDockSpace("Preview", Rect(0, 0, 400, 300));
    auto& drag = ctx.docking().dragState();
    drag.active = true;
    drag.hoveredNodeId = rootId;
    drag.hoveredDirection = DockDirection::Left;
    drag.mousePos = Vec2(20, 150);

    ctx.beginFrame(window);
    ctx.drawList().setLayer(DrawLayer::Floating);

    RenderDockPreview(ctx);

    EXPECT_EQ(ctx.drawList().currentLayer(), DrawLayer::Floating);
    ctx.endFrame();
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
