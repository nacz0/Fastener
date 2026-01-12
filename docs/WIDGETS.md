# Widget Catalog

Fastener provides a rich set of built-in widgets for building complex desktop applications.

## 🔘 Basic Widgets

### `Button`
Standard clickable button.
```cpp
// Simple
if (fst::Button("Save")) { /* click action */ }

// Primary (Accent color)
if (fst::ButtonPrimary("Submit")) { ... }

// Options
fst::ButtonOptions opts;
opts.disabled = true;
fst::Button("Ghosted", opts);
```

### `Label`
Static text display.
```cpp
fst::Label("Standard Text");
fst::LabelSecondary("Less prominent text");
fst::LabelHeading("Section Header");

// With wrapping
fst::Label("A very long text...", { .wrap = true });
```

### `Checkbox`
Toggle switch for boolean values.
```cpp
bool isEnabled = true;
if (fst::Checkbox("Enable Sound", isEnabled)) {
    // Value changed
}
```

### `Slider`
Numeric value adjustment.
```cpp
float volume = 0.5f;
fst::Slider("Volume", volume, 0.0f, 1.0f);

int count = 10;
fst::SliderInt("Items", count, 0, 100);
```

### `TextInput`
Single-line text entry.
```cpp
std::string username;
fst::TextInput("user_id", username, { .placeholder = "Username..." });

// With label prepended
fst::TextInputWithLabel("Email", email);
```

---

## 🏗 Containers & Layout

### `Panel`
A container with an optional background and title.
```cpp
if (Panel("MainPanel", { .title = "Settings" })) {
    fst::Button("Action 1");
    fst::Button("Action 2");
}
```

### `ScrollArea`
A region for content that exceeds its bounds.
```cpp
fst::ScrollArea sa;
sa.setContentSize({ 1000, 2000 });
sa.render("my_scroll", bounds, [&](const fst::Rect& viewport) {
    // Draw content here using viewport as clip boundaries
});
```

### `Splitter`
Interactive divider for resizable layouts.
```cpp
static float sidebarWidth = 200.0f;
fst::Splitter("layout_split", sidebarWidth, totalBounds, { .direction = fst::Direction::Vertical });
```

---

## 🗂 Advanced Widgets

### `TabControl`
Tabbed interface for multiple documents or views.
```cpp
fst::TabControl tabs;
tabs.addTab("main.cpp", "main.cpp");

fst::TabControlEvents events;
events.onClose = [&](int idx, const fst::TabItem& tab) { tabs.removeTab(idx); };

fst::Rect contentArea = tabs.render("editor_tabs", bounds, {}, events);
```

### `TreeView`
Hierarchical data display (e.g., file explorer).
```cpp
fst::TreeView tree;
auto root = tree.root();
root->addChild("src", "src")->addChild("main.cpp", "main.cpp", true);

tree.render("explorer", bounds);
```

### `TextEditor`
Highly optimized multi-line code editor.
```cpp
fst::TextEditor editor;
editor.setText("void main() { ... }");
editor.render(bounds);
```

---

## 🛠 Status & Overlays

### `ProgressBar`
Visual progress indicator.
```cpp
fst::ProgressBar(0.75f); // 75%

// Indeterminate (Animated)
fst::ProgressBar("Loading...", 0.0f, { .indeterminate = true });
```

### `ComboBox`
Dropdown selection menu.
```cpp
int selection = 0;
std::vector<std::string> options = { "Option A", "Option B", "Option C" };
fst::ComboBox("Settings", selection, options);
```

### `Tooltip`
Floating information shown on hover.
```cpp
fst::Button("Delete");
fst::Tooltip("Permanently remove this file"); // Attaches to previous widget
```

### `Menu`
Horizontal menu bar or popup context menus.
```cpp
// Menu Bar
fst::MenuBar menu;
menu.addMenu("File", { fst::MenuItem("exit", "Exit") });
menu.render({ 0, 0, width, 28 });

// Context Menu (Right-click)
fst::ShowContextMenu({ fst::MenuItem("copy", "Copy") }, mousePos);
```

### `RadioButton`
Select one option from a group.
```cpp
static int selectedOption = 0;
if (fst::RadioButton("Option A", selectedOption == 0)) selectedOption = 0;
if (fst::RadioButton("Option B", selectedOption == 1)) selectedOption = 1;
if (fst::RadioButton("Option C", selectedOption == 2)) selectedOption = 2;
```

