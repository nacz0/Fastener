/**
 * @file flex_layout.cpp
 * @brief Implementation of Flexbox-like layout containers
 */

#include "fastener/ui/flex_layout.h"
#include "fastener/core/context.h"
#include "fastener/graphics/draw_list.h"
#include "fastener/graphics/font.h"
#include "fastener/ui/layout.h"
#include "fastener/ui/theme.h"
#include "fastener/ui/widget.h"
#include "fastener/ui/widget_utils.h"

namespace fst {

//=============================================================================
// HStack Implementation
//=============================================================================

HStackScope::HStackScope(Context& ctx, const FlexOptions& options)
    : m_ctx(&ctx)
{
    BeginHStack(ctx, options);
}

HStackScope::~HStackScope() {
    if (m_ctx) {
        EndHStack(*m_ctx);
    }
}

void BeginHStack(Context& ctx, const FlexOptions& options) {
    LayoutContext& lc = ctx.layout();
    const Theme& theme = ctx.theme();
    
    // Determine container bounds
    Rect bounds;
    if (options.style.width > 0 || options.style.height > 0) {
        float w = options.style.width > 0 ? options.style.width : lc.currentBounds().width();
        float h = options.style.height > 0 ? options.style.height : theme.metrics.buttonHeight;
        bounds = allocateWidgetBounds(ctx, options.style, w, h);
    } else {
        bounds = lc.allocateRemaining();
    }
    
    // Draw background if specified
    if (options.style.backgroundColor.a > 0) {
        IDrawList& dl = *ctx.activeDrawList();
        float radius = options.style.borderRadius > 0 ? options.style.borderRadius : 0.0f;
        dl.addRectFilled(bounds, options.style.backgroundColor, radius);
        
        if (options.style.borderWidth > 0) {
            Color borderColor = options.style.borderColor.a > 0 
                ? options.style.borderColor 
                : theme.colors.border;
            dl.addRect(bounds, borderColor, radius);
        }
    }
    
    // Apply padding
    Rect contentBounds = bounds;
    if (options.padding.x > 0 || options.padding.y > 0 || 
        options.padding.z > 0 || options.padding.w > 0) {
        contentBounds = bounds.shrunk(options.padding);
    } else if (options.style.padding.x > 0) {
        contentBounds = bounds.shrunk(options.style.padding);
    }
    
    // Begin horizontal container
    lc.beginContainer(contentBounds, LayoutDirection::Horizontal);
    
    // Set spacing
    float gap = options.gap > 0 ? options.gap : theme.metrics.itemSpacing;
    lc.setSpacing(gap);
    
    // Set alignment
    lc.setAlignment(options.mainAlign, options.crossAlign);
}

void EndHStack(Context& ctx) {
    ctx.layout().endContainer();
}

//=============================================================================
// VStack Implementation
//=============================================================================

VStackScope::VStackScope(Context& ctx, const FlexOptions& options)
    : m_ctx(&ctx)
{
    BeginVStack(ctx, options);
}

VStackScope::~VStackScope() {
    if (m_ctx) {
        EndVStack(*m_ctx);
    }
}

void BeginVStack(Context& ctx, const FlexOptions& options) {
    LayoutContext& lc = ctx.layout();
    const Theme& theme = ctx.theme();
    
    // Determine container bounds
    Rect bounds;
    if (options.style.width > 0 || options.style.height > 0) {
        float w = options.style.width > 0 ? options.style.width : lc.currentBounds().width();
        float h = options.style.height > 0 ? options.style.height : 200.0f;
        bounds = allocateWidgetBounds(ctx, options.style, w, h);
    } else {
        bounds = lc.allocateRemaining();
    }
    
    // Draw background if specified
    if (options.style.backgroundColor.a > 0) {
        IDrawList& dl = *ctx.activeDrawList();
        float radius = options.style.borderRadius > 0 ? options.style.borderRadius : 0.0f;
        dl.addRectFilled(bounds, options.style.backgroundColor, radius);
        
        if (options.style.borderWidth > 0) {
            Color borderColor = options.style.borderColor.a > 0 
                ? options.style.borderColor 
                : theme.colors.border;
            dl.addRect(bounds, borderColor, radius);
        }
    }
    
    // Apply padding
    Rect contentBounds = bounds;
    if (options.padding.x > 0 || options.padding.y > 0 || 
        options.padding.z > 0 || options.padding.w > 0) {
        contentBounds = bounds.shrunk(options.padding);
    } else if (options.style.padding.x > 0) {
        contentBounds = bounds.shrunk(options.style.padding);
    }
    
    // Begin vertical container
    lc.beginContainer(contentBounds, LayoutDirection::Vertical);
    
    // Set spacing
    float gap = options.gap > 0 ? options.gap : theme.metrics.itemSpacing;
    lc.setSpacing(gap);
    
    // Set alignment
    lc.setAlignment(options.mainAlign, options.crossAlign);
}

void EndVStack(Context& ctx) {
    ctx.layout().endContainer();
}

//=============================================================================
// Grid Implementation
//=============================================================================

// Grid tracking state (thread-local for nested grids)
struct GridState {
    Context* ctx = nullptr;
    int columns = 2;
    int currentColumn = 0;
    int currentRow = 0;
    float columnWidth = 0.0f;
    float rowHeight = 0.0f;
    float columnGap = 0.0f;
    float rowGap = 0.0f;
    Rect bounds;
    Vec2 startPos;
    Vec2 currentPos;
    float maxRowHeight = 0.0f;  // Track max height in current row
};

static thread_local std::vector<GridState> s_gridStack;

// Helper to get current grid state
static GridState* currentGrid() {
    return s_gridStack.empty() ? nullptr : &s_gridStack.back();
}

GridScope::GridScope(Context& ctx, const GridOptions& options)
    : m_ctx(&ctx)
{
    BeginGrid(ctx, options);
}

GridScope::~GridScope() {
    if (m_ctx) {
        EndGrid(*m_ctx);
    }
}

void BeginGrid(Context& ctx, const GridOptions& options) {
    LayoutContext& lc = ctx.layout();
    const Theme& theme = ctx.theme();
    
    // Determine container bounds
    Rect bounds;
    if (options.style.width > 0 || options.style.height > 0) {
        float w = options.style.width > 0 ? options.style.width : lc.currentBounds().width();
        float h = options.style.height > 0 ? options.style.height : 200.0f;
        bounds = allocateWidgetBounds(ctx, options.style, w, h);
    } else {
        bounds = lc.allocateRemaining();
    }
    
    // Draw background if specified
    if (options.style.backgroundColor.a > 0) {
        IDrawList& dl = *ctx.activeDrawList();
        float radius = options.style.borderRadius > 0 ? options.style.borderRadius : 0.0f;
        dl.addRectFilled(bounds, options.style.backgroundColor, radius);
        
        if (options.style.borderWidth > 0) {
            Color borderColor = options.style.borderColor.a > 0 
                ? options.style.borderColor 
                : theme.colors.border;
            dl.addRect(bounds, borderColor, radius);
        }
    }
    
    // Apply padding
    Rect contentBounds = bounds;
    if (options.padding.x > 0 || options.padding.y > 0 || 
        options.padding.z > 0 || options.padding.w > 0) {
        contentBounds = bounds.shrunk(options.padding);
    } else if (options.style.padding.x > 0) {
        contentBounds = bounds.shrunk(options.style.padding);
    }
    
    // Calculate gaps
    float colGap = options.columnGap > 0 ? options.columnGap : theme.metrics.itemSpacing;
    float rGap = options.rowGap > 0 ? options.rowGap : theme.metrics.itemSpacing;
    
    // Calculate column width
    int cols = options.columns > 0 ? options.columns : 2;
    float totalGapWidth = colGap * (cols - 1);
    float columnWidth = (contentBounds.width() - totalGapWidth) / cols;
    
    // Push grid state
    GridState state;
    state.ctx = &ctx;
    state.columns = cols;
    state.currentColumn = 0;
    state.currentRow = 0;
    state.columnWidth = columnWidth;
    state.rowHeight = theme.metrics.buttonHeight;  // Default row height
    state.columnGap = colGap;
    state.rowGap = rGap;
    state.bounds = contentBounds;
    state.startPos = contentBounds.pos;
    state.currentPos = contentBounds.pos;
    state.maxRowHeight = 0.0f;
    s_gridStack.push_back(state);
    
    // Begin a vertical container for rows
    lc.beginContainer(contentBounds, LayoutDirection::Vertical);
    lc.setSpacing(rGap);
}

void EndGrid(Context& ctx) {
    if (!s_gridStack.empty()) {
        s_gridStack.pop_back();
    }
    ctx.layout().endContainer();
}

// Custom allocate function for grid items - called by widgets inside Grid
Rect GridAllocate(Context& ctx, float width, float height) {
    GridState* grid = currentGrid();
    if (!grid) {
        // Fallback to normal allocation
        return ctx.layout().allocate(width, height);
    }
    
    // Calculate position
    float x = grid->currentPos.x + grid->currentColumn * (grid->columnWidth + grid->columnGap);
    float y = grid->currentPos.y + grid->currentRow * (grid->rowHeight + grid->rowGap);
    
    // Clamp width to column width
    float itemWidth = std::min(width, grid->columnWidth);
    float itemHeight = height;
    
    // Track max height for this row
    grid->maxRowHeight = std::max(grid->maxRowHeight, itemHeight);
    
    // Move to next column
    grid->currentColumn++;
    if (grid->currentColumn >= grid->columns) {
        grid->currentColumn = 0;
        grid->currentRow++;
        grid->rowHeight = std::max(grid->rowHeight, grid->maxRowHeight);
        grid->maxRowHeight = 0.0f;
    }
    
    return Rect(x, y, itemWidth, itemHeight);
}

//=============================================================================
// Layout Helpers
//=============================================================================

void Spacer(Context& ctx, float flex) {
    LayoutContext& lc = ctx.layout();
    
    // Get remaining space and allocate it
    float remaining = lc.remainingSpace();
    if (remaining > 0) {
        if (lc.currentDirection() == LayoutDirection::Horizontal) {
            lc.allocate(remaining, 0, flex);
        } else {
            lc.allocate(0, remaining, flex);
        }
    }
}

void FixedSpacer(Context& ctx, float size) {
    LayoutContext& lc = ctx.layout();
    
    if (lc.currentDirection() == LayoutDirection::Horizontal) {
        lc.allocate(size, 0, 0.0f);
    } else {
        lc.allocate(0, size, 0.0f);
    }
}

void Divider(Context& ctx, const DividerOptions& options) {
    LayoutContext& lc = ctx.layout();
    const Theme& theme = ctx.theme();
    IDrawList& dl = *ctx.activeDrawList();
    Font* font = ctx.font();
    
    // Use a visible color - border color or slightly lighter than background
    Color lineColor = options.color.a > 0 ? options.color : theme.colors.border.lighter(0.3f);
    
    bool isHorizontal = (lc.currentDirection() == LayoutDirection::Vertical);
    
    if (isHorizontal) {
        // Horizontal divider (in VStack)
        float thickness = std::max(1.0f, options.thickness);
        float totalHeight = options.margin * 2 + thickness;
        
        // Get parent container bounds for width
        Rect containerBounds = lc.currentBounds();
        float lineWidth = containerBounds.width();
        
        // Fallback: if width is 0, use a reasonable default
        if (lineWidth <= 0) {
            lineWidth = 200.0f;
        }
        
        Rect bounds = lc.allocate(lineWidth, totalHeight, 0.0f);
        
        float lineY = bounds.y() + options.margin;
        
        if (options.label.empty()) {
            // Simple line - draw full width
            dl.addRectFilled(
                Rect(bounds.x(), lineY, lineWidth, thickness),
                lineColor
            );
        } else {
            // Line with centered label
            float textWidth = font ? font->measureText(options.label).x : 0;
            float labelPadding = 8.0f;
            float centerX = bounds.center().x;
            
            // Left line
            dl.addRectFilled(
                Rect(bounds.x(), lineY - options.thickness / 2, 
                     centerX - textWidth / 2 - labelPadding - bounds.x(), options.thickness),
                lineColor
            );
            
            // Label
            if (font) {
                Vec2 textPos(centerX - textWidth / 2, bounds.y() + options.margin - 2);
                dl.addText(font, textPos, options.label, theme.colors.textSecondary);
            }
            
            // Right line
            float rightStart = centerX + textWidth / 2 + labelPadding;
            dl.addRectFilled(
                Rect(rightStart, lineY - options.thickness / 2, 
                     bounds.right() - rightStart, options.thickness),
                lineColor
            );
        }
    } else {
        // Vertical divider (in HStack)
        float totalWidth = options.margin * 2 + options.thickness;
        Rect bounds = lc.allocate(totalWidth, lc.currentBounds().height(), 0.0f);
        
        float lineX = bounds.x() + options.margin + options.thickness / 2;
        
        dl.addRectFilled(
            Rect(lineX - options.thickness / 2, bounds.y(), options.thickness, bounds.height()),
            lineColor
        );
    }
}

} // namespace fst
