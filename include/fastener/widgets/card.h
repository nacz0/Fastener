#pragma once

#include "fastener/core/types.h"
#include "fastener/ui/style.h"
#include "fastener/ui/layout.h"
#include <string>

/**
 * @file card.h
 * @brief Content container widget with shadow and optional title
 * 
 * @ai_hint Cards are container widgets similar to Panel but with visual emphasis.
 *          Uses Begin/End pattern with RAII scope guard. Children layout
 *          automatically with vertical direction by default.
 * 
 * @example
 *   // RAII style (recommended):
 *   Card(ctx, "user_profile", {.title = "Profile"}) {
 *       fst::Label(ctx, "John Doe");
 *       fst::Button(ctx, "Edit");
 *   }
 *   
 *   // Begin/End style:
 *   if (fst::BeginCard(ctx, "settings", {.shadow = 2.0f})) {
 *       fst::Checkbox(ctx, "Dark Mode", darkMode);
 *   }
 *   fst::EndCard(ctx);  // Always call
 */

namespace fst {

class Context;

//=============================================================================
// Card
//=============================================================================
struct CardOptions {
    Style style;
    std::string title;
    float shadow = 4.0f;            // Shadow blur radius (0 = no shadow)
    float padding = 0.0f;           // 0 = use theme default
    LayoutDirection direction = LayoutDirection::Vertical;
    float spacing = 0.0f;           // 0 = use theme default
};

// Card scope guard for RAII usage
class CardScope {
public:
    CardScope(Context& ctx, const std::string& id, const CardOptions& options = {});
    ~CardScope();
    
    operator bool() const { return m_visible; }
    
    CardScope(const CardScope&) = delete;
    CardScope& operator=(const CardScope&) = delete;
    
private:
    Context* m_ctx;
    bool m_visible;
    bool m_needsEnd;
};

// Usage: Card(ctx, "myCard") { ... }
#define Card(ctx, id, ...) if (fst::CardScope _card_##__LINE__{ctx, id, ##__VA_ARGS__})

// Begin/End style
bool BeginCard(Context& ctx, const std::string& id, const CardOptions& options = {});
void EndCard(Context& ctx);

} // namespace fst
