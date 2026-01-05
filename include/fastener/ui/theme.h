#pragma once

#include "fastener/core/types.h"

namespace fst {

//=============================================================================
// Theme Colors
//=============================================================================
struct ThemeColors {
    // Background
    Color windowBackground;
    Color panelBackground;
    Color popupBackground;
    
    // Primary elements
    Color primary;
    Color primaryHover;
    Color primaryActive;
    Color primaryText;
    
    // Secondary elements
    Color secondary;
    Color secondaryHover;
    Color secondaryActive;
    
    // Neutral
    Color text;
    Color textDisabled;
    Color textSecondary;
    
    // Borders
    Color border;
    Color borderHover;
    Color borderFocused;
    
    // Input fields
    Color inputBackground;
    Color inputBorder;
    Color inputFocused;
    
    // Button
    Color buttonBackground;
    Color buttonHover;
    Color buttonActive;
    Color buttonText;
    
    // Status colors
    Color success;
    Color warning;
    Color error;
    Color info;
    
    // Selection
    Color selection;
    Color selectionText;
    
    // Scrollbar
    Color scrollbarTrack;
    Color scrollbarThumb;
    Color scrollbarThumbHover;
    
    // Shadow
    Color shadow;
    
    // Tooltip
    Color tooltipBackground;
    Color tooltipText;
    Color tooltipBorder;
};

//=============================================================================
// Theme Metrics
//=============================================================================
struct ThemeMetrics {
    // Spacing
    float paddingSmall = 4.0f;
    float paddingMedium = 8.0f;
    float paddingLarge = 16.0f;
    
    float marginSmall = 4.0f;
    float marginMedium = 8.0f;
    float marginLarge = 16.0f;
    
    float itemSpacing = 8.0f;
    
    // Sizing
    float buttonHeight = 32.0f;
    float inputHeight = 32.0f;
    float sliderHeight = 20.0f;
    float checkboxSize = 18.0f;
    float scrollbarWidth = 12.0f;
    
    // Borders
    float borderWidth = 1.0f;
    float borderRadius = 6.0f;
    float borderRadiusSmall = 4.0f;
    float borderRadiusLarge = 8.0f;
    
    // Font
    float fontSize = 14.0f;
    float fontSizeSmall = 12.0f;
    float fontSizeLarge = 18.0f;
    float fontSizeHeading = 24.0f;
    
    // Animation
    float animationDuration = 0.15f;
    
    // Shadow
    float shadowSize = 8.0f;
    float shadowOffset = 2.0f;
};

//=============================================================================
// Theme
//=============================================================================
class Theme {
public:
    ThemeColors colors;
    ThemeMetrics metrics;
    
    Theme() = default;
    
    // Built-in themes
    static Theme dark();
    static Theme light();
    
