# API Reference

This document provides a technical reference for the core components of the Fastener library.

## 📐 Core Types

Found in `fastener/core/types.h`:

### `struct Vec2`
Basic 2D vector (float).
- `x`, `y` coordinates.
- Supports standard operators (`+`, `-`, `*`, `/`).
- `length()`, `normalized()`, `dot()`.

### `struct Rect`
Screen-space rectangle.
- `pos` (Vec2) and `size` (Vec2).
- `contains(Vec2)`: Hit testing.
- `intersects(Rect)`: Collision testing.
- `shrunk(padding)`, `expanded(amount)`: Geometric operations.
- `clipped(Rect)`: Intersection with another rectangle.

### `struct Color`
RGBA Color (0-255).
- `r`, `g`, `b`, `a` channels.
- `fromHex(0xRRGGBB)`: Hex utility.
- `fromHSL(h, s, l)`: Color space conversion.
- `lighter(amount)`, `darker(amount)`: Modification helpers.

---

## 🏗 Context & Lifecycle

### `class Context`
Manages the application state. One context is usually sufficient for an entire application.

| Method | Description |
|--------|-------------|
| `beginFrame(Window&)` | Starts a new UI frame. |
| `endFrame()` | Completes rendering for the current frame. |
| `setTheme(Theme)` | Sets the global visual style. |
| `loadFont(path, size)` | Loads a TTF/OTF font. |
| `pushId(string)` | Enters a new ID namespace. |
| `popId()` | Exits current ID namespace. |
| `deferRender(lambda)` | Schedules a command to be drawn last (for popups). |

---

## 🖥 Window & Input

### `class Window`
Platform abstraction for physical windows.

| Method | Description |
|--------|-------------|
| `pollEvents()` | Processes OS messages. |
| `swapBuffers()` | Displays the rendered frame. |
| `isOpen()` | Returns true if the window exists. |
| `setCursor(Cursor)` | Changes OS mouse cursor. |
| `dpiScale()` | Returns UI scale factor for high-DPI screens. |

### `class InputState`
Accessed via `ctx.input()`.
- `isKeyDown(Key)`, `isKeyPressed(Key)`.
- `mousePos()`, `mouseDelta()`, `scrollDelta()`.
- `isMousePressed(MouseButton)`.
- `modifiers()`: Access Ctrl, Shift, Alt state.

---

## 🎨 Graphics & Drawing

### `class DrawList`
The primary way to draw manual primitives.

| Method | Description |
|--------|-------------|
| `addRect(Rect, Color, rounding)` | Draws an outline rectangle. |
| `addRectFilled(Rect, Color, rounding)` | Draws a solid rectangle. |
| `addText(Font*, pos, string, Color)` | Renders text. |
| `addLine(p1, p2, Color, thickness)` | Draws a line segment. |
| `addCircleFilled(center, radius, Color)` | Draws a filled circle. |
| `addShadow(Rect, Color, size, rounding)` | Draws a soft drop-shadow. |

---

## 🎭 Theming

### `struct ThemeColors`
Configurable palette used by all widgets.
- `windowBackground`, `panelBackground`.
- `primary`, `primaryHover`, `primaryActive`.
- `text`, `textSecondary`, `textDisabled`.
- `border`, `inputBackground`, `selection`.

### `struct ThemeMetrics`
Spacing and sizing constants.
- `paddingSmall`, `paddingMedium`, `paddingLarge`.
- `borderRadius`, `borderWidth`.
- `buttonHeight`, `fontSize`, `scrollbarWidth`.

### `class Style`
Per-widget overrides. Many widgets accept a `Style` struct in their options.
```cpp
fst::Style myStyle;
myStyle.withWidth(200).withBackground(fst::Color::red());
fst::Button("Error", { myStyle });
```

---
[Next: Widget Catalog →](WIDGETS.md)
