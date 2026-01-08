#include <gtest/gtest.h>
#include <fastener/ui/dock_context.h>
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
