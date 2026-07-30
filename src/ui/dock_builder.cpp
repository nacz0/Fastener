#include "fastener/ui/dock_builder.h"
#include "fastener/ui/dock_context.h"
#include "fastener/core/context.h"
#include "../core/widget_state_registry.h"
#include <vector>

namespace fst {

//=============================================================================
// Context-owned state for builder
//=============================================================================

namespace {

struct DockBuilderContextState {
    bool building = false;
    DockNode::Id currentDockSpaceId = DockNode::INVALID_ID;
};

DockBuilderContextState& getBuilderState(Context& ctx) {
    return detail::widgetStates(ctx).get<DockBuilderContextState>();
}

bool isNodeInCurrentDockSpace(Context& ctx,
                              const DockBuilderContextState& state,
                              DockNode::Id nodeId) {
    if (!state.building ||
        state.currentDockSpaceId == DockNode::INVALID_ID ||
        nodeId == DockNode::INVALID_ID) {
        return false;
    }

    const DockNode* root = ctx.docking().getDockNode(state.currentDockSpaceId);
    return root && root->findNodeById(nodeId);
}

} // namespace

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

void DockBuilder::Begin(Context& ctx, DockNode::Id dockspaceId) {
    auto& state = getBuilderState(ctx);
    state.building = ctx.docking().getDockNode(dockspaceId) != nullptr;
    state.currentDockSpaceId =
        state.building ? dockspaceId : DockNode::INVALID_ID;
}

void DockBuilder::Finish(Context& ctx) {
    auto& state = getBuilderState(ctx);
    state.building = false;
    state.currentDockSpaceId = DockNode::INVALID_ID;
}

bool DockBuilder::IsBuilding(Context& ctx) {
    return getBuilderState(ctx).building;
}

DockNode::Id DockBuilder::SplitNode(Context& ctx,
                                     DockNode::Id nodeId, 
                                     DockDirection direction, 
                                     float sizeRatio) {
    const auto& state = getBuilderState(ctx);
    if (!isNodeInCurrentDockSpace(ctx, state, nodeId)) {
        return DockNode::INVALID_ID;
    }
    
    auto& docking = ctx.docking();
    DockNode* node = docking.getDockNode(nodeId);
    
    if (!node) {
        return DockNode::INVALID_ID;
    }
    
    return docking.splitNode(nodeId, direction, sizeRatio);
}

void DockBuilder::DockWindow(Context& ctx, const std::string& windowId, DockNode::Id nodeId) {
    const auto& state = getBuilderState(ctx);
    if (!isNodeInCurrentDockSpace(ctx, state, nodeId)) {
        return;
    }
    
    auto& docking = ctx.docking();
    WidgetId widgetId = ctx.makeId(windowId.c_str());
    
    docking.dockWindow(widgetId, nodeId, DockDirection::Center);
}

void DockBuilder::SetNodeFlags(Context& ctx, DockNode::Id nodeId, DockNodeFlags flags) {
    ctx.docking().setNodeFlags(nodeId, flags);
}

DockNode::Id DockBuilder::GetNode(Context& ctx, DockNode::Id parentId, DockDirection direction) {
    auto& docking = ctx.docking();
    DockNode* parent = docking.getDockNode(parentId);
    
    if (!parent || !parent->isSplitNode()) {
        return DockNode::INVALID_ID;
    }
    
    // Determine which child based on direction and split type
    int childIdx = 0;
    if (parent->m_type == DockNodeType::SplitHorizontal) {
        childIdx = (direction == DockDirection::Right) ? 1 : 0;
    } else {
        childIdx = (direction == DockDirection::Bottom) ? 1 : 0;
    }
    
    return parent->m_children[childIdx]
        ? parent->m_children[childIdx]->m_id
        : DockNode::INVALID_ID;
}

void DockBuilder::ClearDockSpace(Context& ctx, DockNode::Id dockspaceId) {
    auto& docking = ctx.docking();
    DockNode* root = docking.getDockNode(dockspaceId);
    
    if (root) {
        // Snapshot window IDs before mutating the tree. Undocking can erase from
        // dockedWindows and merge parent nodes, invalidating traversal state.
        std::vector<WidgetId> windowsToUndock;
        root->forEachLeaf([&windowsToUndock](DockNode* leaf) {
            windowsToUndock.insert(
                windowsToUndock.end(),
                leaf->m_dockedWindows.begin(),
                leaf->m_dockedWindows.end()
            );
        });

        for (WidgetId windowId : windowsToUndock) {
            docking.undockWindow(windowId);
        }
        
        root->m_children[0].reset();
        root->m_children[1].reset();
        root->m_type = DockNodeType::Leaf;
        root->m_dockedWindows.clear();
    }
}

} // namespace fst
