#include <gtest/gtest.h>
#include <fastener/ui/dock_context.h>
#include <fastener/ui/dock_builder.h>
#include <fastener/ui/dock_node.h>
#include <fastener/core/context.h>

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
