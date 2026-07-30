#include "fastener/widgets/dock_space.h"
#include "fastener/core/context.h"
#include "fastener/ui/dock_builder.h"
#include "fastener/ui/dock_context.h"
#include "fastener/graphics/draw_list.h"
#include "fastener/graphics/font.h"
#include "fastener/ui/theme.h"
#include "fastener/ui/widget.h"
#include "fastener/ui/widget_utils.h"
#include "fastener/platform/window.h"
#include "../core/widget_state_registry.h"


namespace fst {

namespace {

struct DockSpaceInteractionState {
    WidgetId activeSplitter = INVALID_WIDGET_ID;
    WidgetId activeDockTab = INVALID_WIDGET_ID;
    Vec2 dragStartPos;
    int dragTabIndex = -1;
    DockNode::Id dragNodeId = DockNode::INVALID_ID;
};

DockSpaceInteractionState& getDockSpaceInteractionState(Context& ctx) {
    return detail::widgetStates(ctx).get<DockSpaceInteractionState>();
}

} // namespace

//=============================================================================
// DockSpace Widget
//=============================================================================

DockNode::Id DockSpace(Context& ctx, const std::string& id, const Rect& bounds, 
                       const DockSpaceOptions& options) {
    auto& docking = ctx.docking();
    auto& dl = ctx.drawList();
    const auto& theme = ctx.theme();

    
    // Create or get the dock space
    auto nodeId = docking.createDockSpace(id, bounds);
    DockNode* root = docking.getDockSpace(id);
    
    if (!root) {
        return DockNode::INVALID_ID;
    }
    
    // Apply options
    DockNodeFlags nodeFlags = options.nodeFlags;
    nodeFlags.passthruCentralNode = options.passthruCentralNode;
    DockBuilder::SetNodeFlags(ctx, nodeId, nodeFlags);
    
    // Update layout
    root->updateLayout(bounds);
    
    // Render background if not passthru
    if (!options.passthruCentralNode) {
        dl.addRectFilled(bounds, theme.colors.panelBackground);
    }
    
    // Render splitters
    RenderDockSplitters(ctx, root);

    
    // Render tab bars for nodes with multiple windows
    root->forEachLeaf([&ctx](DockNode* leaf) {
        if (leaf->windows().size() > 1 || !leaf->flags().noTabBar) {
            RenderDockTabBar(ctx, leaf);
        }
    });
    
    return nodeId;
}

DockNode::Id DockSpaceOverViewport(Context& ctx, const DockSpaceOptions& options) {
    auto& window = ctx.window();
    Rect viewportBounds(0, 0, window.width(), window.height());
    
    return DockSpace(ctx, "##MainDockSpace", viewportBounds, options);
}


//=============================================================================
// Splitter Rendering and Interaction
//=============================================================================

void RenderDockSplitters(Context& ctx, DockNode* rootNode) {
    if (!rootNode) return;
    
    rootNode->forEachNode([&ctx](DockNode* node) {

        if (!node->isSplitNode()) return;
        
        const float splitterSize = 4.0f;
        Rect splitterRect;
        bool isVertical;
        
        if (node->type() == DockNodeType::SplitHorizontal) {
            // Vertical splitter (resizes horizontally)
            float splitX =
                node->bounds().x() +
                node->bounds().width() * node->splitRatio();
            splitterRect = Rect(
                splitX - splitterSize * 0.5f,
                node->bounds().y(),
                splitterSize,
                node->bounds().height()
            );
            isVertical = true;
        } else {
            // Horizontal splitter (resizes vertically)
            float splitY =
                node->bounds().y() +
                node->bounds().height() * node->splitRatio();
            splitterRect = Rect(
                node->bounds().x(),
                splitY - splitterSize * 0.5f,
                node->bounds().width(),
                splitterSize
            );
            isVertical = false;
        }
        
        HandleDockSplitter(ctx, node, splitterRect, isVertical);
    });
}


bool HandleDockSplitter(Context& ctx, DockNode* node, const Rect& splitterRect, bool isVertical) {
    auto& input = ctx.input();
    auto& dl = ctx.drawList();
    const auto& theme = ctx.theme();
    auto& window = ctx.window();

    
    // Create unique ID for this splitter
    WidgetId splitterId =
        combineIds(hashString("##DockSplitter"), node->id());
    
    auto& interactionState = getDockSpaceInteractionState(ctx);
    
    bool isHovered = splitterRect.contains(input.mousePos()) && !ctx.isOccluded(input.mousePos()) && !input.isMouseConsumed();
    bool isDragging = false;
    
    // Start drag on mouse press
    if (isHovered && input.isMousePressed(MouseButton::Left)) {
        interactionState.activeSplitter = splitterId;
        ctx.setActiveWidget(splitterId);
    }
    
    // Handle dragging
    if (interactionState.activeSplitter == splitterId && ctx.getActiveWidget() == splitterId) {
        if (input.isMouseReleased(MouseButton::Left)) {
            interactionState.activeSplitter = INVALID_WIDGET_ID;
            ctx.clearActiveWidget();
        } else if (input.isMouseDown(MouseButton::Left)) {

            isDragging = true;
            
            // Update split ratio based on mouse position
            if (isVertical) {
                float newRatio =
                    (input.mousePos().x - node->bounds().x()) /
                    node->bounds().width();
                node->setSplitRatio(std::clamp(newRatio, 0.1f, 0.9f));
            } else {
                float newRatio =
                    (input.mousePos().y - node->bounds().y()) /
                    node->bounds().height();
                node->setSplitRatio(std::clamp(newRatio, 0.1f, 0.9f));
            }
            
            // Re-layout after resize
            node->updateLayout(node->bounds());
        }
    }
    
    // Set cursor
    if (isHovered || isDragging) {
        window.setCursor(isVertical ? Cursor::ResizeH : Cursor::ResizeV);
    }
    
    // Draw splitter
    Color splitterColor = isDragging ? theme.colors.primary : 
                          (isHovered ? theme.colors.borderHover : theme.colors.border);
    dl.addRectFilled(splitterRect, splitterColor);
    
    return isDragging;
}


//=============================================================================
// Tab Bar Rendering
//=============================================================================

void RenderDockTabBar(Context& ctx, DockNode* node) {
    if (!node || node->windows().empty()) return;
    
    auto& dl = ctx.drawList();
    auto& input = ctx.input();
    const auto& theme = ctx.theme();
    auto& interactionState = getDockSpaceInteractionState(ctx);

    
    // Tab bar dimensions
    const float tabHeight = 24.0f;
    const float tabPadding = 8.0f;
    const float minTabWidth = 60.0f;
    const float maxTabWidth = 200.0f;
    
    Rect tabBarRect = Rect(
        node->bounds().x(),
        node->bounds().y(),
        node->bounds().width(),
        tabHeight
    );
    
    // Draw tab bar background
    dl.addRectFilled(tabBarRect, theme.colors.panelBackground.darker(0.1f));
    
    // Calculate tab width
    float availableWidth = tabBarRect.width() - tabPadding * 2;
    int tabCount = static_cast<int>(node->windows().size());
    float tabWidth = std::clamp(availableWidth / tabCount, minTabWidth, maxTabWidth);
    
    // Draw tabs
    float tabX = tabBarRect.x() + tabPadding;
    
    for (int i = 0; i < tabCount; ++i) {
        Rect tabRect(tabX, tabBarRect.y(), tabWidth - 2.0f, tabHeight);
        bool isSelected = (i == node->selectedTabIndex());
        bool isHovered = tabRect.contains(input.mousePos()) && !ctx.isOccluded(input.mousePos()) && !input.isMouseConsumed();
        
        WidgetId tabId =
            combineIds(hashString("##DockTab"), node->id() ^ i);
        
        // Start tracking on mouse press
        if (isHovered && input.isMousePressed(MouseButton::Left)) {
            node->selectTab(i);
            interactionState.activeDockTab = tabId;
            interactionState.dragStartPos = input.mousePos();
            interactionState.dragTabIndex = i;
            interactionState.dragNodeId = node->id();
            ctx.setActiveWidget(tabId);
        }
        
        // Handle dragging for tab undocking - only if this is the active tab
        if (interactionState.activeDockTab == tabId) {
            if (input.isMouseReleased(MouseButton::Left)) {
                interactionState.activeDockTab = INVALID_WIDGET_ID;
                interactionState.dragNodeId = DockNode::INVALID_ID;
                interactionState.dragTabIndex = -1;
                ctx.clearActiveWidget();
            } else if (input.isMouseDown(MouseButton::Left) &&
                       interactionState.dragNodeId == node->id() &&
                       interactionState.dragTabIndex == i) {
                float dragDistSq =
                    (input.mousePos() - interactionState.dragStartPos).lengthSquared();
                if (dragDistSq > 25.0f) { // 5 pixel threshold
                    // Start dragging the window out of the dock
                    ctx.docking().beginDrag(
                        node->windows()[static_cast<std::size_t>(i)],
                        input.mousePos());
                    interactionState.activeDockTab = INVALID_WIDGET_ID;
                    interactionState.dragNodeId = DockNode::INVALID_ID;
                    interactionState.dragTabIndex = -1;
                    ctx.clearActiveWidget();
                }
            }
        }

        
        // Tab color
        Color tabColor = isSelected ? theme.colors.panelBackground : 
                        (isHovered ? theme.colors.buttonHover : theme.colors.panelBackground.darker(0.05f));
        
        // Ghost if being dragged
        if (ctx.docking().dragState().active &&
            ctx.docking().dragState().windowId ==
                node->windows()[static_cast<std::size_t>(i)]) {
            tabColor = tabColor.withAlpha(0.5f);
        }


        // Draw tab
        dl.addRectFilled(tabRect, tabColor, 4.0f);
        
        // Draw tab border on selected
        if (isSelected) {
            dl.addRect(tabRect, theme.colors.primary, 4.0f);
        }
        
        // Draw tab label
        std::string title = ctx.docking().getWindowTitle(
            node->windows()[static_cast<std::size_t>(i)]);
        if (ctx.font()) {
            Vec2 textSize = ctx.font()->measureText(title);
            Vec2 textPos = tabRect.center() - textSize * 0.5f;
            dl.addText(ctx.font(), textPos, title, isSelected ? theme.colors.text : theme.colors.textSecondary);
        }

        
        tabX += tabWidth;
    }
    
    // Draw separator line
    dl.addLine(
        Vec2(tabBarRect.x(), tabBarRect.bottom()),
        Vec2(tabBarRect.right(), tabBarRect.bottom()),
        theme.colors.border,
        1.0f
    );
}

} // namespace fst
