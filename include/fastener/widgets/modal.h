#pragma once

#include "fastener/core/types.h"
#include "fastener/ui/style.h"
#include "fastener/ui/layout.h"
#include <string>

/**
 * @file modal.h
 * @brief Centered popup dialog with backdrop overlay
 * 
 * @ai_hint Modal displays a centered dialog with a semi-transparent backdrop.
 *          Use BeginModal/EndModal pattern. The backdrop captures clicks outside.
 *          Pass a bool& to control open state. Use ModalButton for dialog actions.
 * 
 * @example
 *   static bool showAlert = false;
 *   if (fst::Button(ctx, "Show Alert")) showAlert = true;
 *   
 *   if (fst::BeginModal(ctx, "alert", showAlert, {.title = "Alert"})) {
 *       fst::Label(ctx, "Something happened!");
 *       if (fst::ModalButton(ctx, "OK")) {
 *           showAlert = false;  // Close modal
 *       }
 *   }
 *   fst::EndModal(ctx);
 */

namespace fst {

class Context;

//=============================================================================
// Modal
//=============================================================================
struct ModalOptions {
    Style style;
    std::string title;
    float width = 400.0f;
    float height = 0.0f;        // 0 = auto-size to content
    bool closeable = true;      // Show X button in title bar
    bool closeOnBackdrop = true; // Click backdrop to close
    LayoutDirection direction = LayoutDirection::Vertical;
    float padding = 0.0f;       // 0 = use theme default
};

// Modal scope guard for RAII usage
class ModalScope {
public:
    ModalScope(Context& ctx, const std::string& id, bool& isOpen, const ModalOptions& options = {});
    ~ModalScope();
    
    operator bool() const { return m_visible; }
    
    ModalScope(const ModalScope&) = delete;
    ModalScope& operator=(const ModalScope&) = delete;
    
private:
    Context* m_ctx;
    bool m_visible;
    bool m_needsEnd;
};

// Usage: Modal(ctx, "myModal", isOpen) { ... }
#define Modal(ctx, id, isOpen, ...) if (fst::ModalScope _modal_##__LINE__{ctx, id, isOpen, ##__VA_ARGS__})

/// Begin modal dialog - returns true if modal should render content
bool BeginModal(Context& ctx, const std::string& id, bool& isOpen, const ModalOptions& options = {});

/// End modal dialog
void EndModal(Context& ctx);

/// Button styled for modal dialog footer
[[nodiscard]] bool ModalButton(Context& ctx, std::string_view label, bool primary = false);

} // namespace fst
