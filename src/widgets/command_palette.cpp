/**
 * @file command_palette.cpp
 * @brief Command palette widget implementation.
 */

#include "fastener/widgets/command_palette.h"
#include "fastener/core/context.h"
#include "fastener/graphics/draw_list.h"
#include "fastener/graphics/font.h"
#include "fastener/ui/theme.h"
#include "fastener/ui/widget_utils.h"
#include "fastener/widgets/text_input.h"
#include <algorithm>
#include <cctype>
#include <cmath>

namespace fst {

namespace {

std::string toLower(std::string_view text) {
    std::string out;
    out.reserve(text.size());
    for (char ch : text) {
        out.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(ch))));
    }
    return out;
}

bool matchesQuery(const CommandPaletteCommand& cmd, const std::string& queryLower) {
    if (queryLower.empty()) {
        return true;
    }

    std::string haystack;
    haystack.reserve(cmd.id.size() + cmd.label.size() + cmd.description.size() + cmd.shortcut.size() + 4);
    haystack.append(cmd.label);
    haystack.push_back(' ');
    haystack.append(cmd.description);
    haystack.push_back(' ');
    haystack.append(cmd.id);
    haystack.push_back(' ');
    haystack.append(cmd.shortcut);

    return toLower(haystack).find(queryLower) != std::string::npos;
}

bool isOpenShortcutPressed(const InputState& input) {
    const Modifiers mods = input.modifiers();
    return mods.ctrl && mods.shift && !mods.alt && !mods.super && input.isKeyPressed(Key::P);
}

} // namespace

//=============================================================================
// CommandPalette
//=============================================================================

CommandPalette::CommandPalette() = default;
CommandPalette::~CommandPalette() = default;

void CommandPalette::setCommands(const std::vector<CommandPaletteCommand>& commands) {
    m_commands = commands;
    m_selectedIndex = 0;
    m_scrollOffset = 0.0f;
}

void CommandPalette::addCommand(const CommandPaletteCommand& command) {
    m_commands.push_back(command);
}

void CommandPalette::clearCommands() {
    m_commands.clear();
    m_selectedIndex = 0;
    m_scrollOffset = 0.0f;
}

void CommandPalette::open(bool clearQuery) {
    m_open = true;
    m_shouldFocusInput = true;
    if (clearQuery) {
        m_query.clear();
        m_lastQuery.clear();
    }
    m_selectedIndex = 0;
    m_scrollOffset = 0.0f;
}

void CommandPalette::close() {
    m_open = false;
    m_shouldFocusInput = false;
}

void CommandPalette::setQuery(const std::string& query) {
    m_query = query;
    m_lastQuery.clear();
    m_selectedIndex = 0;
    m_scrollOffset = 0.0f;
}

