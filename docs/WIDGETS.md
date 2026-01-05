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

---
[Next: Theming Guide →](THEMING.md)
