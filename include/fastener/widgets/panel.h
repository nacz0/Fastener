#pragma once

#include "fastener/core/types.h"
#include "fastener/ui/style.h"
#include "fastener/ui/layout.h"
#include <string>
#include <functional>

/**
 * @file panel.h
 * @brief Container widget with background, title, and layout management
 * 
 * @ai_hint Use the Panel() MACRO for RAII-style scoping (recommended).
 *          The macro expands to an if-statement, so use braces:
 *            Panel("id") { Button("OK"); }
 *          
 *          For more control, use BeginPanel()/EndPanel() pair.
 *          Panel automatically sets up a new Layout context for children.
 *          Returns false (skips content) if panel is collapsed.
 * 
 * @example
 *   // RAII style (recommended):
 *   Panel("settings", {.title = "Settings"}) {
 *       fst::Checkbox("Dark Mode", darkMode);
 *       fst::Slider("Font Size", fontSize, 8, 24);
 *   }
 *   
 *   // Begin/End style:
 *   if (fst::BeginPanel("options", {.collapsible = true})) {
 *       fst::Button("Save");
 *   }
 *   fst::EndPanel(); // Always call, even if BeginPanel returned false
 */

namespace fst {

class Context;

//=============================================================================
// Panel
//=============================================================================
struct PanelOptions {
    Style style;
    std::string title;
    bool collapsible = false;
    bool scrollable = false;
    LayoutDirection direction = LayoutDirection::Vertical;
    float spacing = 0.0f;  // 0 = use theme default
};

// Panel scope guard
class PanelScope {
public:
    /// RAII constructor with explicit context
    PanelScope(Context& ctx, const std::string& id, const PanelOptions& options = {});
    
    /// RAII constructor using context stack
    PanelScope(const std::string& id, const PanelOptions& options = {});
    ~PanelScope();
    
    // For if(Panel) usage
    operator bool() const { return m_visible; }
    
    // Disable copy
    PanelScope(const PanelScope&) = delete;
    PanelScope& operator=(const PanelScope&) = delete;
    
private:
    bool m_visible;
    bool m_needsEnd;
};

// Usage: Panel("myPanel") { ... }
#define Panel(id, ...) if (fst::PanelScope _panel_##__LINE__{id, ##__VA_ARGS__})

// Begin/End style (alternative)
bool BeginPanel(Context& ctx, const std::string& id, const PanelOptions& options = {});
bool BeginPanel(const std::string& id, const PanelOptions& options = {});
void EndPanel();

} // namespace fst
