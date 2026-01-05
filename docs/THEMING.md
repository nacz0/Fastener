# Theming Guide

Fastener features a powerful, flexible theming system that control every visual aspect of the library.

## 🌙 Using Built-in Themes

By default, Fastener doesn't apply a theme. You should set one during initialization:

```cpp
fst::Context ctx;

// Modern Dark Theme (Default recommendation)
ctx.setTheme(fst::Theme::dark());

// Clean Light Theme
ctx.setTheme(fst::Theme::light());
```

## 🎨 Global Customization

You can create a custom theme by modifying the `ThemeColors` and `ThemeMetrics` structs.

```cpp
fst::Theme customTheme = fst::Theme::dark();

// Change accent colors
customTheme.colors.primary = fst::Color::fromHex(0x6366f1); // Indigo
customTheme.colors.primaryHover = customTheme.colors.primary.lighter(0.1f);

// Change metrics
customTheme.metrics.borderRadius = 0.0f; // Flat design
customTheme.metrics.paddingMedium = 12.0f;

ctx.setTheme(customTheme);
```

## 🧩 Key Color Properties

The `ThemeColors` struct contains over 40 properties. The most important ones are:

| Property | Usage |
|----------|-------|
| `windowBackground` | Main application background. |
| `panelBackground` | Container background (slightly lighter/darker than window). |
| `primary` | Accent color for buttons, indicators, and focus. |
| `text` | Primary foreground text color. |
| `textSecondary` | Muted text for hints and labels. |
| `border` | Standard UI boundaries. |
| `selection` | Highlight color for text and lists. |

## 📐 Metrics Reference

| Property | Default (px) | Description |
|----------|--------------|-------------|
| `paddingMedium` | 8.0f | Standard gap between elements. |
| `borderRadius` | 6.0f | Corner rounding for most widgets. |
| `fontSize` | 14.0f | Base text size. |
| `buttonHeight` | 32.0f | Standard height for buttons and inputs. |
| `borderWidth` | 1.0f | Thickness of UI outlines. |

## 💅 Per-Widget Styling

Many widgets accept an `options` struct containing a `Style` object. This allows you to override the global theme for a specific instance.

```cpp
fst::Style dangerStyle;
dangerStyle.withBackground(fst::Color::red())
           .withTextColor(fst::Color::white())
           .withWidth(150);

fst::Button("DELETE PERMANENTLY", { .style = dangerStyle });
```

---
[Next: AI Context →](AI_CONTEXT.md)
