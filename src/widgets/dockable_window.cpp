#include "fastener/widgets/dockable_window.h"
#include "fastener/core/context.h"
#include "fastener/ui/dock_context.h"
#include "fastener/ui/dock_node.h"
#include "fastener/graphics/draw_list.h"
#include "fastener/graphics/font.h"
#include "fastener/ui/theme.h"
#include "fastener/ui/layout.h"
#include "fastener/platform/window.h"
#include <unordered_map>

namespace fst {

//=============================================================================
// Window state tracking
//=============================================================================

struct DockableWindowState {
    WidgetId id = INVALID_WIDGET_ID;
    std::string title;
    Rect floatingBounds;
    bool isDragging = false;
    Vec2 dragOffset;
};

// Simple window state storage
static std::unordered_map<WidgetId, DockableWindowState> s_windowStates;

static DockableWindowState& getWindowState(WidgetId id) {
    return s_windowStates[id];
}

//=============================================================================
// Active window stack
//=============================================================================

static std::vector<WidgetId> s_windowStack;

//=============================================================================
// BeginDockableWindow
//=============================================================================

bool BeginDockableWindow(const std::string& id, const DockableWindowOptions& options) {
    auto* ctx = Context::current();
    if (!ctx) {
        return false;
    }
    
    // Calculate final widget ID without pushing to stack yet
    WidgetId widgetId = ctx->makeId(id.c_str());
    auto& docking = ctx->docking();
    auto& dl = ctx->drawList();
    auto& input = ctx->input();
    auto& layout = ctx->layout();
    const auto& theme = ctx->theme();
    auto& window = ctx->window();
    
    // Get or create window state
    auto& state = getWindowState(widgetId);
    if (state.id == INVALID_WIDGET_ID) {
        state.id = widgetId;
        state.title = options.title.empty() ? id : options.title;
        state.floatingBounds = Rect(100, 100, 400, 300);  // Default floating size
    }
    
    // Register title even if invisible/docked so DockSpace knows labels for tabs
    docking.setWindowTitle(widgetId, state.title);

    // Check visibility
    if (options.open && !*options.open) {
        return false;
    }
    
    // Determine docking and drag status
    DockNode* dockNode = docking.getWindowDockNode(widgetId);
    bool docked = (dockNode != nullptr);
    auto& dragState = docking.dragState();
    bool beingDragged = (dragState.active && dragState.windowId == widgetId);

    // Visibility logic: show if floating, if being dragged, or if it's the selected tab
    bool shouldShow = true;
    if (docked && !beingDragged) {
        int windowIdx = -1;
        for (size_t i = 0; i < dockNode->dockedWindows.size(); ++i) {
            if (dockNode->dockedWindows[i] == widgetId) {
                windowIdx = static_cast<int>(i);
                break;
            }
        }
        shouldShow = (windowIdx == dockNode->selectedTabIndex);
    } else if (!docked && !beingDragged && !options.allowFloating) {
        shouldShow = false;
    }

    if (!shouldShow) {
        return false;
    }

    // --- FROM HERE WE ARE VISIBLE AND RENDERING ---
    
    // Push to window stack and ID stack
    s_windowStack.push_back(widgetId);
    ctx->pushId(id.c_str());
    
    Rect contentBounds;
    const float titleBarHeight = 28.0f;
    
    // Switch to appropriate draw layer
    if (beingDragged) {
        dl.setLayer(DrawList::Layer::Overlay);
    } else if (!docked) {
        dl.setLayer(DrawList::Layer::Floating);
    } else {
        dl.setLayer(DrawList::Layer::Default);
    }
    
    if (beingDragged) {
        // DRAGGING STATE (Follow mouse)
        contentBounds = state.floatingBounds;
        // Offset so mouse is near top-center of the window
        contentBounds.pos = input.mousePos() - Vec2(contentBounds.width() * 0.5f, 15.0f);
        
        // Draw ghost-like background
        dl.addRectFilled(contentBounds, theme.colors.panelBackground.withAlpha(0.7f), 8.0f);
        dl.addRect(contentBounds, theme.colors.primary, 8.0f);
        
    } else if (docked) {
        // DOCKED MODE
        contentBounds = dockNode->bounds;
        
        // Account for tab bar
        const float tabBarHeight = 24.0f;
        if (dockNode->dockedWindows.size() > 1 || !dockNode->flags.noTabBar) {
            contentBounds.pos.y += tabBarHeight;
            contentBounds.size.y -= tabBarHeight;
        }
        
        // Draw content background
        dl.addRectFilled(contentBounds, theme.colors.panelBackground);
        
    } else if (options.allowFloating) {
        // FLOATING MODE
        if (state.isDragging) {
            if (input.isMouseReleased(MouseButton::Left)) {
                state.isDragging = false;
                ctx->clearActiveWidget();
                
                if (options.allowDocking) {
                    docking.endDrag(true);
                }
            } else {
                state.floatingBounds.pos = input.mousePos() - state.dragOffset;
                contentBounds = state.floatingBounds;
                
                // Update drag preview for docking
                if (options.allowDocking) {
                    if (!docking.dragState().active) {
                        docking.beginDrag(widgetId, input.mousePos());
                    }
                }
            }
            
            window.setCursor(Cursor::Move);
        }
        
        contentBounds = state.floatingBounds;
        
        // Draw shadow
        Rect shadowRect = contentBounds.expanded(4.0f);
        dl.addRectFilled(shadowRect, theme.colors.shadow, 8.0f);
        
        // Draw window background
        dl.addRectFilled(contentBounds, theme.colors.panelBackground, 8.0f);
        
        // Draw title bar
        Rect titleBarRect(contentBounds.x(), contentBounds.y(), 
                         contentBounds.width(), titleBarHeight);
        dl.addRectFilled(titleBarRect, theme.colors.panelBackground.darker(0.1f), 8.0f);
        
        // Draw title
        if (ctx->font()) {
            Vec2 titlePos(titleBarRect.x() + 8.0f, 
                         titleBarRect.y() + (titleBarHeight - 14.0f) * 0.5f);
            dl.addText(ctx->font(), titlePos, state.title, theme.colors.text);
        }
        
        // Handle dragging via title bar
        if (!state.isDragging && !beingDragged) {
            bool titleHovered = titleBarRect.contains(input.mousePos()) && !ctx->isOccluded(input.mousePos());
            if (titleHovered && input.isMousePressed(MouseButton::Left)) {
                state.isDragging = true;
                state.dragOffset = input.mousePos() - contentBounds.pos;
                ctx->setActiveWidget(widgetId);
                ctx->input().consumeMouse();
            }
        }

        // Draw border
        dl.addRect(contentBounds, theme.colors.border, 8.0f);
        
        // Update content bounds for body
        contentBounds = Rect(contentBounds.x(), contentBounds.y() + titleBarHeight,
                            contentBounds.width(), contentBounds.height() - titleBarHeight);

        // Register occlusion
        ctx->addFloatingWindowRect(state.floatingBounds);
    }
    
    // Begin layout for window content
    dl.pushClipRect(contentBounds);
    layout.beginContainer(contentBounds);
    
    return true;
}

//=============================================================================
// EndDockableWindow
//=============================================================================

void EndDockableWindow() {
    auto* ctx = Context::current();
    if (!ctx || s_windowStack.empty()) {
        return;
    }
    
    auto& layout = ctx->layout();
    auto& dl = ctx->drawList();
    
    // End layout
    layout.endContainer();
    dl.popClipRect();
    
    // Reset layer
    dl.setLayer(DrawList::Layer::Default);
    
    // Pop ID
    ctx->popId();
    
    // Pop from stack
    s_windowStack.pop_back();
}

//=============================================================================
// DockableWindowScope
//=============================================================================

DockableWindowScope::DockableWindowScope(const std::string& id, 
                                         const DockableWindowOptions& options)
    : m_visible(BeginDockableWindow(id, options)) {
}

DockableWindowScope::~DockableWindowScope() {
    if (m_visible) {
        EndDockableWindow();
    }
}

} // namespace fst
