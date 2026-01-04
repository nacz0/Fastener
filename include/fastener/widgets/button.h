#pragma once

#include "fastener/core/types.h"
#include "fastener/ui/style.h"
#include <string>

namespace fst {

//=============================================================================
// Button
//=============================================================================
struct ButtonOptions {
    Style style;
    bool primary = false;      // Use primary color
    bool disabled = false;
    std::string icon;          // Optional icon (future)
};

bool Button(const std::string& label, const ButtonOptions& options = {});
bool Button(const char* label, const ButtonOptions& options = {});

// Convenience overloads
bool ButtonPrimary(const std::string& label);
bool ButtonPrimary(const char* label);

} // namespace fst
