#pragma once

/**
 * @file flex_layout.h
 * @brief Flexbox-like automatic layout containers
 * 
 * @ai_hint HStack arranges children HORIZONTALLY with automatic spacing.
 *          VStack arranges children VERTICALLY with automatic spacing.
 *          Grid arranges children in a fixed-column grid.
 *          Use the MACRO variants for RAII-style scoping (recommended).
 * 
 * @example
 *   // Horizontal row of buttons with gap
 *   HStack(ctx, {.gap = 10}) {
 *       Button(ctx, "Save");
 *       Button(ctx, "Cancel");
 *   }
 *   
 *   // Vertical form with stretched children
 *   VStack(ctx, {.gap = 8, .crossAlign = Alignment::Stretch}) {
 *       Label(ctx, "Settings");
 *       TextInput(ctx, "name", userName);
 *       Spacer(ctx);  // Push remaining items to bottom
 *       Button(ctx, "Apply");
 *   }
 *   
 *   // 3-column grid
 *   Grid(ctx, {.columns = 3, .gap = 10}) {
 *       for (int i = 0; i < 9; ++i) {
 *           ctx.pushId(i);
 *           Button(ctx, std::to_string(i));
 *           ctx.popId();
 *       }
 *   }
 */

#include "fastener/core/types.h"
#include "fastener/ui/style.h"
#include <string>

namespace fst {

class Context;

//=============================================================================
// Flex Options - Common options for HStack/VStack containers
//=============================================================================
struct FlexOptions {
    Alignment mainAlign = Alignment::Start;    ///< Main axis alignment (row for HStack, column for VStack)
    Alignment crossAlign = Alignment::Start;   ///< Cross axis alignment
    float gap = 0.0f;                          ///< Gap between children (0 = use theme default)
    Vec4 padding = Vec4(0.0f);                 ///< Inner padding (top, right, bottom, left)
    Style style;                               ///< Container style (size, background, etc.)
    bool wrap = false;                         ///< Allow wrapping to next line/column (future)
};

//=============================================================================
// Grid Options - Options for Grid container
//=============================================================================
struct GridOptions {
    int columns = 2;                           ///< Number of columns
    float rowGap = 0.0f;                       ///< Gap between rows (0 = use theme default)
    float columnGap = 0.0f;                    ///< Gap between columns (0 = use theme default)
    Vec4 padding = Vec4(0.0f);                 ///< Inner padding
    Style style;                               ///< Container style
};

//=============================================================================
// Divider Options - Options for visual separator
//=============================================================================
struct DividerOptions {
    std::string label;                         ///< Optional label text
    float thickness = 1.0f;                    ///< Line thickness
    float margin = 8.0f;                       ///< Vertical/horizontal margin around divider
    Color color = Color::transparent();        ///< Custom color (transparent = use theme border)
};

//=============================================================================
// HStack - Horizontal Stack Container
//=============================================================================

/**
 * @brief RAII scope guard for horizontal stack layout.
 * 
 * Arranges child widgets horizontally from left to right with
 * automatic spacing between them.
 */
class HStackScope {
public:
    /// @brief Create horizontal stack with explicit context
    HStackScope(Context& ctx, const FlexOptions& options = {});
    ~HStackScope();
    
    /// @brief For if(HStack(ctx)) usage - always returns true
    operator bool() const { return true; }
    
    // Disable copy
    HStackScope(const HStackScope&) = delete;
    HStackScope& operator=(const HStackScope&) = delete;
    
private:
    Context* m_ctx;
};

/**
 * @brief Macro for RAII-style HStack usage
 * @param ctx Context reference
 * @param ... Optional FlexOptions
 * 
 * Usage: HStack(ctx, {.gap = 10}) { Button(ctx, "OK"); }
 */
#define HStack(ctx, ...) if (fst::HStackScope _hstack_##__LINE__{ctx, ##__VA_ARGS__})

/// @brief Begin a horizontal stack container (manual pair with EndHStack)
void BeginHStack(Context& ctx, const FlexOptions& options = {});

/// @brief End a horizontal stack container
void EndHStack(Context& ctx);

//=============================================================================
// VStack - Vertical Stack Container
//=============================================================================

/**
 * @brief RAII scope guard for vertical stack layout.
 * 
 * Arranges child widgets vertically from top to bottom with
 * automatic spacing between them.
 */
class VStackScope {
public:
    /// @brief Create vertical stack with explicit context
    VStackScope(Context& ctx, const FlexOptions& options = {});
    ~VStackScope();
    
    /// @brief For if(VStack(ctx)) usage - always returns true
    operator bool() const { return true; }
    
    // Disable copy
    VStackScope(const VStackScope&) = delete;
    VStackScope& operator=(const VStackScope&) = delete;
    
private:
    Context* m_ctx;
};

/**
 * @brief Macro for RAII-style VStack usage
 * @param ctx Context reference
 * @param ... Optional FlexOptions
 * 
 * Usage: VStack(ctx, {.gap = 8}) { Label(ctx, "Name"); TextInput(ctx, "id", val); }
 */
#define VStack(ctx, ...) if (fst::VStackScope _vstack_##__LINE__{ctx, ##__VA_ARGS__})

/// @brief Begin a vertical stack container (manual pair with EndVStack)
void BeginVStack(Context& ctx, const FlexOptions& options = {});

/// @brief End a vertical stack container
void EndVStack(Context& ctx);

//=============================================================================
// Grid - Grid Container
//=============================================================================

/**
 * @brief RAII scope guard for grid layout.
 * 
 * Arranges child widgets in a grid with a fixed number of columns.
 * Items automatically wrap to the next row.
 */
class GridScope {
public:
    /// @brief Create grid with explicit context
    GridScope(Context& ctx, const GridOptions& options = {});
    ~GridScope();
    
    /// @brief For if(Grid(ctx)) usage - always returns true
    operator bool() const { return true; }
    
    // Disable copy
    GridScope(const GridScope&) = delete;
    GridScope& operator=(const GridScope&) = delete;
    
private:
    Context* m_ctx;
};

/**
 * @brief Macro for RAII-style Grid usage
 * @param ctx Context reference
 * @param ... Optional GridOptions
 * 
 * Usage: Grid(ctx, {.columns = 3}) { for (...) Button(ctx, label); }
 */
#define Grid(ctx, ...) if (fst::GridScope _grid_##__LINE__{ctx, ##__VA_ARGS__})

/// @brief Begin a grid container (manual pair with EndGrid)
void BeginGrid(Context& ctx, const GridOptions& options = {});

/// @brief End a grid container
void EndGrid(Context& ctx);

//=============================================================================
// Layout Helpers
//=============================================================================

/**
 * @brief Insert flexible space that grows to fill available space.
 * 
 * Similar to SwiftUI's Spacer. In an HStack, pushes items apart horizontally.
 * In a VStack, pushes items apart vertically.
 * 
 * @param ctx Context reference
 * @param flex Flex grow factor (default 1.0)
 */
void Spacer(Context& ctx, float flex = 1.0f);

/**
 * @brief Insert a fixed-size space.
 * 
 * Unlike Spacer, this does not grow. It's a fixed gap.
 * 
 * @param ctx Context reference
 * @param size Size in pixels
 */
void FixedSpacer(Context& ctx, float size);

/**
 * @brief Insert a visual divider/separator line.
 * 
 * In HStack: vertical line
 * In VStack: horizontal line
 * 
 * @param ctx Context reference
 * @param options Divider styling options
 */
void Divider(Context& ctx, const DividerOptions& options = {});

} // namespace fst
