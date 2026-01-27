#pragma once

#include "fastener/core/input.h"
#include "fastener/core/types.h"
#include "fastener/ui/style.h"
#include <functional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

/**
 * @file command_palette.h
 * @brief Command palette overlay with search and keyboard navigation.
 *
 * @ai_hint CommandPalette is a VSCode-style command search overlay.
 *          It opens on Ctrl+Shift+P by default, filters commands by query,
 *          and executes actions on Enter or click.
 */

namespace fst {

class Context;

//=============================================================================
// Command Palette Command
//=============================================================================
struct CommandPaletteCommand {
    std::string id;
    std::string label;
    std::string description;
    std::string shortcut;
    bool enabled = true;
    std::function<void()> action;

    CommandPaletteCommand() = default;
    CommandPaletteCommand(std::string_view id, std::string_view label,
                          std::function<void()> action = nullptr)
        : id(id), label(label), action(std::move(action)) {}

    CommandPaletteCommand& withDescription(std::string_view desc) {
        description = desc;
        return *this;
    }

    CommandPaletteCommand& withShortcut(std::string_view sc) {
        shortcut = sc;
        return *this;
    }

    CommandPaletteCommand& disabled() {
        enabled = false;
        return *this;
    }
};

//=============================================================================
// Command Palette Options
//=============================================================================
struct CommandPaletteOptions {
    Style style;
    float width = 520.0f;
    float maxHeight = 360.0f;
    float yOffset = 80.0f;
    float padding = 0.0f;       // 0 = theme default
    float inputHeight = 0.0f;   // 0 = theme default
    float itemHeight = 0.0f;    // 0 = auto from font height
    int maxVisibleItems = 8;
    bool openOnShortcut = true;
    bool closeOnSelect = true;
    bool closeOnEscape = true;
    bool closeOnBackdrop = true;
    bool clearQueryOnOpen = true;
    bool showDescription = true;
    bool showShortcut = true;
    std::string placeholder = "Type a command...";
    std::string emptyText = "No matching commands";
};

//=============================================================================
// Command Palette
//=============================================================================
class CommandPalette {
public:
    CommandPalette();
    ~CommandPalette();

    void setCommands(const std::vector<CommandPaletteCommand>& commands);
    void addCommand(const CommandPaletteCommand& command);
    void clearCommands();
    const std::vector<CommandPaletteCommand>& commands() const { return m_commands; }

    void open(bool clearQuery = true);
    void close();
    bool isOpen() const { return m_open; }

    const std::string& query() const { return m_query; }
    void setQuery(const std::string& query);

    // Render palette overlay. Returns the command executed this frame, or nullptr.
    [[nodiscard]] const CommandPaletteCommand* render(Context& ctx, const CommandPaletteOptions& options = {});

private:
    std::vector<CommandPaletteCommand> m_commands;
    std::string m_query;
    std::string m_lastQuery;
    bool m_open = false;
    bool m_shouldFocusInput = false;
    int m_selectedIndex = 0;
    float m_scrollOffset = 0.0f;
};

} // namespace fst
