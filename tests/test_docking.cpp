#include <gtest/gtest.h>
#include <fastener/ui/dock_context.h>
#include <fastener/ui/dock_builder.h>
#include <fastener/ui/dock_node.h>
#include <fastener/core/context.h>
#include <fastener/widgets/dock_space.h>
#include <fastener/widgets/dockable_window.h>
#include "TestContext.h"
#include <limits>

using namespace fst;

class DockingTest : public ::testing::Test {
protected:
    void SetUp() override {
    }
};

TEST_F(DockingTest, MultipleSplitsDoNotRecursivelyLoop) {
    DockContext docking;
    Rect rootBounds(0, 0, 1000, 1000);
    
    // Create root dock space
    DockNode::Id rootId = docking.createDockSpace("Main", rootBounds);
    EXPECT_NE(rootId, DockNode::INVALID_ID);
    
    const DockNode::Id newNodeId =
        docking.splitNode(rootId, DockDirection::Left, 0.5f);
    ASSERT_NE(newNodeId, DockNode::INVALID_ID);

    const DockNode::Id nestedNodeId =
        docking.splitNode(newNodeId, DockDirection::Top, 0.5f);
    ASSERT_NE(nestedNodeId, DockNode::INVALID_ID);
    
    // If we reached here without stack overflow, it's a good sign
    EXPECT_TRUE(true);
}

TEST_F(DockingTest, IdUniqueness) {
    DockContext docking;
    std::vector<DockNode::Id> ids;
    DockNode::Id leafId =
        docking.createDockSpace("Main", Rect(0, 0, 1000, 1000));
    ids.push_back(leafId);

    for (int i = 0; i < 100; ++i) {
        leafId = docking.splitNode(leafId, DockDirection::Left);
        ASSERT_NE(leafId, DockNode::INVALID_ID);
        ids.push_back(leafId);
    }
    
    // Check for duplicates
    for (size_t i = 0; i < ids.size(); ++i) {
        for (size_t j = i + 1; j < ids.size(); ++j) {
            EXPECT_NE(ids[i], ids[j]) << "Duplicate ID found at " << i << " and " << j;
        }
    }
}

TEST_F(DockingTest, InvalidMoveTargetPreservesExistingDocking) {
    DockContext docking;
    const DockNode::Id rootId =
        docking.createDockSpace("Main", Rect(0, 0, 1000, 800));
    constexpr WidgetId windowId = 101;
    docking.dockWindow(windowId, rootId);
    DockNode* originalNode = docking.getWindowDockNode(windowId);
    ASSERT_NE(originalNode, nullptr);
    const DockNode::Id originalNodeId = originalNode->id();

    docking.dockWindow(windowId, DockNode::INVALID_ID);

    EXPECT_TRUE(docking.isWindowDocked(windowId));
    ASSERT_NE(docking.getWindowDockNode(windowId), nullptr);
    EXPECT_EQ(docking.getWindowDockNode(windowId)->id(), originalNodeId);
    EXPECT_TRUE(docking.getWindowDockNode(windowId)->hasWindow(windowId));
}

TEST_F(DockingTest, SplitContainerCannotBeUsedAsALeafDockTarget) {
    DockContext docking;
    const DockNode::Id rootId =
        docking.createDockSpace("Main", Rect(0, 0, 1000, 800));
    DockNode* root = docking.getDockNode(rootId);
    ASSERT_NE(root, nullptr);
    const DockNode::Id leftId =
        docking.splitNode(rootId, DockDirection::Left);
    DockNode* left = docking.getDockNode(leftId);
    ASSERT_NE(left, nullptr);

    constexpr WidgetId windowId = 202;
    docking.dockWindow(windowId, left->id());
    DockNode* originalNode = docking.getWindowDockNode(windowId);
    ASSERT_EQ(originalNode, left);
    const DockNode::Id originalNodeId = originalNode->id();

    docking.dockWindow(windowId, rootId, DockDirection::Center);

    ASSERT_NE(docking.getWindowDockNode(windowId), nullptr);
    EXPECT_EQ(docking.getWindowDockNode(windowId)->id(), originalNodeId);
    EXPECT_TRUE(docking.getWindowDockNode(windowId)->hasWindow(windowId));
    EXPECT_TRUE(root->isSplitNode());
    EXPECT_FALSE(root->hasWindow(windowId));
}

