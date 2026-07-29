#include <gtest/gtest.h>
#include <fastener/ui/dock_context.h>
#include <fastener/ui/dock_builder.h>
#include <fastener/ui/dock_node.h>
#include <fastener/core/context.h>
#include <fastener/widgets/dock_space.h>
#include <fastener/widgets/dockable_window.h>
#include "TestContext.h"

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
    
    // Split multiple times
    // This previously caused collisions because DockNode generated its own IDs
    // that could overlap with "Main"'s ID (which is usually small).
    
    DockNode::Id leftNode = DockDirection::None == DockDirection::None ? 0 : 0; // dummy
    
    // We simulate what DockBuilder does
    DockNode* root = docking.getDockNode(rootId);
    ASSERT_NE(root, nullptr);
    
    // First split
    DockNode::Id childId1 = docking.generateNodeId();
    DockNode::Id childId2 = docking.generateNodeId();
    DockNode* newNode = root->splitNode(DockDirection::Left, childId1, childId2, 0.5f);
    ASSERT_NE(newNode, nullptr);
    docking.refreshMappings(rootId);
    
    // Second split on a child
    DockNode::Id childId3 = docking.generateNodeId();
    DockNode::Id childId4 = docking.generateNodeId();
    DockNode* newNode2 = newNode->splitNode(DockDirection::Top, childId3, childId4, 0.5f);
    ASSERT_NE(newNode2, nullptr);
    
    // This call used to trigger infinite recursion if IDs collided
    docking.refreshMappings(rootId);
    
    // If we reached here without stack overflow, it's a good sign
    EXPECT_TRUE(true);
}

TEST_F(DockingTest, IdUniqueness) {
    DockContext docking;
    std::vector<DockNode::Id> ids;
    
    for (int i = 0; i < 100; ++i) {
        ids.push_back(docking.generateNodeId());
    }
    
    // Check for duplicates
    for (size_t i = 0; i < ids.size(); ++i) {
        for (size_t j = i + 1; j < ids.size(); ++j) {
            EXPECT_NE(ids[i], ids[j]) << "Duplicate ID found at " << i << " and " << j;
        }
    }
}

TEST_F(DockingTest, ClearDockSpaceRemovesEveryWindowMappingFromSplitLeaves) {
    Context ctx(false);
    DockContext& docking = ctx.docking();
    DockNode::Id rootId = docking.createDockSpace("Main", Rect(0, 0, 1000, 800));
    DockNode* root = docking.getDockNode(rootId);
    ASSERT_NE(root, nullptr);

    DockNode::Id leftId = docking.generateNodeId();
    DockNode::Id rightId = docking.generateNodeId();
    DockNode* left = root->splitNode(DockDirection::Left, leftId, rightId, 0.3f);
    ASSERT_NE(left, nullptr);
    ASSERT_NE(root->children[1], nullptr);

    std::vector<WidgetId> windowIds;
    for (WidgetId id = 100; id < 116; ++id) {
        windowIds.push_back(id);
        docking.dockWindow(id, (id % 2 == 0) ? left->id : root->children[1]->id);
    }

    DockBuilder::ClearDockSpace(ctx, rootId);

    for (WidgetId id : windowIds) {
        EXPECT_FALSE(docking.isWindowDocked(id)) << "stale mapping for window " << id;
        EXPECT_EQ(docking.getWindowDockNode(id), nullptr);
    }
    EXPECT_TRUE(root->isLeafNode());
    EXPECT_TRUE(root->dockedWindows.empty());
    EXPECT_EQ(root->children[0], nullptr);
    EXPECT_EQ(root->children[1], nullptr);
}

TEST_F(DockingTest, ClearDockSpaceResetsNestedSplitTreeWithoutInvalidatingRoot) {
    Context ctx(false);
    DockContext& docking = ctx.docking();
    Rect originalBounds(10, 20, 1200, 900);
    DockNode::Id rootId = docking.createDockSpace("Nested", originalBounds);
    DockNode* root = docking.getDockNode(rootId);
    ASSERT_NE(root, nullptr);

    DockNode::Id firstA = docking.generateNodeId();
    DockNode::Id firstB = docking.generateNodeId();
    DockNode* left = root->splitNode(DockDirection::Left, firstA, firstB, 0.25f);
    ASSERT_NE(left, nullptr);

    DockNode::Id nestedA = docking.generateNodeId();
    DockNode::Id nestedB = docking.generateNodeId();
    DockNode* topLeft = left->splitNode(DockDirection::Top, nestedA, nestedB, 0.5f);
    ASSERT_NE(topLeft, nullptr);

    docking.dockWindow(201, topLeft->id);
    docking.dockWindow(202, left->children[1]->id);
    docking.dockWindow(203, root->children[1]->id);

    DockBuilder::ClearDockSpace(ctx, rootId);

    EXPECT_EQ(docking.getDockNode(rootId), root);
    EXPECT_EQ(root->id, rootId);
    EXPECT_EQ(root->bounds, originalBounds);
    EXPECT_TRUE(root->isLeafNode());
    EXPECT_TRUE(root->isEmpty());
    EXPECT_EQ(docking.getDockNode(firstA), nullptr);
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
    ASSERT_NE(firstRoot->splitNode(
                  DockDirection::Left,
                  first.context().docking().generateNodeId(),
                  first.context().docking().generateNodeId()),
              nullptr);
    firstRoot->updateLayout(firstRoot->bounds);

    (void)second.context().docking().generateNodeId();
    DockNode::Id secondRootId =
        second.context().docking().createDockSpace("SecondDockSpace", Rect(0, 0, 100, 100));
    DockNode* secondRoot = second.context().docking().getDockNode(secondRootId);
    ASSERT_NE(secondRoot, nullptr);
    ASSERT_NE(secondRoot->splitNode(
                  DockDirection::Left,
                  second.context().docking().generateNodeId(),
                  second.context().docking().generateNodeId()),
              nullptr);
    secondRoot->updateLayout(secondRoot->bounds);

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
    EXPECT_FLOAT_EQ(firstRoot->splitRatio, 0.7f);
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

    (void)second.context().docking().generateNodeId();
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
