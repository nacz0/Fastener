#pragma once

/**
 * @file constants.h
 * @brief Library-wide constants to replace magic numbers.
 */

namespace fst {
namespace constants {

//=============================================================================
// Math Constants
//=============================================================================

/// Small epsilon for preventing division by zero in color calculations
constexpr float EPSILON = 1e-20f;

/// Tolerance for floating-point comparisons
constexpr float FLOAT_TOLERANCE = 0.0001f;

//=============================================================================
// DrawList Buffer Sizes
//=============================================================================

/// Default vertex buffer reservation size
constexpr int DEFAULT_VERTEX_RESERVE = 1024;

/// Default index buffer reservation size
constexpr int DEFAULT_INDEX_RESERVE = 2048;

/// Default command buffer reservation size
constexpr int DEFAULT_COMMAND_RESERVE = 64;

//=============================================================================
// Font Atlas
//=============================================================================

/// Default font atlas width in pixels
constexpr int DEFAULT_ATLAS_WIDTH = 512;

/// Default font atlas height in pixels
constexpr int DEFAULT_ATLAS_HEIGHT = 512;

/// Maximum allowed atlas size
constexpr int MAX_ATLAS_SIZE = 4096;

/// Atlas padding between glyphs
constexpr int ATLAS_GLYPH_PADDING = 2;

//=============================================================================
// Circle/Arc Rendering
//=============================================================================

/// Minimum number of segments for circle approximation
constexpr int MIN_CIRCLE_SEGMENTS = 12;

/// Segments per unit radius for adaptive circle tessellation
constexpr float CIRCLE_SEGMENTS_PER_RADIUS = 0.5f;

//=============================================================================
// Input
//=============================================================================

/// Time window for double-click detection in seconds
constexpr float DOUBLE_CLICK_TIME = 0.3f;

/// Invalid time value for click detection initialization
constexpr float INVALID_CLICK_TIME = -1.0f;

//=============================================================================
// UI Defaults
//=============================================================================

/// Default border radius for rounded rectangles
constexpr float DEFAULT_BORDER_RADIUS = 4.0f;

/// Default shadow blur radius
constexpr float DEFAULT_SHADOW_BLUR = 8.0f;

/// Default shadow offset
constexpr float DEFAULT_SHADOW_OFFSET = 4.0f;

} // namespace constants
} // namespace fst
