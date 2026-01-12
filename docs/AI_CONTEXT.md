# Fastener Library AI Context

This file provides a structured summary of the Fastener library for AI assistants. Use this to generate correct C++ code using the library.

## 📋 Library Overview
- **Name**: Fastener (fst)
- **Type**: Immediate-Mode GUI (custom batching engine)
- **Language**: C++17
- **Coordinates**: Screen-space (0,0 = top-left)
- **Namespace**: `fst`
- **Main Header**: `#include "fastener/fastener.h"`

## 📐 Core Data Types
- `fst::Vec2(x, y)`
- `fst::Rect(x, y, w, h)` or `fst::Rect(pos, size)`
- `fst::Color(r, g, b, a)` or `fst::Color::fromHex(0xRRGGBB)`
- `fst::WidgetId` (uint64_t)

## 🏗 Lifecycle Patterns
```cpp
fst::Window window("Title", 1280, 720);
fst::Context ctx;
ctx.loadFont("path.ttf", 14.0f);
ctx.setTheme(fst::Theme::dark());

while(window.isOpen()) {
    window.pollEvents();
    ctx.beginFrame(window);
    // UI Calls
    ctx.endFrame();
    window.swapBuffers();
}
```

## 🛠 Widget API Quick Reference

| Widget | Signature | Notes |
|--------|-----------|-------|
| **Button** | `bool Button(label, options={})` | Returns true on click |
| **Label** | `void Label(text, options={})` | Includes `LabelHeading`, `LabelSecondary` |
| **TextInput** | `bool TextInput(id, string& val, options={})` | Returns true when changed |
| **Checkbox** | `bool Checkbox(label, bool& val, options={})` | Returns true when toggled |
| **RadioButton** | `bool RadioButton(label, bool selected, opts={})` | For option groups |
| **Slider** | `bool Slider(label, float& val, min, max, opts)` | `SliderInt` for integers |
| **InputNumber** | `bool InputNumber(id, float& val, opts={})` | Spinbox-style numeric input |
| **Panel** | `Panel(id, options)` (Scope macro) | Container: `if(Panel("id")){...}` |
| **TreeView** | `tree.render(id, bounds, options, events)` | Uses `fst::TreeNode` structure |
| **TabControl**| `tabs.render(id, bounds, options, events)` | Returns `fst::Rect` for content area |
| **Splitter** | `bool Splitter(id, float& pos, bounds, opts)` | `pos` is X (Vert) or Y (Horiz) |
| **ComboBox** | `bool ComboBox(label, int& idx, vector<string>)`| Dropdown selection |
| **Listbox** | `bool Listbox(id, int& idx, vector<string>, bounds)` | Scrollable list |
| **Selectable** | `bool Selectable(label, bool selected, opts={})` | List item with selection |
| **ProgressBar**| `void ProgressBar(val, options)` | `val` is 0.0 to 1.0 |
| **Spinner** | `void Spinner(options={})` | Animated loading indicator |
| **Tooltip** | `void Tooltip(text, options={})` | Call AFTER the widget to attach |
| **ColorPicker** | `bool ColorPicker(id, Color& col, opts={})` | Interactive color selection |
| **Table** | `table.render(id, bounds, opts)` | Multi-column data display |
| **TextArea** | `bool TextArea(id, string& text, bounds, opts)` | Multi-line text input |
| **TextEditor** | `editor.render(bounds, options)` | Multi-line code editor |
| **Image** | `void Image(Texture*, size, opts={})` | Display texture/image |
| **Separator** | `void Separator(options={})` | Horizontal/vertical divider |
| **CollapsingHeader** | `bool CollapsingHeader(label, bool& open, opts)` | Collapsible section |
| **Menu** | `menuBar.render(bounds)` | Popups: `menuBar.renderPopups()` at frame end |
| **Layout** | `BeginHorizontal()`, `BeginVertical()`, `Spacing(v)`, `Padding(v)` | Automatic positioning |

## 💡 Code Patterns & Idioms

### Layout with Rects
```cpp
fst::Rect bounds = {10, 10, 200, 30};
if (fst::Button("Action", { .style = fst::Style().withPos(bounds.x, bounds.y) })) { ... }
```

### Conditional Rendering
```cpp
static bool showSettings = false;
fst::Checkbox("Settings", showSettings);
if (showSettings) {
    if (Panel("SettingsPanel")) { ... }
}
```

### Hierarchy (ID Stack)
```cpp
for (int i=0; i<10; ++i) {
    ctx.pushId(i);
    if (fst::Button("Item")) { ... } // Each has unique ID
    ctx.popId();
}
```

### Automatic Layout
```cpp
fst::BeginVertical();
    fst::Label("Settings");
    fst::BeginHorizontal();
        fst::Button("Save");
        fst::Spacing(10);
        fst::Button("Cancel");
    fst::EndHorizontal();
fst::EndVertical();
```

### Drag and Drop
```cpp
// SOURCE: After rendering the draggable widget
fst::Button("Draggable Item");
if (fst::BeginDragDropSource()) {
    int itemData = 42;
    fst::SetDragDropPayload("MY_TYPE", &itemData, sizeof(int));
    fst::SetDragDropDisplayText("Item 42");
    fst::EndDragDropSource();
}

// TARGET: After rendering the drop target widget
fst::Panel("drop_area");
if (fst::BeginDragDropTarget()) {
    if (const auto* payload = fst::AcceptDragDropPayload("MY_TYPE")) {
        int droppedData = payload->getData<int>();
        // Handle drop
    }
    fst::EndDragDropTarget();
}

// Cross-window drag
if (fst::BeginDragDropSource(fst::DragDropFlags_CrossWindow)) {
    // Setup payload...
    fst::EndDragDropSource();
}
```

## ⚠️ Common Mistakes to Avoid
1.  **Missing Theme**: Always call `ctx.setTheme()` or widgets look invisible/wrong.
2.  **Missing Font**: `ctx.loadFont()` is required for any text.
3.  **Polling**: Forgetting `window.pollEvents()` causes window to hang.
4.  **EndFrame Rendering**: Custom `dl.add...` calls made AFTER `ctx.endFrame()` won't be visible.
5.  **Coordinate Systems**: Ensure `bounds` passed to widgets are in absolute screen coordinates, not local to parents.

## 🎨 Styling Hook
```cpp
// Generic flexible style
fst::Style style = fst::Style::flexible(1.0f).withPadding(8.0f);
```
