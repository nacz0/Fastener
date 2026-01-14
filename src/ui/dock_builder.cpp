#include "fastener/ui/dock_builder.h"
#include "fastener/ui/dock_context.h"
#include "fastener/core/context.h"

namespace fst {

//=============================================================================
// Static state for builder
//=============================================================================

static struct {
    bool building = false;
    DockNode::Id currentDockSpaceId = DockNode::INVALID_ID;
} s_builderState;

//=============================================================================
// DockBuilder Implementation
//=============================================================================

DockNode::Id DockBuilder::GetDockSpaceId(Context& ctx, const std::string& name) {
    auto& docking = ctx.docking();
    auto id = docking.getNodeIdFromString(name);
    
    if (id == DockNode::INVALID_ID) {
        // Create a placeholder - will be properly initialized when DockSpace is rendered
        id = docking.createDockSpace(name, Rect());
    }
    
    return id;
}

void DockBuilder::Begin(DockNode::Id dockspaceId) {
    s_builderState.building = true;
    s_builderState.currentDockSpaceId = dockspaceId;
}

void DockBuilder::Finish() {
    s_builderState.building = false;
    s_builderState.currentDockSpaceId = DockNode::INVALID_ID;
}

bool DockBuilder::IsBuilding() {
    return s_builderState.building;
}

DockNode::Id DockBuilder::SplitNode(Context& ctx,
                                     DockNode::Id nodeId, 
                                     DockDirection direction, 
                                     float sizeRatio) {
    if (!s_builderState.building) {
        return DockNode::INVALID_ID;
    }
    
    auto& docking = ctx.docking();
    DockNode* node = docking.getDockNode(nodeId);
    
    if (!node) {
        return DockNode::INVALID_ID;
    }
    
    DockNode::Id childId0 = docking.generateNodeId();
    DockNode::Id childId1 = docking.generateNodeId();
    
    DockNode* newNode = node->splitNode(direction, childId0, childId1, sizeRatio);
    if (newNode) {
        docking.refreshMappings(nodeId);
        return newNode->id;
    }
    return DockNode::INVALID_ID;
}

void DockBuilder::DockWindow(Context& ctx, const std::string& windowId, DockNode::Id nodeId) {
    if (!s_builderState.building) {
        return;
    }
    
    auto& docking = ctx.docking();
    WidgetId widgetId = ctx.makeId(windowId.c_str());
    
    docking.dockWindow(widgetId, nodeId, DockDirection::Center);
}

void DockBuilder::SetNodeFlags(Context& ctx, DockNode::Id nodeId, DockNodeFlags flags) {
    auto& docking = ctx.docking();
    DockNode* node = docking.getDockNode(nodeId);
    
    if (node) {
        node->flags = flags;
    }
}

DockNode::Id DockBuilder::GetNode(Context& ctx, DockNode::Id parentId, DockDirection direction) {
    auto& docking = ctx.docking();
    DockNode* parent = docking.getDockNode(parentId);
    
    if (!parent || !parent->isSplitNode()) {
        return DockNode::INVALID_ID;
    }
    
    // Determine which child based on direction and split type
    int childIdx = 0;
    if (parent->type == DockNodeType::SplitHorizontal) {
        childIdx = (direction == DockDirection::Right) ? 1 : 0;
    } else {
        childIdx = (direction == DockDirection::Bottom) ? 1 : 0;
    }
    
    return parent->children[childIdx] ? parent->children[childIdx]->id : DockNode::INVALID_ID;
}

void DockBuilder::ClearDockSpace(Context& ctx, DockNode::Id dockspaceId) {
    auto& docking = ctx.docking();
    DockNode* root = docking.getDockNode(dockspaceId);
    
    if (root) {
        // Clear all windows and children
        root->forEachLeaf([&docking](DockNode* leaf) {
            for (auto winId : leaf->dockedWindows) {
                docking.undockWindow(winId);
            }
        });
        
        root->children[0].reset();
        root->children[1].reset();
        root->type = DockNodeType::Leaf;
        root->dockedWindows.clear();
    }
}

} // namespace fst
