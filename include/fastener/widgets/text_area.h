#pragma once

#include "fastener/core/types.h"
#include "fastener/ui/style.h"
#include <string>

namespace fst {
class Context;


/**
 * @brief Options for customizing TextArea appearance and behavior.
 */
struct TextAreaOptions {
    Style style;
    std::string placeholder;
    bool readonly = false;
    bool wordWrap = true;       ///< Wrap text at boundaries
    float height = 100.0f;      ///< Total height of the text area
    int maxLength = 0;          ///< 0 = no limit
    bool showLineNumbers = false;
};

/**
 * @brief Renders a multi-line text input area.
 * 
 * This is a simplified alternative to TextEditor for basic
 * multi-line text input without advanced features like 
 * syntax highlighting or cursor positioning.
 * 
 * @param id Unique identifier string for the widget
 * @param value Reference to the text string (modified on input)
 * @param options TextArea styling and behavior options
 * @return true if the text value was changed this frame
 */
bool TextArea(Context& ctx, const std::string& id, std::string& value, 
              const TextAreaOptions& options = {});

bool TextArea(Context& ctx, const char* id, std::string& value, 
              const TextAreaOptions& options = {});


/**
 * @brief TextArea with integrated label above.
 * 
 * @param label Label text displayed above the text area
 * @param value Reference to the text string
 * @param options TextArea styling and behavior options
 * @return true if the text value was changed this frame
 */
bool TextAreaWithLabel(Context& ctx, const std::string& label, std::string& value,
                       const TextAreaOptions& options = {});


} // namespace fst
