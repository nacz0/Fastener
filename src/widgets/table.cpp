/**
 * @file table.cpp
 * @brief Table widget implementation for displaying tabular data.
 */

#include "fastener/widgets/table.h"
#include "fastener/core/context.h"
#include "fastener/graphics/draw_list.h"
#include "fastener/graphics/font.h"
#include "fastener/ui/widget.h"
#include "fastener/ui/widget_utils.h"
#include "fastener/ui/theme.h"
#include "fastener/ui/layout.h"
#include <algorithm>
#include <unordered_map>

namespace fst {

//=============================================================================
// Table State (for immediate-mode API)
//=============================================================================

struct TableState {
    std::vector<TableColumn> columns;
    TableOptions options;
    TableEvents events;
    Rect bounds;
    Rect contentBounds;
    int sortColumn = -1;
    bool sortAscending = true;
    int currentRow = 0;
    int clickedRow = -1;
    int hoveredRow = -1;
    float scrollOffset = 0.0f;
    float rowHeight = 24.0f;
    float headerHeight = 28.0f;
    bool inTable = false;
    int resizingColumn = -1;
    float resizeStartX = 0.0f;
    float resizeStartWidth = 0.0f;
};

static std::unordered_map<WidgetId, TableState> s_tableStates;
static TableState* s_currentTable = nullptr;
static WidgetId s_currentTableId = 0;

//=============================================================================
// Table Class Implementation
//=============================================================================

Table::Table() = default;
Table::~Table() = default;

void Table::setColumns(const std::vector<TableColumn>& columns) {
    m_columns = columns;
}

void Table::setSort(int column, bool ascending) {
    m_sortColumn = column;
    m_sortAscending = ascending;
}

float Table::getTotalWidth() const {
    float total = 0;
    for (const auto& col : m_columns) {
        total += col.width;
    }
    return total;
}

void Table::begin(Context& ctx, const std::string& id, const Rect& bounds,
                  const TableOptions& options,
                  const TableEvents& events) {
    m_currentId = id;
    m_bounds = bounds;
    m_options = options;
    m_events = events;
    m_currentRow = 0;
    m_clickedRow = -1;
    m_hoveredRow = -1;
    m_inTable = true;

    ctx.pushId(id.c_str());

    const Theme& theme = ctx.theme();
    Font* font = ctx.font();

    // Calculate heights
    m_rowHeight = options.rowHeight > 0 
        ? options.rowHeight 
        : (font ? font->lineHeight() + theme.metrics.paddingSmall * 2 : 24.0f);
    m_headerHeight = options.headerHeight > 0 
        ? options.headerHeight 
        : (font ? font->lineHeight() + theme.metrics.paddingMedium * 2 : 28.0f);

    // Content area (below header)
    float headerH = options.showHeader ? m_headerHeight : 0;
    m_contentBounds = Rect(bounds.x(), bounds.y() + headerH, 
                           bounds.width(), bounds.height() - headerH);

    // Handle scrolling
    handleScroll(ctx);
    
    // Handle column resizing
    handleColumnResize(ctx);

    // Render header
    if (options.showHeader) {
        renderHeader(ctx);
    }
}

void Table::begin(const std::string& id, const Rect& bounds,
                  const TableOptions& options,
                  const TableEvents& events) {
    auto wc = getWidgetContext();
    if (!wc.valid()) return;
    begin(*wc.ctx, id, bounds, options, events);
}


void Table::renderHeader(Context& ctx) {
    const Theme& theme = ctx.theme();
    IDrawList& dl = *ctx.activeDrawList();
    Font* font = ctx.font();
    InputState& input = ctx.input();

    Rect headerRect(m_bounds.x(), m_bounds.y(), m_bounds.width(), m_headerHeight);
    
    // Header background
    dl.addRectFilled(headerRect, theme.colors.panelBackground.darker(0.05f));
    
    // Header bottom border
    dl.addLine(
        Vec2(headerRect.x(), headerRect.bottom()),
        Vec2(headerRect.right(), headerRect.bottom()),
        theme.colors.border
    );

    float x = headerRect.x();
    for (size_t i = 0; i < m_columns.size(); ++i) {
        const TableColumn& col = m_columns[i];
        Rect cellRect(x, headerRect.y(), col.width, m_headerHeight);

        // Header cell hover
        bool hovered = cellRect.contains(input.mousePos());
        if (hovered && col.sortable) {
            dl.addRectFilled(cellRect, theme.colors.selection.withAlpha((uint8_t)30));
        }

        // Handle click for sorting
        bool canClick = hovered && col.sortable && !ctx.isInputCaptured() && !input.isMouseConsumed();
        if (canClick && input.isMousePressed(MouseButton::Left)) {

            if (m_sortColumn == (int)i) {
                m_sortAscending = !m_sortAscending;
            } else {
                m_sortColumn = (int)i;
                m_sortAscending = true;
            }
            if (m_events.onSort) {
                m_events.onSort(m_sortColumn, m_sortAscending);
            }
        }

        // Draw header text
        if (font) {
            Vec2 textPos;
            Vec2 textSize = font->measureText(col.header);
            float padding = theme.metrics.paddingSmall;

            switch (col.alignment) {
                case Alignment::Center:
                    textPos.x = cellRect.x() + (cellRect.width() - textSize.x) * 0.5f;
                    break;
                case Alignment::End:
                    textPos.x = cellRect.right() - textSize.x - padding;
                    break;
                default:
                    textPos.x = cellRect.x() + padding;
            }
            textPos.y = cellRect.y() + (m_headerHeight - font->lineHeight()) * 0.5f;

            dl.addText(font, textPos, col.header.c_str(), theme.colors.text);

            // Sort indicator
            if (m_sortColumn == (int)i) {
                float arrowX = cellRect.right() - 16;
                float arrowY = cellRect.center().y;
                const char* arrow = m_sortAscending ? "▲" : "▼";
                dl.addText(font, Vec2(arrowX, textPos.y), arrow, theme.colors.textSecondary);
            }
        }

        // Column divider (for resizing)
        if (m_options.resizableColumns && col.resizable && i < m_columns.size() - 1) {
            float dividerX = cellRect.right();
            Rect dividerRect(dividerX - 3, headerRect.y(), 6, m_headerHeight);
            
            if (dividerRect.contains(input.mousePos())) {
                dl.addRectFilled(Rect(dividerX - 1, headerRect.y(), 2, m_headerHeight),
                                theme.colors.primary);
            }
        }

        // Cell border
        if (m_options.bordered) {
            dl.addLine(Vec2(cellRect.right(), cellRect.y()),
                      Vec2(cellRect.right(), cellRect.bottom()),
                      theme.colors.border.withAlpha((uint8_t)100));
        }

        x += col.width;
    }
}

void Table::handleColumnResize(Context& ctx) {
    InputState& input = ctx.input();

    // Start resizing
    if (m_resizingColumn < 0 && input.isMousePressed(MouseButton::Left)) {

        float x = m_bounds.x();
        for (size_t i = 0; i < m_columns.size() - 1; ++i) {
            x += m_columns[i].width;
            Rect dividerRect(x - 3, m_bounds.y(), 6, m_headerHeight);
            if (dividerRect.contains(input.mousePos()) && m_columns[i].resizable) {
                m_resizingColumn = (int)i;
                m_resizeStartX = input.mousePos().x;
                m_resizeStartWidth = m_columns[i].width;
                break;
            }
        }
    }

    // During resize
    if (m_resizingColumn >= 0) {
        if (input.isMouseDown(MouseButton::Left)) {
            float delta = input.mousePos().x - m_resizeStartX;
            float newWidth = m_resizeStartWidth + delta;
            newWidth = std::clamp(newWidth, m_columns[m_resizingColumn].minWidth, 
                                  m_columns[m_resizingColumn].maxWidth);
            m_columns[m_resizingColumn].width = newWidth;
            
            if (m_events.onColumnResize) {
                m_events.onColumnResize(m_resizingColumn, newWidth);
            }
        } else {
            m_resizingColumn = -1;
        }
    }
}

void Table::handleScroll(Context& ctx) {
    InputState& input = ctx.input();

    if (m_contentBounds.contains(input.mousePos())) {

        m_scrollOffset -= input.scrollDelta().y * m_rowHeight;
        // Will be clamped in end() when we know total rows
    }
}

bool Table::row(Context& ctx, const std::vector<std::string>& cells, bool selected) {
    if (!m_inTable) return false;

    const Theme& theme = ctx.theme();
    IDrawList& dl = *ctx.activeDrawList();
    Font* font = ctx.font();
    InputState& input = ctx.input();


    float y = m_contentBounds.y() + m_currentRow * m_rowHeight - m_scrollOffset;
    
    // Skip if outside visible area
    if (y + m_rowHeight < m_contentBounds.y() || y > m_contentBounds.bottom()) {
        m_currentRow++;
        return false;
    }

    Rect rowRect(m_contentBounds.x(), y, m_contentBounds.width(), m_rowHeight);
    
    // Clip to content area
    dl.pushClipRect(m_contentBounds);

    // Row background
    bool hovered = rowRect.contains(input.mousePos()) && m_contentBounds.contains(input.mousePos());
    if (hovered) {
        m_hoveredRow = m_currentRow;
    }

    Color bgColor;
    if (selected) {
        bgColor = theme.colors.selection;
    } else if (hovered) {
        bgColor = theme.colors.selection.withAlpha((uint8_t)60);
    } else if (m_options.alternateRowColors && m_currentRow % 2 == 1) {
        bgColor = theme.colors.panelBackground.darker(0.02f);
    } else {
        bgColor = Color(0, 0, 0, 0);
    }

    if (bgColor.a > 0) {
        dl.addRectFilled(rowRect, bgColor);
    }

    // Handle click
    bool clicked = false;
    bool canClick = hovered && !ctx.isInputCaptured() && !input.isMouseConsumed();
    if (canClick && input.isMousePressed(MouseButton::Left)) {
        m_clickedRow = m_currentRow;
        clicked = true;
        if (m_events.onRowClick) {
            m_events.onRowClick(m_currentRow);
        }
    }


    // Draw cells
    float x = rowRect.x();
    for (size_t i = 0; i < m_columns.size() && i < cells.size(); ++i) {
        const TableColumn& col = m_columns[i];
        Rect cellRect(x, y, col.width, m_rowHeight);

        if (font) {
            Vec2 textSize = font->measureText(cells[i]);
            Vec2 textPos;
            float padding = theme.metrics.paddingSmall;

            switch (col.alignment) {
                case Alignment::Center:
                    textPos.x = cellRect.x() + (cellRect.width() - textSize.x) * 0.5f;
                    break;
                case Alignment::End:
                    textPos.x = cellRect.right() - textSize.x - padding;
                    break;
                default:
                    textPos.x = cellRect.x() + padding;
            }
            textPos.y = cellRect.y() + (m_rowHeight - font->lineHeight()) * 0.5f;

            // Clip text if too wide
            dl.pushClipRect(cellRect);
            Color textColor = selected ? theme.colors.selectionText : theme.colors.text;
            dl.addText(font, textPos, cells[i].c_str(), textColor);
            dl.popClipRect();
        }

        // Cell border
        if (m_options.bordered) {
            dl.addLine(Vec2(cellRect.right(), cellRect.y()),
                      Vec2(cellRect.right(), cellRect.bottom()),
                      theme.colors.border.withAlpha((uint8_t)50));
        }

        x += col.width;
    }

    // Row bottom border
    if (m_options.bordered) {
        dl.addLine(Vec2(rowRect.x(), rowRect.bottom()),
                  Vec2(rowRect.right(), rowRect.bottom()),
                  theme.colors.border.withAlpha((uint8_t)50));
    }

    dl.popClipRect();
    m_currentRow++;
    return clicked;
}

void Table::end(Context& ctx) {
    if (!m_inTable) return;

    const Theme& theme = ctx.theme();
    IDrawList& dl = *ctx.activeDrawList();
    InputState& input = ctx.input();

    // Clamp scroll offset
    float totalHeight = m_currentRow * m_rowHeight;
    float maxScroll = std::max(0.0f, totalHeight - m_contentBounds.height());
    m_scrollOffset = std::clamp(m_scrollOffset, 0.0f, maxScroll);

    // Draw scrollbar if needed
    if (totalHeight > m_contentBounds.height()) {
        float scrollbarWidth = 8.0f;
        Rect track(m_contentBounds.right() - scrollbarWidth, m_contentBounds.y(),
                   scrollbarWidth, m_contentBounds.height());

        float thumbHeight = std::max(20.0f, 
            (m_contentBounds.height() / totalHeight) * track.height());
        float thumbY = track.y() + (m_scrollOffset / maxScroll) * (track.height() - thumbHeight);

        Rect thumb(track.x(), thumbY, track.width(), thumbHeight);
        Color thumbColor = track.contains(input.mousePos())
            ? theme.colors.scrollbarThumbHover
            : theme.colors.scrollbarThumb;
        dl.addRectFilled(thumb, thumbColor, scrollbarWidth / 2);
    }

    // Outer border
    dl.addRect(m_bounds, theme.colors.border, theme.metrics.borderRadiusSmall);

    m_inTable = false;
}

void Table::end() {
    auto wc = getWidgetContext();
    if (!wc.valid()) return;
    end(*wc.ctx);
}


//=============================================================================
// Immediate-Mode API
//=============================================================================

bool BeginTable(Context& ctx, const std::string& id, const std::vector<TableColumn>& columns,
                const TableOptions& options) {
    WidgetId widgetId = ctx.makeId(id.c_str());
    TableState& state = s_tableStates[widgetId];
    
    s_currentTable = &state;
    s_currentTableId = widgetId;
    
    state.columns = columns;
    state.options = options;
    state.currentRow = 0;
    state.clickedRow = -1;
    state.hoveredRow = -1;
    state.inTable = true;

    const Theme& theme = ctx.theme();
    Font* font = ctx.font();


    // Calculate dimensions
    float width = options.style.width > 0 ? options.style.width : 400.0f;
    float height = options.style.height > 0 ? options.style.height : 200.0f;

    state.bounds = allocateWidgetBounds(options.style, width, height);

    // Note: We intentionally do NOT call handleWidgetInteraction here.
    // Table handles its own row clicks directly without capturing input,
    // which allows other widgets to remain interactive.

    // Calculate heights
    state.rowHeight = options.rowHeight > 0 
        ? options.rowHeight 
        : (font ? font->lineHeight() + theme.metrics.paddingSmall * 2 : 24.0f);
    state.headerHeight = options.headerHeight > 0 
        ? options.headerHeight 
        : (font ? font->lineHeight() + theme.metrics.paddingMedium * 2 : 28.0f);

    float headerH = options.showHeader ? state.headerHeight : 0;
    state.contentBounds = Rect(state.bounds.x(), state.bounds.y() + headerH,
                               state.bounds.width(), state.bounds.height() - headerH);

    // Handle scrolling
    InputState& input = ctx.input();
    if (state.contentBounds.contains(input.mousePos())) {
        state.scrollOffset -= input.scrollDelta().y * state.rowHeight;
    }

    return true;
}

bool BeginTable(const std::string& id, const std::vector<TableColumn>& columns,
                const TableOptions& options) {
    auto wc = getWidgetContext();
    if (!wc.valid()) return false;
    return BeginTable(*wc.ctx, id, columns, options);
}


void TableHeader(Context& ctx, int sortColumn, bool sortAscending) {
    if (!s_currentTable || !s_currentTable->inTable) return;
    
    TableState& state = *s_currentTable;
    state.sortColumn = sortColumn;
    state.sortAscending = sortAscending;

    const Theme& theme = ctx.theme();
    IDrawList& dl = *ctx.activeDrawList();
    Font* font = ctx.font();
    InputState& input = ctx.input();


    if (!state.options.showHeader) return;

    Rect headerRect(state.bounds.x(), state.bounds.y(), state.bounds.width(), state.headerHeight);
    dl.addRectFilled(headerRect, theme.colors.panelBackground.darker(0.05f));
    dl.addLine(Vec2(headerRect.x(), headerRect.bottom()),
               Vec2(headerRect.right(), headerRect.bottom()),
               theme.colors.border);

    float x = headerRect.x();
    for (size_t i = 0; i < state.columns.size(); ++i) {
        const TableColumn& col = state.columns[i];
        Rect cellRect(x, headerRect.y(), col.width, state.headerHeight);

        bool hovered = cellRect.contains(input.mousePos());
        bool canClick = hovered && col.sortable && !ctx.isInputCaptured() && !input.isMouseConsumed();

        if (hovered && col.sortable) {
            dl.addRectFilled(cellRect, theme.colors.selection.withAlpha((uint8_t)30));
            
            if (canClick && input.isMousePressed(MouseButton::Left)) {
                if (state.sortColumn == (int)i) {
                    state.sortAscending = !state.sortAscending;
                } else {
                    state.sortColumn = (int)i;
                    state.sortAscending = true;
                }
            }
        }

        if (font) {
            Vec2 textSize = font->measureText(col.header);
            float padding = theme.metrics.paddingSmall;
            Vec2 textPos;

            switch (col.alignment) {
                case Alignment::Center:
                    textPos.x = cellRect.x() + (cellRect.width() - textSize.x) * 0.5f;
                    break;
                case Alignment::End:
                    textPos.x = cellRect.right() - textSize.x - padding;
                    break;
                default:
                    textPos.x = cellRect.x() + padding;
            }
            textPos.y = cellRect.y() + (state.headerHeight - font->lineHeight()) * 0.5f;

            dl.addText(font, textPos, col.header.c_str(), theme.colors.text);

            if (state.sortColumn == (int)i) {
                float arrowX = cellRect.right() - 16;
                const char* arrow = state.sortAscending ? "^" : "v";
                dl.addText(font, Vec2(arrowX, textPos.y), arrow, theme.colors.textSecondary);
            }
        }

        if (state.options.bordered) {
            dl.addLine(Vec2(cellRect.right(), cellRect.y()),
                      Vec2(cellRect.right(), cellRect.bottom()),
                      theme.colors.border.withAlpha((uint8_t)100));
        }

        x += col.width;
    }
}

void TableHeader(int sortColumn, bool sortAscending) {
    auto wc = getWidgetContext();
    if (!wc.valid()) return;
    TableHeader(*wc.ctx, sortColumn, sortAscending);
}


bool TableRow(Context& ctx, const std::vector<std::string>& cells, bool selected) {
    if (!s_currentTable || !s_currentTable->inTable) return false;

    TableState& state = *s_currentTable;
    const Theme& theme = ctx.theme();
    IDrawList& dl = *ctx.activeDrawList();
    Font* font = ctx.font();
    InputState& input = ctx.input();


    float y = state.contentBounds.y() + state.currentRow * state.rowHeight - state.scrollOffset;

    if (y + state.rowHeight < state.contentBounds.y() || y > state.contentBounds.bottom()) {
        state.currentRow++;
        return false;
    }

    Rect rowRect(state.contentBounds.x(), y, state.contentBounds.width(), state.rowHeight);
    dl.pushClipRect(state.contentBounds);

    bool hovered = rowRect.contains(input.mousePos()) && state.contentBounds.contains(input.mousePos());
    if (hovered) state.hoveredRow = state.currentRow;

    Color bgColor;
    if (selected) {
        bgColor = theme.colors.selection;
    } else if (hovered) {
        bgColor = theme.colors.selection.withAlpha((uint8_t)60);
    } else if (state.options.alternateRowColors && state.currentRow % 2 == 1) {
        bgColor = theme.colors.panelBackground.darker(0.02f);
    } else {
        bgColor = Color(0, 0, 0, 0);
    }

    if (bgColor.a > 0) {
        dl.addRectFilled(rowRect, bgColor);
    }

    bool clicked = false;
    // Only handle click if input is not captured by another widget
    bool canClick = hovered && !ctx.isInputCaptured() && !input.isMouseConsumed();
    if (canClick && input.isMousePressed(MouseButton::Left)) {

        state.clickedRow = state.currentRow;
        clicked = true;
    }

    float x = rowRect.x();
    for (size_t i = 0; i < state.columns.size() && i < cells.size(); ++i) {
        const TableColumn& col = state.columns[i];
        Rect cellRect(x, y, col.width, state.rowHeight);

        if (font) {
            Vec2 textSize = font->measureText(cells[i]);
            float padding = theme.metrics.paddingSmall;
            Vec2 textPos;

            switch (col.alignment) {
                case Alignment::Center:
                    textPos.x = cellRect.x() + (cellRect.width() - textSize.x) * 0.5f;
                    break;
                case Alignment::End:
                    textPos.x = cellRect.right() - textSize.x - padding;
                    break;
                default:
                    textPos.x = cellRect.x() + padding;
            }
            textPos.y = cellRect.y() + (state.rowHeight - font->lineHeight()) * 0.5f;

            dl.pushClipRect(cellRect);
            Color textColor = selected ? theme.colors.selectionText : theme.colors.text;
            dl.addText(font, textPos, cells[i].c_str(), textColor);
            dl.popClipRect();
        }

        if (state.options.bordered) {
            dl.addLine(Vec2(cellRect.right(), cellRect.y()),
                      Vec2(cellRect.right(), cellRect.bottom()),
                      theme.colors.border.withAlpha((uint8_t)50));
        }

        x += col.width;
    }

    if (state.options.bordered) {
        dl.addLine(Vec2(rowRect.x(), rowRect.bottom()),
                  Vec2(rowRect.right(), rowRect.bottom()),
                  theme.colors.border.withAlpha((uint8_t)50));
    }

    dl.popClipRect();
    state.currentRow++;
    return clicked;
}

bool TableRow(const std::vector<std::string>& cells, bool selected) {
    auto wc = getWidgetContext();
    if (!wc.valid()) return false;
    return TableRow(*wc.ctx, cells, selected);
}


void EndTable(Context& ctx) {
    if (!s_currentTable || !s_currentTable->inTable) return;

    TableState& state = *s_currentTable;
    const Theme& theme = ctx.theme();
    IDrawList& dl = *ctx.activeDrawList();
    InputState& input = ctx.input();


    float totalHeight = state.currentRow * state.rowHeight;
    float maxScroll = std::max(0.0f, totalHeight - state.contentBounds.height());
    state.scrollOffset = std::clamp(state.scrollOffset, 0.0f, maxScroll);

    if (totalHeight > state.contentBounds.height()) {
        float scrollbarWidth = 8.0f;
        Rect track(state.contentBounds.right() - scrollbarWidth, state.contentBounds.y(),
                   scrollbarWidth, state.contentBounds.height());

        float thumbHeight = std::max(20.0f,
            (state.contentBounds.height() / totalHeight) * track.height());
        float thumbY = track.y() + (state.scrollOffset / maxScroll) * (track.height() - thumbHeight);

        Rect thumb(track.x(), thumbY, track.width(), thumbHeight);
        Color thumbColor = track.contains(input.mousePos())
            ? theme.colors.scrollbarThumbHover
            : theme.colors.scrollbarThumb;
        dl.addRectFilled(thumb, thumbColor, scrollbarWidth / 2);
    }

    dl.addRect(state.bounds, theme.colors.border, theme.metrics.borderRadiusSmall);

    state.inTable = false;
    s_currentTable = nullptr;
    s_currentTableId = 0;
}

void EndTable() {
    auto wc = getWidgetContext();
    if (!wc.valid()) return;
    EndTable(*wc.ctx);
}


int GetTableClickedRow(Context& ctx) {
    (void)ctx;
    return s_currentTable ? s_currentTable->clickedRow : -1;
}

int GetTableClickedRow() {
    return GetTableClickedRow(*Context::current());
}

int GetTableSortColumn(Context& ctx) {
    (void)ctx;
    return s_currentTable ? s_currentTable->sortColumn : -1;
}

int GetTableSortColumn() {
    return GetTableSortColumn(*Context::current());
}

bool GetTableSortAscending(Context& ctx) {
    (void)ctx;
    return s_currentTable ? s_currentTable->sortAscending : true;
}

bool GetTableSortAscending() {
    return GetTableSortAscending(*Context::current());
}

void SetTableSort(Context& ctx, int column, bool ascending) {
    (void)ctx;
    if (s_currentTable) {
        s_currentTable->sortColumn = column;
        s_currentTable->sortAscending = ascending;
    }
}

void SetTableSort(int column, bool ascending) {
    if (Context::current()) SetTableSort(*Context::current(), column, ascending);
}


} // namespace fst