TEST_F(DockingTest, MovingBetweenSiblingLeavesSurvivesSourceCollapse) {
    DockContext docking;
    const DockNode::Id rootId =
        docking.createDockSpace("Main", Rect(0, 0, 1000, 800));
    DockNode* root = docking.getDockNode(rootId);
    ASSERT_NE(root, nullptr);
    const DockNode::Id leftId =
        docking.splitNode(rootId, DockDirection::Left);
    DockNode* left = docking.getDockNode(leftId);
    ASSERT_NE(left, nullptr);
    ASSERT_NE(root->child(1), nullptr);
    const DockNode::Id rightId = root->child(1)->id();

    constexpr WidgetId movedWindow = 401;
    constexpr WidgetId existingWindow = 402;
    docking.dockWindow(movedWindow, left->id());
    docking.dockWindow(existingWindow, rightId);

    docking.dockWindow(movedWindow, rightId, DockDirection::Center);

    const DockNode* destination = docking.getWindowDockNode(movedWindow);
    ASSERT_NE(destination, nullptr);
    EXPECT_TRUE(destination->hasWindow(movedWindow));
    EXPECT_TRUE(destination->hasWindow(existingWindow));
    EXPECT_EQ(docking.getWindowDockNode(existingWindow), destination);
}

TEST_F(DockingTest, ExistingSplitTreeCannotBeDestructivelySplitAgain) {
    DockContext docking;
    const DockNode::Id rootId =
        docking.createDockSpace("Main", Rect(0, 0, 100, 100));
    docking.dockWindow(303, rootId);
    ASSERT_NE(
        docking.splitNode(rootId, DockDirection::Left),
        DockNode::INVALID_ID);
    DockNode* root = docking.getDockNode(rootId);
    ASSERT_NE(root->child(0), nullptr);
    ASSERT_NE(root->child(1), nullptr);
    const DockNode::Id firstChildId = root->child(0)->id();
    const DockNode::Id secondChildId = root->child(1)->id();

    EXPECT_EQ(
        docking.splitNode(rootId, DockDirection::Top),
        DockNode::INVALID_ID);

    ASSERT_NE(root->child(0), nullptr);
    ASSERT_NE(root->child(1), nullptr);
    EXPECT_EQ(root->child(0)->id(), firstChildId);
    EXPECT_EQ(root->child(1)->id(), secondChildId);
    EXPECT_NE(root->findNodeByWindowId(303), nullptr);
}

TEST_F(DockingTest, NoSplitFlagPreventsSplitting) {
    DockContext docking;
    const DockNode::Id rootId =
        docking.createDockSpace("Main", Rect(0, 0, 100, 100));
    DockNodeFlags flags;
    flags.noSplit = true;
    ASSERT_TRUE(docking.setNodeFlags(rootId, flags));

    EXPECT_EQ(
        docking.splitNode(rootId, DockDirection::Left),
        DockNode::INVALID_ID);
    const DockNode* node = docking.getDockNode(rootId);
    ASSERT_NE(node, nullptr);
    EXPECT_TRUE(node->isLeafNode());
    EXPECT_EQ(node->child(0), nullptr);
    EXPECT_EQ(node->child(1), nullptr);
}

TEST_F(DockingTest, InvalidSplitParametersPreserveLeaf) {
    DockContext docking;
    const DockNode::Id rootId =
        docking.createDockSpace("Main", Rect(0, 0, 100, 100));
    docking.dockWindow(404, rootId);

    EXPECT_EQ(
        docking.splitNode(
            rootId,
            DockDirection::Left,
            std::numeric_limits<float>::quiet_NaN()),
        DockNode::INVALID_ID);
    EXPECT_EQ(
        docking.splitNode(rootId, DockDirection::Left, 0.0f),
        DockNode::INVALID_ID);
    EXPECT_EQ(
        docking.splitNode(rootId, DockDirection::Left, 1.0f),
        DockNode::INVALID_ID);
    EXPECT_EQ(
        docking.splitNode(rootId, DockDirection::None, 0.5f),
        DockNode::INVALID_ID);
    EXPECT_EQ(
        docking.splitNode(rootId, DockDirection::Center, 0.5f),
        DockNode::INVALID_ID);

    const DockNode* node = docking.getDockNode(rootId);
    ASSERT_NE(node, nullptr);
    EXPECT_TRUE(node->isLeafNode());
    EXPECT_TRUE(node->hasWindow(404));
    EXPECT_EQ(node->child(0), nullptr);
    EXPECT_EQ(node->child(1), nullptr);
}

