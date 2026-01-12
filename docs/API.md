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

## 🎯 Drag and Drop System

Fastener provides a comprehensive drag-and-drop system for transferring data between widgets, with support for cross-window operations.

### Core Types

#### `struct DragPayload`
Data container for drag operations.
- `type` (string): Type identifier (must match between source/target)
- `data` (vector<uint8_t>): Serialized payload data
- `displayText` (string): Preview text shown during drag
- `sourceWidget` (WidgetId): Widget that initiated the drag
- `sourceWindow` (IPlatformWindow*): Source window for cross-window drags
- `isDelivered` (bool): Whether payload was accepted

Helper methods:
```cpp
payload.setData<int>(42);           // Set typed data
int value = payload.getData<int>(); // Get typed data
```

#### `enum DragDropFlags_`
Flags to customize drag-drop behavior:
- `DragDropFlags_None`: Default behavior
- `DragDropFlags_SourceNoPreviewTooltip`: Don't show preview tooltip
- `DragDropFlags_SourceNoDisableHover`: Keep source widget hover
- `DragDropFlags_SourceNoHoldToOpenOthers`: Don't open on hold
- `DragDropFlags_AcceptNoHighlight`: Don't highlight target
- `DragDropFlags_AcceptNoPreviewTooltip`: Don't show accept preview
- `DragDropFlags_CrossWindow`: Allow drag between windows

### Workflow Patterns

**Source (Drag Initiator):**
```cpp
if (fst::BeginDragDropSource()) {
    int itemIndex = 42;
    fst::SetDragDropPayload("MY_TYPE", &itemIndex, sizeof(int));
    fst::SetDragDropDisplayText("Dragging item 42");
    fst::EndDragDropSource();
}
```

**Target (Drop Receiver):**
```cpp
if (fst::BeginDragDropTarget()) {
    if (const auto* payload = fst::AcceptDragDropPayload("MY_TYPE")) {
        int droppedIndex = payload->getData<int>();
        handleDrop(droppedIndex);
    }
    fst::EndDragDropTarget();
}
```

### API Functions

| Function | Description |
|----------|-------------|
| `BeginDragDropSource(flags)` | Start drag source block, returns true if dragging |
| `SetDragDropPayload(type, data, size)` | Set typed payload data |
| `SetDragDropDisplayText(text)` | Set preview text |
| `EndDragDropSource()` | End drag source block |
| `BeginDragDropTarget()` | Start drop target block |
| `BeginDragDropTarget(rect)` | Start drop target with explicit bounds |
| `AcceptDragDropPayload(type, flags)` | Accept payload of specific type |
| `EndDragDropTarget()` | End drop target block |
| `IsDragDropActive()` | Check if drag in progress |
| `GetDragDropPayload()` | Peek at current payload |
| `CancelDragDrop()` | Abort current drag operation |

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
