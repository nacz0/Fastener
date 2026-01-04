#pragma once

#include "fastener/core/types.h"
#include "fastener/ui/style.h"
#include <string>

namespace fst {

//=============================================================================
// TextInput
//=============================================================================
struct TextInputOptions {
    Style style;
    std::string placeholder;
    bool password = false;     // Mask input
    bool readonly = false;
    bool multiline = false;
    int maxLength = 0;         // 0 = no limit
};

bool TextInput(const std::string& id, std::string& value, const TextInputOptions& options = {});
bool TextInput(const char* id, std::string& value, const TextInputOptions& options = {});

// With label
bool TextInputWithLabel(const std::string& label, std::string& value, 
                        const TextInputOptions& options = {});

} // namespace fst