TEST_F(DockingTest, SerializedLayoutRoundTripsTreeAndWindowMappings) {
    DockContext source;
    const Rect bounds(10, 20, 900, 700);
    const DockNode::Id rootId = source.createDockSpace("Main Workspace", bounds);
    source.dockWindow(501, rootId);
    source.dockWindow(502, rootId, DockDirection::Left);

    DockNode* root = source.getDockNode(rootId);
    ASSERT_NE(root, nullptr);
    ASSERT_TRUE(root->isSplitNode());
    ASSERT_TRUE(root->setSplitRatio(0.35f));
    DockNodeFlags flags;
    flags.noResize = true;
    ASSERT_TRUE(source.setNodeFlags(rootId, flags));
    root->updateLayout(bounds);

    const std::string serialized = source.serializeLayout();

    DockContext restored;
    ASSERT_TRUE(restored.deserializeLayout(serialized));
    const DockNode* restoredRoot = restored.getDockSpace("Main Workspace");
    ASSERT_NE(restoredRoot, nullptr);
    EXPECT_EQ(restoredRoot->id(), rootId);
    EXPECT_EQ(restoredRoot->type(), DockNodeType::SplitHorizontal);
    EXPECT_FLOAT_EQ(restoredRoot->splitRatio(), 0.35f);
    EXPECT_TRUE(restoredRoot->flags().noResize);
    EXPECT_EQ(restoredRoot->bounds(), bounds);
    ASSERT_NE(restoredRoot->child(0), nullptr);
    ASSERT_NE(restoredRoot->child(1), nullptr);
    EXPECT_EQ(
        restored.getWindowDockNode(501)->id(),
        root->findNodeByWindowId(501)->id());
    EXPECT_EQ(
        restored.getWindowDockNode(502)->id(),
        root->findNodeByWindowId(502)->id());
}

TEST_F(DockingTest, InvalidSerializedLayoutPreservesCurrentState) {
    DockContext docking;
    const DockNode::Id rootId =
        docking.createDockSpace("Keep", Rect(1, 2, 300, 200));
    docking.dockWindow(601, rootId);
    const std::string original = docking.serializeLayout();

    EXPECT_FALSE(docking.deserializeLayout(
        "FST_DOCK_LAYOUT 1\n"
        "SPACES 1\n"
        "SPACE \"Broken\"\n"
        "NODE 1 4 0 0.5 0 0 0 100 100 0 2\n"
        "NODE 2 4 0 0.5 0 0 0 50 100 0 0\n"));

    EXPECT_EQ(docking.serializeLayout(), original);
    ASSERT_NE(docking.getWindowDockNode(601), nullptr);
    EXPECT_EQ(docking.getWindowDockNode(601)->id(), rootId);
}

TEST_F(DockingTest, SerializedLayoutRejectsUnknownVersionAndDuplicateIds) {
    DockContext docking;

    EXPECT_FALSE(docking.deserializeLayout("FST_DOCK_LAYOUT 99\nSPACES 0\nEND\n"));
    EXPECT_FALSE(docking.deserializeLayout(
        "FST_DOCK_LAYOUT 1\n"
        "SPACES 1\n"
        "SPACE \"Main\"\n"
        "NODE 1 1 0 0.5 0 0 0 100 100 0 2\n"
        "NODE 1 4 0 0.5 0 0 0 50 100 0 0\n"
        "NODE 3 4 0 0.5 0 50 0 50 100 0 0\n"
        "END\n"));
}

TEST_F(DockingTest, ClearDockSpaceRemovesEveryWindowMappingFromSplitLeaves) {
    Context ctx(false);
    DockContext& docking = ctx.docking();
    DockNode::Id rootId = docking.createDockSpace("Main", Rect(0, 0, 1000, 800));
    DockNode* root = docking.getDockNode(rootId);
    ASSERT_NE(root, nullptr);

    DockNode::Id leftId =
        docking.splitNode(rootId, DockDirection::Left, 0.3f);
    DockNode* left = docking.getDockNode(leftId);
    ASSERT_NE(left, nullptr);
    ASSERT_NE(root->child(1), nullptr);
    const DockNode::Id rightId = root->child(1)->id();

    std::vector<WidgetId> windowIds;
    for (WidgetId id = 100; id < 116; ++id) {
        windowIds.push_back(id);
        docking.dockWindow(id, (id % 2 == 0) ? left->id() : rightId);
    }

    DockBuilder::ClearDockSpace(ctx, rootId);

    for (WidgetId id : windowIds) {
        EXPECT_FALSE(docking.isWindowDocked(id)) << "stale mapping for window " << id;
        EXPECT_EQ(docking.getWindowDockNode(id), nullptr);
    }
    EXPECT_TRUE(root->isLeafNode());
    EXPECT_TRUE(root->windows().empty());
    EXPECT_EQ(root->child(0), nullptr);
    EXPECT_EQ(root->child(1), nullptr);
}

