#pragma once

#include "fastener/core/types.h"
#include "fastener/ui/style.h"
#include "fastener/ui/layout.h"
#include <string>
#include <functional>

namespace fst {

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
bool BeginPanel(const std::string& id, const PanelOptions& options = {});
void EndPanel();

} // namespace fst
