#pragma once

#include "fastener/core/types.h"
#include "fastener/ui/style.h"
#include <string>
#include <vector>
#include <functional>

namespace fst {
class Context;


/**
 * @brief Definition of a table column.
 */
struct TableColumn {
    std::string id;                         ///< Unique column identifier
    std::string header;                     ///< Display text for header
    float width = 100.0f;                   ///< Column width in pixels
    float minWidth = 40.0f;                 ///< Minimum width when resizing
    float maxWidth = 500.0f;                ///< Maximum width when resizing
    Alignment alignment = Alignment::Start; ///< Text alignment in cells
    bool sortable = false;                  ///< Can this column be sorted?
    bool resizable = true;                  ///< Can this column be resized?
};

/**
 * @brief Options for customizing table appearance and behavior.
 */
struct TableOptions {
    Style style;
    float rowHeight = 0;                    ///< 0 = auto from font
    float headerHeight = 0;                 ///< 0 = auto from font
    bool showHeader = true;
    bool showRowNumbers = false;
    bool alternateRowColors = true;
    bool multiSelect = false;
    bool resizableColumns = true;
    bool bordered = true;                   ///< Show cell borders
    float scrollHeight = 0;                 ///< 0 = auto, >0 = fixed scroll area
};

/**
 * @brief Event callbacks for table interactions.
 */
struct TableEvents {
    std::function<void(int columnIndex, bool ascending)> onSort;
    std::function<void(int rowIndex)> onRowClick;
    std::function<void(int rowIndex)> onRowDoubleClick;
    std::function<void(int columnIndex, float newWidth)> onColumnResize;
};

/**
 * @brief Table widget for displaying tabular data.
 * 
 * Usage:
 * @code
 * std::vector<TableColumn> columns = {
 *     {"name", "Name", 200},
 *     {"size", "Size", 100, 50, 200, Alignment::End, true}
 * };
 * 
 * Table table;
 * table.setColumns(columns);
 * 
 * TableEvents events;
 * events.onSort = [](int col, bool asc) { ... };
 * 
 * table.begin("my_table", bounds, {}, events);
 * table.row({"file.txt", "1.2 KB"}, false);
 * table.row({"image.png", "4.5 MB"}, true);  // selected
 * table.end();
 * @endcode
 */
class Table {
public:
    Table();
    ~Table();

    /// Set column definitions
    void setColumns(const std::vector<TableColumn>& columns);
    const std::vector<TableColumn>& columns() const { return m_columns; }

    /// Get/set sort state
    int sortColumn() const { return m_sortColumn; }
    bool sortAscending() const { return m_sortAscending; }
    void setSort(int column, bool ascending);

    /// Begin rendering the table
    void begin(Context& ctx, const std::string& id, const Rect& bounds,
               const TableOptions& options = {},
               const TableEvents& events = {});
    void begin(const std::string& id, const Rect& bounds,
               const TableOptions& options = {},
               const TableEvents& events = {});

    /// Render a data row (call between begin/end)
    /// @param cells Cell values for each column
    /// @param selected Whether this row is selected
    /// @return true if row was clicked
    bool row(Context& ctx, const std::vector<std::string>& cells, bool selected = false);
    bool row(const std::vector<std::string>& cells, bool selected = false);


    /// End table rendering
    void end(Context& ctx);
    void end();


    /// Get the clicked row index (-1 if none)
    int clickedRow() const { return m_clickedRow; }

    /// Get scroll offset for virtual scrolling
    float scrollOffset() const { return m_scrollOffset; }
    void setScrollOffset(float offset) { m_scrollOffset = offset; }

private:
    std::vector<TableColumn> m_columns;
    int m_sortColumn = -1;
    bool m_sortAscending = true;
    
    // Render state
    std::string m_currentId;
    Rect m_bounds;
    Rect m_contentBounds;
    TableOptions m_options;
    TableEvents m_events;
    int m_currentRow = 0;
    int m_clickedRow = -1;
    int m_hoveredRow = -1;
    float m_scrollOffset = 0.0f;
    float m_rowHeight = 24.0f;
    float m_headerHeight = 28.0f;
    bool m_inTable = false;

    // Column resize state
    int m_resizingColumn = -1;
    float m_resizeStartX = 0.0f;
    float m_resizeStartWidth = 0.0f;

    void renderHeader(Context& ctx);
    void handleColumnResize(Context& ctx);
    void handleScroll(Context& ctx);

    float getTotalWidth() const;
};

// Immediate-mode API (simpler usage)

/**
 * @brief Begin an immediate-mode table.
 * @return true if table is visible and should render rows
 */
bool BeginTable(Context& ctx, const std::string& id, const std::vector<TableColumn>& columns,
                const TableOptions& options = {});
bool BeginTable(const std::string& id, const std::vector<TableColumn>& columns,
                const TableOptions& options = {});

/**
 * @brief Render table header row with sort indicators.
 * @param sortColumn Current sort column (-1 for none)
 * @param sortAscending Sort direction
 */
void TableHeader(Context& ctx, int sortColumn = -1, bool sortAscending = true);
void TableHeader(int sortColumn = -1, bool sortAscending = true);

/**
 * @brief Render a table row.
 * @param cells Cell values
 * @param selected Whether row is selected
 * @return true if row was clicked
 */
bool TableRow(Context& ctx, const std::vector<std::string>& cells, bool selected = false);
bool TableRow(const std::vector<std::string>& cells, bool selected = false);

/**
 * @brief End table rendering.
 */
void EndTable(Context& ctx);
void EndTable();

/**
 * @brief Get the row that was clicked in the current table (-1 if none).
 */
int GetTableClickedRow(Context& ctx);
int GetTableClickedRow();

/**
 * @brief Get/set the sort state for the current table.
 */
int GetTableSortColumn(Context& ctx);
int GetTableSortColumn();
bool GetTableSortAscending(Context& ctx);
bool GetTableSortAscending();
void SetTableSort(Context& ctx, int column, bool ascending);
void SetTableSort(int column, bool ascending);


} // namespace fst