TEST_F(DockingTest, ClearDockSpaceResetsNestedSplitTreeWithoutInvalidatingRoot) {
    Context ctx(false);
    DockContext& docking = ctx.docking();
    Rect originalBounds(10, 20, 1200, 900);
    DockNode::Id rootId = docking.createDockSpace("Nested", originalBounds);
    DockNode* root = docking.getDockNode(rootId);
    ASSERT_NE(root, nullptr);

    DockNode::Id leftId =
        docking.splitNode(rootId, DockDirection::Left, 0.25f);
    DockNode* left = docking.getDockNode(leftId);
    ASSERT_NE(left, nullptr);
    ASSERT_NE(root->child(1), nullptr);
    const DockNode::Id firstB = root->child(1)->id();

    DockNode::Id nestedA =
        docking.splitNode(leftId, DockDirection::Top, 0.5f);
    DockNode* topLeft = docking.getDockNode(nestedA);
    ASSERT_NE(topLeft, nullptr);
    ASSERT_NE(left->child(1), nullptr);
    const DockNode::Id nestedB = left->child(1)->id();

    docking.dockWindow(201, topLeft->id());
    docking.dockWindow(202, nestedB);
    docking.dockWindow(203, firstB);

    DockBuilder::ClearDockSpace(ctx, rootId);

    EXPECT_EQ(docking.getDockNode(rootId), root);
    EXPECT_EQ(root->id(), rootId);
    EXPECT_EQ(root->bounds(), originalBounds);
    EXPECT_TRUE(root->isLeafNode());
    EXPECT_TRUE(root->isEmpty());
    EXPECT_EQ(docking.getDockNode(leftId), nullptr);
    EXPECT_EQ(docking.getDockNode(firstB), nullptr);
    EXPECT_EQ(docking.getDockNode(nestedA), nullptr);
    EXPECT_EQ(docking.getDockNode(nestedB), nullptr);
    EXPECT_FALSE(docking.isWindowDocked(201));
    EXPECT_FALSE(docking.isWindowDocked(202));
    EXPECT_FALSE(docking.isWindowDocked(203));
}

TEST_F(DockingTest, DockableWindowStateDoesNotLeakAcrossContexts) {
    const std::string windowName = "SharedDockableWindow";

    {
        fst::testing::TestContext first;
        DockableWindowOptions options;
        options.title = "First context";
        options.showTitleBar = false;

        first.beginFrame();
        ASSERT_TRUE(BeginDockableWindow(first.context(), windowName, options));
        EndDockableWindow(first.context());
        EXPECT_EQ(first.context().docking().getWindowTitle(first.context().makeId(windowName.c_str())),
                  "First context");
        first.endFrame();
    }

    {
        fst::testing::TestContext second;
        DockableWindowOptions options;
        options.title = "Second context";
        options.showTitleBar = false;

        second.beginFrame();
        ASSERT_TRUE(BeginDockableWindow(second.context(), windowName, options));
        EndDockableWindow(second.context());
        EXPECT_EQ(second.context().docking().getWindowTitle(second.context().makeId(windowName.c_str())),
                  "Second context");
        second.endFrame();
    }
}