const CommandPaletteCommand* CommandPalette::render(Context& ctx, const CommandPaletteOptions& options) {
    InputState& input = ctx.input();
    if (!m_open && options.openOnShortcut && isOpenShortcutPressed(input)) {
        open(options.clearQueryOnOpen);
    }
    if (!m_open) {
        return nullptr;
    }

    auto wc = WidgetContext::make(ctx);
    const Theme& theme = *wc.theme;
    IDrawList& dl = *wc.dl;
    Font* font = wc.font;

    float windowW = static_cast<float>(ctx.window().width());
    float windowH = static_cast<float>(ctx.window().height());

    float padding = options.padding > 0.0f ? options.padding : theme.metrics.paddingMedium;
    float inputHeight = options.inputHeight > 0.0f ? options.inputHeight : theme.metrics.inputHeight;
    float itemHeight = options.itemHeight > 0.0f
        ? options.itemHeight
        : (font ? font->lineHeight() + theme.metrics.paddingSmall * 2 : 24.0f);

    int maxVisibleItems = options.maxVisibleItems > 0 ? options.maxVisibleItems : 8;
    float listHeight = itemHeight * static_cast<float>(maxVisibleItems);
    float totalHeight = padding * 2 + inputHeight + listHeight;
    float maxHeight = options.maxHeight > 0.0f ? options.maxHeight : totalHeight;
    if (totalHeight > maxHeight) {
        listHeight = std::max(itemHeight, maxHeight - padding * 2 - inputHeight);
        totalHeight = padding * 2 + inputHeight + listHeight;
    }

    float width = options.style.width > 0.0f ? options.style.width : options.width;
    if (width <= 0.0f) {
        width = 520.0f;
    }
    float x = options.style.x >= 0.0f ? options.style.x : (windowW - width) * 0.5f;
    float y = options.style.y >= 0.0f ? options.style.y : std::max(20.0f, options.yOffset);
    Rect paletteRect(x, y, width, totalHeight);

    DrawLayer prevLayer = dl.currentLayer();
    dl.setLayer(DrawLayer::Overlay);

    Rect backdrop(0, 0, windowW, windowH);
    dl.pushClipRect(backdrop);
    dl.addRectFilled(backdrop, Color(0, 0, 0, 110));
    ctx.addFloatingWindowRect(backdrop);
    ctx.addFloatingWindowRect(paletteRect);

    if (options.closeOnEscape && input.isKeyPressed(Key::Escape)) {
        close();
        dl.popClipRect();
        dl.setLayer(prevLayer);
        return nullptr;
    }

    if (options.closeOnBackdrop && input.isMousePressed(MouseButton::Left) &&
        !paletteRect.contains(input.mousePos())) {
        close();
        dl.popClipRect();
        dl.setLayer(prevLayer);
        return nullptr;
    }

    dl.addShadow(paletteRect, theme.colors.shadow, 10.0f, theme.metrics.borderRadiusLarge);
    dl.addRectFilled(paletteRect, theme.colors.popupBackground, theme.metrics.borderRadiusLarge);
    dl.addRect(paletteRect, theme.colors.border, theme.metrics.borderRadiusLarge);

    ctx.pushId("command_palette");

    Rect inputRect(
        paletteRect.x() + padding,
        paletteRect.y() + padding,
        paletteRect.width() - padding * 2,
        inputHeight
    );

    if (m_shouldFocusInput) {
        WidgetId inputId = ctx.makeId("query");
        ctx.setFocusedWidget(inputId);
        m_shouldFocusInput = false;
    }

    TextInputOptions inputOpts;
    inputOpts.placeholder = options.placeholder;
    inputOpts.style = Style().withPos(inputRect.x(), inputRect.y())
                             .withSize(inputRect.width(), inputRect.height());
    (void)TextInput(ctx, "query", m_query, inputOpts);

    std::vector<int> filtered;
    filtered.reserve(m_commands.size());
    std::string queryLower = toLower(m_query);
    for (size_t i = 0; i < m_commands.size(); ++i) {
        if (matchesQuery(m_commands[i], queryLower)) {
            filtered.push_back(static_cast<int>(i));
        }
    }

    if (m_query != m_lastQuery) {
        m_selectedIndex = 0;
        m_scrollOffset = 0.0f;
        m_lastQuery = m_query;
    }

    if (filtered.empty()) {
        m_selectedIndex = -1;
    } else if (m_selectedIndex < 0) {
        m_selectedIndex = 0;
    } else {
        m_selectedIndex = std::min(m_selectedIndex, static_cast<int>(filtered.size()) - 1);
    }

    if (m_selectedIndex >= 0 && !filtered.empty()) {
        if (input.isKeyPressed(Key::Down)) {
            m_selectedIndex = std::min(m_selectedIndex + 1, static_cast<int>(filtered.size()) - 1);
        }
        if (input.isKeyPressed(Key::Up)) {
            m_selectedIndex = std::max(m_selectedIndex - 1, 0);
        }
    }

    const CommandPaletteCommand* activated = nullptr;
    if (m_selectedIndex >= 0 && !filtered.empty() &&
        (input.isKeyPressed(Key::Enter) || input.isKeyPressed(Key::KPEnter))) {
        CommandPaletteCommand& cmd = m_commands[filtered[m_selectedIndex]];
        if (cmd.enabled) {
            if (cmd.action) {
                cmd.action();
            }
            activated = &cmd;
            if (options.closeOnSelect) {
                close();
            }
        }
    }

    Rect listRect(
        paletteRect.x() + padding,
        inputRect.bottom() + padding,
        paletteRect.width() - padding * 2,
        listHeight
    );

    dl.addLine(
        Vec2(listRect.x(), inputRect.bottom() + padding * 0.5f),
        Vec2(listRect.right(), inputRect.bottom() + padding * 0.5f),
        theme.colors.border
    );

    float maxScroll = std::max(0.0f, static_cast<float>(filtered.size()) * itemHeight - listHeight);
    if (listRect.contains(input.mousePos()) && input.scrollDelta().y != 0.0f) {
        m_scrollOffset -= input.scrollDelta().y * itemHeight;
    }
    m_scrollOffset = std::clamp(m_scrollOffset, 0.0f, maxScroll);

    if (m_selectedIndex >= 0) {
        float selTop = m_selectedIndex * itemHeight;
        if (selTop < m_scrollOffset) {
            m_scrollOffset = selTop;
        } else if (selTop + itemHeight > m_scrollOffset + listHeight) {
            m_scrollOffset = selTop + itemHeight - listHeight;
        }
        m_scrollOffset = std::clamp(m_scrollOffset, 0.0f, maxScroll);
    }

    dl.pushClipRect(listRect);

    int hoveredIndex = -1;
    for (int row = 0; row < static_cast<int>(filtered.size()); ++row) {
        float yPos = listRect.y() + row * itemHeight - m_scrollOffset;
        if (yPos + itemHeight < listRect.y() || yPos > listRect.bottom()) {
            continue;
        }

        Rect itemRect(listRect.x(), yPos, listRect.width(), itemHeight);
        bool hovered = itemRect.contains(input.mousePos()) && listRect.contains(input.mousePos());
        bool selected = (row == m_selectedIndex);
        CommandPaletteCommand& cmd = m_commands[filtered[row]];

        if (hovered) {
            hoveredIndex = row;
        }

        if (selected) {
            dl.addRectFilled(itemRect, theme.colors.selection, theme.metrics.borderRadiusSmall);
        } else if (hovered) {
            dl.addRectFilled(itemRect, theme.colors.selection.withAlpha(static_cast<uint8_t>(80)),
                             theme.metrics.borderRadiusSmall);
        }

        if (font) {
            Color textColor = cmd.enabled ? theme.colors.text : theme.colors.textDisabled;
            if (selected) {
                textColor = theme.colors.selectionText;
            }

            float textY = itemRect.center().y - font->lineHeight() * 0.5f;
            float textX = itemRect.x() + theme.metrics.paddingSmall;

            dl.addText(font, Vec2(textX, textY), cmd.label, textColor);

            if (options.showDescription && !cmd.description.empty()) {
                float labelWidth = font->measureText(cmd.label).x;
                dl.addText(font,
                           Vec2(textX + labelWidth + theme.metrics.paddingSmall, textY),
                           cmd.description,
                           selected ? theme.colors.selectionText : theme.colors.textSecondary);
            }

            if (options.showShortcut && !cmd.shortcut.empty()) {
                float shortcutWidth = font->measureText(cmd.shortcut).x;
                dl.addText(font,
                           Vec2(itemRect.right() - shortcutWidth - theme.metrics.paddingSmall, textY),
                           cmd.shortcut,
                           selected ? theme.colors.selectionText : theme.colors.textSecondary);
            }
        }

        if (hovered && input.isMousePressed(MouseButton::Left)) {
            if (cmd.enabled) {
                if (cmd.action) {
                    cmd.action();
                }
                activated = &cmd;
                if (options.closeOnSelect) {
                    close();
                }
            }
        }
    }

    if (hoveredIndex >= 0) {
        m_selectedIndex = hoveredIndex;
    }

    if (filtered.empty() && font) {
        Vec2 textSize = font->measureText(options.emptyText);
        Vec2 textPos(
            listRect.center().x - textSize.x * 0.5f,
            listRect.center().y - textSize.y * 0.5f
        );
        dl.addText(font, textPos, options.emptyText, theme.colors.textSecondary);
    }

    dl.popClipRect();

    if (maxScroll > 0.0f) {
        float scrollbarWidth = theme.metrics.scrollbarWidth;
        Rect track(
            listRect.right() - scrollbarWidth,
            listRect.y(),
            scrollbarWidth,
            listRect.height()
        );
        dl.addRectFilled(track, theme.colors.scrollbarTrack);

        float thumbHeight = std::max(20.0f, (listRect.height() / (listRect.height() + maxScroll)) * listRect.height());
        float t = maxScroll > 0.001f ? std::clamp(m_scrollOffset / maxScroll, 0.0f, 1.0f) : 0.0f;
        float thumbY = track.y() + t * (track.height() - thumbHeight);

        Rect thumb(track.x() + 2, thumbY, track.width() - 4, thumbHeight);
        dl.addRectFilled(thumb, theme.colors.scrollbarThumb, (track.width() - 4) * 0.5f);
    }

    ctx.popId();

    dl.popClipRect();
    dl.setLayer(prevLayer);

    return activated;
}

} // namespace fst