### `InputNumber`
Numeric input field with spin controls.
```cpp
float value = 10.0f;
fst::InputNumber("value_id", value, { .min = 0.0f, .max = 100.0f });

int intValue = 5;
fst::InputNumberInt("int_id", intValue, { .step = 1 });
```

### `Selectable`
Single-line selectable item (for lists).
```cpp
bool selected = false;
if (fst::Selectable("Item 1", selected)) {
    // Clicked
}

// Double-click detection
if (fst::Selectable("Item 2", selected, { .allowDoubleClick = true })) {
    if (/* isDoubleClick */) { /* open item */ }
}
```

### `Listbox`
Scrollable list of items with selection.
```cpp
std::vector<std::string> items = { "Item 1", "Item 2", "Item 3" };
static int selectedIdx = 0;
fst::Listbox("my_list", selectedIdx, items, bounds);
```

### `ColorPicker`
Interactive color selection widget.
```cpp
fst::Color color = fst::Color::fromHex(0xFF5733);
if (fst::ColorPicker("color_id", color)) {
    // Color changed
}
```

### `Spinner`
Loading/activity indicator (animated).
```cpp
fst::Spinner({ .size = 24.0f }); // Default animation
```

### `Table`
Multi-column data table with headers.
```cpp
fst::Table table;
table.addColumn("Name", 200.0f);
table.addColumn("Value", 100.0f);

table.beginRow();
table.addCell("Item 1");
table.addCell("100");
table.endRow();

table.render("data_table", bounds);
```

### `TextArea`
Multi-line text input.
```cpp
std::string text = "Line 1\nLine 2\nLine 3";
if (fst::TextArea("notes_id", text, bounds)) {
    // Text changed
}
```

### `Image`
Display a texture or image.
```cpp
fst::Texture* texture = /* load texture */;
fst::Image(texture, { 200, 150 }); // width, height
```

### `Separator`
Horizontal or vertical dividing line.
```cpp
fst::Separator(); // Horizontal
fst::Separator({ .vertical = true, .height = 100.0f });
```

### `CollapsingHeader`
Collapsible section header.
```cpp
static bool open = false;
if (fst::CollapsingHeader("Advanced Settings", open)) {
    fst::Label("Content inside...");
}
```

---

## 🎯 Drag and Drop

Enable data transfer between widgets and windows.

### Basic Usage

**Drag Source:**
```cpp
fst::Button("Draggable Item");
if (fst::BeginDragDropSource()) {
    int data = 123;
    fst::SetDragDropPayload("ITEM_ID", &data, sizeof(int));
    fst::SetDragDropDisplayText("Item 123");
    fst::EndDragDropSource();
}
```

**Drop Target:**
```cpp
fst::Panel("drop_zone");
if (fst::BeginDragDropTarget()) {
    if (const auto* payload = fst::AcceptDragDropPayload("ITEM_ID")) {
        int data = payload->getData<int>();
        // Handle drop
    }
    fst::EndDragDropTarget();
}
```

### Cross-Window Drag
```cpp
// Enable cross-window flag
if (fst::BeginDragDropSource(fst::DragDropFlags_CrossWindow)) {
    // Payload setup...
    fst::EndDragDropSource();
}
```

### Custom Drop Zones
```cpp
// Explicit bounds for drop target
fst::Rect dropZone = { 100, 100, 200, 50 };
if (fst::BeginDragDropTarget(dropZone)) {
    // Accept payload...
    fst::EndDragDropTarget();
}
```

---

## 🪟 Docking System

Create IDE-style dockable layouts.

### `DockSpace`
Main docking container.
```cpp
fst::DockSpace dockSpace;
dockSpace.render("main_dock", windowBounds);
```

### `DockableWindow`
Window that can be docked.
```cpp
fst::DockableWindow window;
window.setTitle("Tool Panel");
if (window.render("tool_1", dockSpace)) {
    fst::Label("Docked content");
}
```

### `DockPreview`
Visual feedback during docking operations.
```cpp
// Automatically handled by DockSpace
// Shows drop zones when dragging windows
```

---

[Next: Theming Guide →](THEMING.md)