    // Custom theme creation
    static Theme custom(const ThemeColors& colors, const ThemeMetrics& metrics = {});
};

//=============================================================================
// Dark Theme
//=============================================================================
inline Theme Theme::dark() {
    Theme theme;
    auto& c = theme.colors;
    
    // Background
    c.windowBackground = Color::fromHex(0x1a1a1a);
    c.panelBackground = Color::fromHex(0x242424);
    c.popupBackground = Color::fromHex(0x363636);  // Brighter for better visibility
    
    // Primary
    c.primary = Color::fromHex(0x3b82f6);
    c.primaryHover = Color::fromHex(0x60a5fa);
    c.primaryActive = Color::fromHex(0x2563eb);
    c.primaryText = Color::fromHex(0xffffff);
    
    // Secondary
    c.secondary = Color::fromHex(0x4b5563);
    c.secondaryHover = Color::fromHex(0x6b7280);
    c.secondaryActive = Color::fromHex(0x374151);
    
    // Text
    c.text = Color::fromHex(0xf3f4f6);
    c.textDisabled = Color::fromHex(0x6b7280);
    c.textSecondary = Color::fromHex(0x9ca3af);
    
    // Borders
    c.border = Color::fromHex(0x3f3f3f);
    c.borderHover = Color::fromHex(0x525252);
    c.borderFocused = Color::fromHex(0x3b82f6);
    
    // Input
    c.inputBackground = Color::fromHex(0x1f1f1f);
    c.inputBorder = Color::fromHex(0x3f3f3f);
    c.inputFocused = Color::fromHex(0x3b82f6);
    
    // Button
    c.buttonBackground = Color::fromHex(0x3f3f3f);
    c.buttonHover = Color::fromHex(0x525252);
    c.buttonActive = Color::fromHex(0x2d2d2d);
    c.buttonText = Color::fromHex(0xf3f4f6);
    
    // Status
    c.success = Color::fromHex(0x22c55e);
    c.warning = Color::fromHex(0xf59e0b);
    c.error = Color::fromHex(0xef4444);
    c.info = Color::fromHex(0x3b82f6);
    
    // Selection
    c.selection = Color::fromHex(0x3b82f6);
    c.selectionText = Color::fromHex(0xffffff);
    
    // Scrollbar
    c.scrollbarTrack = Color::fromHex(0x262626);
    c.scrollbarThumb = Color::fromHex(0x525252);
    c.scrollbarThumbHover = Color::fromHex(0x6b7280);
    
    // Shadow
    c.shadow = Color(0, 0, 0, 80);
    
    // Tooltip
    c.tooltipBackground = Color::fromHex(0x1f1f1f);
    c.tooltipText = Color::fromHex(0xf3f4f6);
    c.tooltipBorder = Color::fromHex(0x525252);
    
    return theme;
}

//=============================================================================
// Light Theme
//=============================================================================
inline Theme Theme::light() {
    Theme theme;
    auto& c = theme.colors;
    
    // Background
    c.windowBackground = Color::fromHex(0xf8fafc);
    c.panelBackground = Color::fromHex(0xffffff);
    c.popupBackground = Color::fromHex(0xffffff);
    
    // Primary
    c.primary = Color::fromHex(0x3b82f6);
    c.primaryHover = Color::fromHex(0x2563eb);
    c.primaryActive = Color::fromHex(0x1d4ed8);
    c.primaryText = Color::fromHex(0xffffff);
    
    // Secondary
    c.secondary = Color::fromHex(0xe5e7eb);
    c.secondaryHover = Color::fromHex(0xd1d5db);
    c.secondaryActive = Color::fromHex(0x9ca3af);
    
    // Text
    c.text = Color::fromHex(0x1f2937);
    c.textDisabled = Color::fromHex(0x9ca3af);
    c.textSecondary = Color::fromHex(0x6b7280);
    
    // Borders
    c.border = Color::fromHex(0xe5e7eb);
    c.borderHover = Color::fromHex(0xd1d5db);
    c.borderFocused = Color::fromHex(0x3b82f6);
    
    // Input
    c.inputBackground = Color::fromHex(0xffffff);
    c.inputBorder = Color::fromHex(0xd1d5db);
    c.inputFocused = Color::fromHex(0x3b82f6);
    
    // Button
    c.buttonBackground = Color::fromHex(0xf3f4f6);
    c.buttonHover = Color::fromHex(0xe5e7eb);
    c.buttonActive = Color::fromHex(0xd1d5db);
    c.buttonText = Color::fromHex(0x1f2937);
    
    // Status
    c.success = Color::fromHex(0x22c55e);
    c.warning = Color::fromHex(0xf59e0b);
    c.error = Color::fromHex(0xef4444);
    c.info = Color::fromHex(0x3b82f6);
    
    // Selection
    c.selection = Color::fromHex(0x3b82f6);
    c.selectionText = Color::fromHex(0xffffff);
    
    // Scrollbar
    c.scrollbarTrack = Color::fromHex(0xf3f4f6);
    c.scrollbarThumb = Color::fromHex(0xd1d5db);
    c.scrollbarThumbHover = Color::fromHex(0x9ca3af);
    
    // Shadow
    c.shadow = Color(0, 0, 0, 40);
    
    // Tooltip
    c.tooltipBackground = Color::fromHex(0x1f2937);
    c.tooltipText = Color::fromHex(0xf8fafc);
    c.tooltipBorder = Color::fromHex(0x4b5563);
    
    return theme;
}

inline Theme Theme::custom(const ThemeColors& colors, const ThemeMetrics& metrics) {
    Theme theme;
    theme.colors = colors;
    theme.metrics = metrics;
    return theme;
}

} // namespace fst
