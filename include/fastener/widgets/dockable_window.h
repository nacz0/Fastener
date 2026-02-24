#pragma once

#include "fastener/core/types.h"
#include "fastener/ui/style.h"
#include <string>

namespace fst {

class Context;


//=============================================================================
// DockableWindowOptions - Configuration for dockable windows
//=============================================================================
struct DockableWindowOptions {
    Style style;
    std::string title;
    bool* open = nullptr;           // Optional pointer to visibility flag
    bool allowDocking = true;       // Can be docked
    bool allowFloating = true;      // Can be undocked
    bool draggable = true;          // Can be dragged when floating
    bool showTitleBar = true;       // Render floating title bar
    bool noTabBar = false;          // Hide tab when only window in node
    WidgetId dockFamilyId = 0;      // Restrict to window family
};

//=============================================================================
// DockableWindow Functions
//=============================================================================

/**
 * Begins a dockable window.
 * 
 * If the window is docked, it renders inside its DockNode.
 * If floating, it renders as a standard window.
 * 
 * @param id Unique identifier for the window
 * @param options Configuration options
 * @return true if the window content should be rendered
 */
bool BeginDockableWindow(Context& ctx, const std::string& id, const DockableWindowOptions& options = {});

/**
 * Ends a dockable window.
 * Must be called after BeginDockableWindow returns true.
 */
void EndDockableWindow(Context& ctx);


//=============================================================================
// DockableWindowScope - RAII wrapper for if() usage pattern
//=============================================================================
class DockableWindowScope {
public:
    DockableWindowScope(Context& ctx, const std::string& id, const DockableWindowOptions& options = {});
    ~DockableWindowScope();

    
    // Conversion to bool for if() usage
    operator bool() const { return m_visible; }
    
    // Non-copyable
    DockableWindowScope(const DockableWindowScope&) = delete;
    DockableWindowScope& operator=(const DockableWindowScope&) = delete;
    
private:
    Context* m_ctx;
    bool m_visible;
};

//=============================================================================
// Macro for convenient usage
//=============================================================================
#define DockableWindow(ctx, id, ...) \
    if (fst::DockableWindowScope _dw_scope_##__LINE__{ctx, id, ##__VA_ARGS__})

} // namespace fst