TEST_F(DockingTest, SplitterDragRemainsActiveWhenAnotherContextUsesDocking) {
    fst::testing::TestContext first;
    fst::testing::TestContext second;

    DockNode::Id firstRootId =
        first.context().docking().createDockSpace("FirstDockSpace", Rect(0, 0, 100, 100));
    DockNode* firstRoot = first.context().docking().getDockNode(firstRootId);
    ASSERT_NE(firstRoot, nullptr);
    ASSERT_NE(
        first.context().docking().splitNode(
            firstRootId, DockDirection::Left),
        DockNode::INVALID_ID);
    first.context().docking().dockWindow(1001, firstRoot->child(0)->id());
    first.context().docking().dockWindow(1002, firstRoot->child(1)->id());
    firstRoot->updateLayout(firstRoot->bounds());

    DockNode::Id secondRootId =
        second.context().docking().createDockSpace("SecondDockSpace", Rect(0, 0, 100, 100));
    DockNode* secondRoot = second.context().docking().getDockNode(secondRootId);
    ASSERT_NE(secondRoot, nullptr);
    ASSERT_NE(
        second.context().docking().splitNode(
            secondRootId, DockDirection::Left),
        DockNode::INVALID_ID);
    second.context().docking().dockWindow(2001, secondRoot->child(0)->id());
    second.context().docking().dockWindow(2002, secondRoot->child(1)->id());
    secondRoot->updateLayout(secondRoot->bounds());

    auto& firstInput = first.window().input();
    firstInput.beginFrame();
    firstInput.onMouseMove(50.0f, 50.0f);
    firstInput.onMouseDown(MouseButton::Left);
    first.beginFrame();
    EXPECT_TRUE(HandleDockSplitter(
        first.context(), firstRoot, Rect(48.0f, 0.0f, 4.0f, 100.0f), true));
    first.endFrame();

    auto& secondInput = second.window().input();
    secondInput.beginFrame();
    secondInput.onMouseMove(50.0f, 50.0f);
    secondInput.onMouseDown(MouseButton::Left);
    second.beginFrame();
    EXPECT_TRUE(HandleDockSplitter(
        second.context(), secondRoot, Rect(48.0f, 0.0f, 4.0f, 100.0f), true));
    second.endFrame();

    firstInput.beginFrame();
    firstInput.onMouseMove(70.0f, 50.0f);
    first.beginFrame();
    EXPECT_TRUE(HandleDockSplitter(
        first.context(), firstRoot, Rect(48.0f, 0.0f, 4.0f, 100.0f), true));
    EXPECT_FLOAT_EQ(firstRoot->splitRatio(), 0.7f);
    first.endFrame();
}

TEST_F(DockingTest, DockTabDragRemainsBoundToItsContext) {
    fst::testing::TestContext first;
    fst::testing::TestContext second;

    DockNode::Id firstRootId =
        first.context().docking().createDockSpace("FirstTabs", Rect(0, 0, 200, 100));
    DockNode* firstRoot = first.context().docking().getDockNode(firstRootId);
    ASSERT_NE(firstRoot, nullptr);
    constexpr WidgetId firstWindow = 101;
    first.context().docking().dockWindow(firstWindow, firstRootId);

    DockNode::Id secondRootId =
        second.context().docking().createDockSpace("SecondTabs", Rect(0, 0, 200, 100));
    DockNode* secondRoot = second.context().docking().getDockNode(secondRootId);
    ASSERT_NE(secondRoot, nullptr);
    constexpr WidgetId secondWindow = 202;
    second.context().docking().dockWindow(secondWindow, secondRootId);

    auto& firstInput = first.window().input();
    firstInput.beginFrame();
    firstInput.onMouseMove(20.0f, 10.0f);
    firstInput.onMouseDown(MouseButton::Left);
    first.beginFrame();
    RenderDockTabBar(first.context(), firstRoot);
    first.endFrame();

    auto& secondInput = second.window().input();
    secondInput.beginFrame();
    secondInput.onMouseMove(20.0f, 10.0f);
    secondInput.onMouseDown(MouseButton::Left);
    second.beginFrame();
    RenderDockTabBar(second.context(), secondRoot);
    second.endFrame();

    firstInput.beginFrame();
    firstInput.onMouseMove(40.0f, 10.0f);
    first.beginFrame();
    RenderDockTabBar(first.context(), firstRoot);
    EXPECT_TRUE(first.context().docking().dragState().active);
    EXPECT_EQ(first.context().docking().dragState().windowId, firstWindow);
    first.endFrame();
}

TEST_F(DockingTest, FinishingOneBuilderDoesNotEndAnotherContextsSession) {
    Context first(false);
    Context second(false);
    DockNode::Id firstRoot =
        DockBuilder::GetDockSpaceId(first, "FirstBuilderDockSpace");
    DockNode::Id secondRoot =
        DockBuilder::GetDockSpaceId(second, "SecondBuilderDockSpace");

    DockBuilder::Begin(first, firstRoot);
    DockBuilder::Begin(second, secondRoot);
    DockBuilder::Finish(second);

    DockNode::Id firstChild =
        DockBuilder::SplitNode(first, firstRoot, DockDirection::Left, 0.5f);

    EXPECT_NE(firstChild, DockNode::INVALID_ID);
    EXPECT_TRUE(DockBuilder::IsBuilding(first));
    EXPECT_FALSE(DockBuilder::IsBuilding(second));

    DockBuilder::Finish(first);
}
