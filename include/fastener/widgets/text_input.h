#pragma once

#include "fastener/core/types.h"
#include "fastener/ui/style.h"
#include <string>

/**
 * @file text_input.h
 * @brief Single-line text entry widget
 * 
 * @ai_hint First parameter is an ID (for focus tracking), NOT a label.
 *          Use TextInputWithLabel if you want a visible label.
 *          Returns true when content changes (typing, paste, delete).
 *          Focus is gained on click, lost on click-away or Tab.
 * 
 * @example
 *   static std::string username;
 *   fst::TextInput("user_input", username, {.placeholder = "Username..."});
 *   
 *   // With visible label:
 *   fst::TextInputWithLabel("Email", email);
 */

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
