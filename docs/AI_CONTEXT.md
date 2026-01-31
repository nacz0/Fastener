# Fastener Library AI Context

This file provides a structured summary of the Fastener API for AI assistants.

## Library Overview

- Name: Fastener (namespace `fst`)
- Type: Immediate-mode GUI with retained internal state
- Language: C++17
- Coordinates: screen-space (0,0 is top-left)
- Main header: `#include "fastener/fastener.h"`
- Platforms: Windows and Linux (X11)

## Core Data Types

- `fst::Vec2(x, y)`
- `fst::Rect(x, y, w, h)` or `fst::Rect(pos, size)`
- `fst::Color(r, g, b, a)` or `fst::Color::fromHex(0xRRGGBB)`
- `fst::WidgetId` (uint64_t)

## Lifecycle Pattern

```cpp
fst::Window window("Title", 1280, 720);
fst::Context ctx;
ctx.loadFont("path.ttf", 14.0f);
ctx.setTheme(fst::Theme::dark());

while (window.isOpen()) {
    window.pollEvents();
    ctx.beginFrame(window);
    // UI calls
    ctx.endFrame();
    window.swapBuffers();
}
```

## Widget API Quick Reference

All widget functions take `Context& ctx` as the first parameter.

| Widget | Signature (summary) | Notes |
|--------|---------------------|-------|
| Button | `bool Button(ctx, label, options)` | True on click |
| Label | `void Label(ctx, text, options)` | `LabelHeading`, `LabelSecondary` |
| TextInput | `bool TextInput(ctx, id, string&, options)` | `TextInputWithLabel` |
| TextArea | `bool TextArea(ctx, id, string&, options)` | Multi-line input |
| Checkbox | `bool Checkbox(ctx, label, bool&, options)` | True on toggle |
| ToggleSwitch | `bool ToggleSwitch(ctx, label, bool&, options)` | iOS-style switch |
| RadioButton | `bool RadioButton(ctx, label, int& selected, int index, options)` | Group selection |
| Slider | `bool Slider(ctx, label, float&, min, max, options)` | `SliderInt` |
| InputNumber | `bool InputNumber(ctx, label, float&, min, max, options)` | `InputNumberInt` |
| Panel | `Panel(ctx, id, options) { ... }` | RAII container macro |
| Card | `Card(ctx, id, options) { ... }` | Emphasized container |
| ScrollArea | `ScrollArea::render(ctx, id, bounds, fn)` | Scrollable region |
| Splitter | `bool Splitter(ctx, id, float&, bounds, options)` | Resizable divider |
| TabControl | `tabs.render(ctx, id, bounds, options, events)` | Class-based |
| TreeView | `tree.render(ctx, id, bounds, options, events)` | Class-based |
| Breadcrumb | `int Breadcrumb(ctx, items)` | Returns clicked index |
| Listbox | `bool Listbox(ctx, label, int&, items)` | `ListboxMulti` |
| Selectable | `bool Selectable(ctx, label, bool&, options)` | List item |
| Table | `table.begin(ctx, id, bounds)` | Also immediate-mode API |
| ProgressBar | `void ProgressBar(ctx, value, options)` | 0.0 to 1.0 |
| Spinner | `void Spinner(ctx, id, options)` | Animated indicator |
| Tooltip | `void Tooltip(ctx, text, options)` | Call after widget |
| ColorPicker | `bool ColorPicker(ctx, id, Color&, options)` | Interactive color |
| Image | `void Image(ctx, texture, size, options)` | Texture display |
| Separator | `void Separator(ctx, options)` | Divider line |
| CollapsingHeader | `bool CollapsingHeader(ctx, label, bool& open, options)` | Collapsible section |
| MenuBar | `menu.render(ctx, bounds); menu.renderPopups(ctx);` | Menu system |
| CommandPalette | `palette.render(ctx, options)` | Ctrl+Shift+P by default |
| Modal | `BeginModal(ctx, id, bool& open, options)` | Centered dialog |
| StatusBar | `BeginStatusBar(ctx)` + `StatusBarSection(ctx, text)` | Bottom bar |
| Badge | `Badge(ctx, count/text, options)` | Notification badge |
| Toast | `ShowToast(ctx, message, options)` + `RenderToasts(ctx)` | Temporary notifications |
| Docking | `DockSpace(ctx, id, bounds)` + `DockableWindow(ctx, id)` | Docking system |

## Layout Helpers

```cpp
HStack(ctx, {.gap = 10}) {
    Button(ctx, "Save");
    Spacer(ctx);
    Button(ctx, "Cancel");
}
```

Low-level layout helpers:

- `BeginHorizontal(ctx)` / `EndHorizontal(ctx)`
- `BeginVertical(ctx)` / `EndVertical(ctx)`
- `Spacing(ctx, px)`, `Padding(ctx, px)`

## Drag and Drop

```cpp
// Source
if (fst::BeginDragDropSource(ctx)) {
    int data = 123;
    fst::SetDragDropPayload("ITEM", &data, sizeof(int));
    fst::SetDragDropDisplayText("Item 123");
    fst::EndDragDropSource(ctx);
}

// Target
if (fst::BeginDragDropTarget(ctx)) {
    if (const auto* payload = fst::AcceptDragDropPayload(ctx, "ITEM")) {
        int value = payload->getData<int>();
    }
    fst::EndDragDropTarget(ctx);
}
```

## Common Mistakes to Avoid

1. Missing theme: call `ctx.setTheme()` or widgets can appear invisible.
2. Missing font: load at least one font before text widgets.
3. Skipping `window.pollEvents()` causes the window to hang.
4. Calling draw commands after `ctx.endFrame()` has no effect.

## Styling Hook

```cpp
fst::Style style = fst::Style::flexible(1.0f).withPadding(8.0f);
```
