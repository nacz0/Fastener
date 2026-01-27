# Theming Guide

Fastener has a flexible theming system that controls the visual style of the library.

## Using Built-in Themes

Fastener does not apply a theme by default. Set one during initialization:

```cpp
fst::Context ctx;

// Dark theme
ctx.setTheme(fst::Theme::dark());

// Light theme
ctx.setTheme(fst::Theme::light());
```

## Global Customization

Customize a theme by modifying ThemeColors and ThemeMetrics.

```cpp
fst::Theme customTheme = fst::Theme::dark();

// Change accent colors
customTheme.colors.primary = fst::Color::fromHex(0x6366f1);
customTheme.colors.primaryHover = customTheme.colors.primary.lighter(0.1f);

// Change metrics
customTheme.metrics.borderRadius = 0.0f;
customTheme.metrics.paddingMedium = 12.0f;

ctx.setTheme(customTheme);
```

## Key Color Properties

| Property | Usage |
|----------|-------|
| `windowBackground` | Main application background |
| `panelBackground` | Container background |
| `primary` | Accent color for buttons and focus |
| `text` | Primary foreground text color |
| `textSecondary` | Muted text for hints and labels |
| `border` | Standard UI boundaries |
| `selection` | Highlight color for text and lists |

## Metrics Reference

| Property | Default (px) | Description |
|----------|--------------|-------------|
| `paddingMedium` | 8.0f | Standard gap between elements |
| `borderRadius` | 6.0f | Corner rounding for most widgets |
| `fontSize` | 14.0f | Base text size |
| `buttonHeight` | 32.0f | Standard height for buttons and inputs |
| `borderWidth` | 1.0f | Thickness of UI outlines |

## Per-Widget Styling

Many widgets accept an options struct containing a Style object to override theme defaults.

```cpp
fst::Style dangerStyle;
dangerStyle.withBackground(fst::Color::red())
           .withTextColor(fst::Color::white())
           .withWidth(150);

fst::Button(ctx, "DELETE", { .style = dangerStyle });
```

---
Next: AI_CONTEXT.md
