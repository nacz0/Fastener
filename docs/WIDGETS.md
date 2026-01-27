# Widget Catalog

Fastener ships with a growing set of built-in widgets. All widget functions use an explicit `Context& ctx` parameter. Some widgets are class-based and require an instance.

## Basic Inputs

### Button
```cpp
if (fst::Button(ctx, "Save")) { /* click */ }
if (fst::ButtonPrimary(ctx, "Submit")) { /* click */ }
```

### Label
```cpp
fst::Label(ctx, "Standard Text");
fst::LabelSecondary(ctx, "Muted");
fst::LabelHeading(ctx, "Section Header");
```

### TextInput
```cpp
std::string username;
fst::TextInput(ctx, "user_id", username, { .placeholder = "Username" });
fst::TextInputWithLabel(ctx, "Email", username);
```

### TextArea
```cpp
std::string notes = "Line 1\nLine 2";
fst::TextArea(ctx, "notes", notes, { .height = 120.0f });
```

### Checkbox and ToggleSwitch
```cpp
bool enabled = true;
fst::Checkbox(ctx, "Enable Sound", enabled);

bool darkMode = false;
fst::ToggleSwitch(ctx, "Dark Mode", darkMode);
```

### RadioButton
```cpp
int selected = 0;
fst::RadioButton(ctx, "Option A", selected, 0);
fst::RadioButton(ctx, "Option B", selected, 1);
```

### Slider and InputNumber
```cpp
float volume = 0.5f;
fst::Slider(ctx, "Volume", volume, 0.0f, 1.0f);

int count = 10;
fst::InputNumberInt(ctx, "Count", count, 0, 100);
```

## Containers and Layout

### Panel
```cpp
Panel(ctx, "settings", { .title = "Settings" }) {
    fst::Checkbox(ctx, "Dark Mode", darkMode);
}
```

### Card
```cpp
Card(ctx, "profile", { .title = "Profile" }) {
    fst::Label(ctx, "John Doe");
}
```

### ScrollArea
```cpp
fst::ScrollArea sa;
sa.setContentSize({1000, 2000});
sa.render(ctx, "scroll", bounds, [&](const fst::Rect& viewport) {
    // render content within viewport
});
```

### Splitter
```cpp
float sidebarWidth = 240.0f;
fst::Splitter(ctx, "layout_split", sidebarWidth, bounds, { .direction = fst::Direction::Vertical });
```

### Flex Layout (HStack, VStack, Grid)
```cpp
HStack(ctx, {.gap = 8}) {
    fst::Button(ctx, "A");
    fst::Button(ctx, "B");
    fst::Spacer(ctx);
    fst::Button(ctx, "C");
}
```

### CollapsingHeader and Separator
```cpp
bool open = true;
if (fst::CollapsingHeader(ctx, "Advanced", open)) {
    fst::Separator(ctx);
}
```

## Navigation and Selection

### TabControl
```cpp
fst::TabControl tabs;
tabs.addTab("main.cpp", "main.cpp");

fst::Rect content = tabs.render(ctx, "editor_tabs", bounds);
```

### TreeView
```cpp
fst::TreeView tree;
auto root = tree.root();
root->addChild("src", "src")->addChild("main.cpp", "main.cpp", true);

tree.render(ctx, "explorer", bounds);
```

### Breadcrumb
```cpp
std::vector<std::string> path = {"Home", "Docs", "Fastener"};
int clicked = fst::Breadcrumb(ctx, path);
```

### Listbox and Selectable
```cpp
std::vector<std::string> items = {"One", "Two", "Three"};
int selectedIdx = 0;
fst::Listbox(ctx, "items", selectedIdx, items);

bool selected = false;
fst::Selectable(ctx, "Item", selected);
```

## Data Display

### Table
```cpp
std::vector<fst::TableColumn> columns = {
    {"name", "Name", 200},
    {"size", "Size", 100}
};

fst::Table table;
table.setColumns(columns);

table.begin(ctx, "files", bounds);
table.row(ctx, {"main.cpp", "12 KB"});
table.end(ctx);
```

## Status and Feedback

### ProgressBar and Spinner
```cpp
fst::ProgressBar(ctx, 0.75f);
fst::Spinner(ctx, "loading", { .size = 24.0f });
```

### Tooltip
```cpp
fst::Button(ctx, "Delete");
fst::Tooltip(ctx, "Permanently remove this file");
```

### Badge and StatusBar
```cpp
fst::Badge(ctx, 5);
if (fst::BeginStatusBar(ctx)) {
    fst::StatusBarSection(ctx, "Ready");
    fst::EndStatusBar(ctx);
}
```

### ColorPicker
```cpp
fst::Color color = fst::Color::fromHex(0xff5733);
fst::ColorPicker(ctx, "accent", color);
```

## Media

### Image
```cpp
fst::Texture* texture = /* load */;
fst::Image(ctx, texture, {200, 150});
```

## Menus and Overlays

### MenuBar and ContextMenu
```cpp
fst::MenuBar menu;
menu.addMenu("File", { fst::MenuItem("exit", "Exit") });
menu.render(ctx, {0, 0, width, 28});
menu.renderPopups(ctx);
```

### CommandPalette
```cpp
fst::CommandPalette palette;
palette.setCommands({
    fst::CommandPaletteCommand("new", "New File", []{}).withShortcut("Ctrl+N")
});

palette.render(ctx);
```

### Modal
```cpp
bool open = true;
if (fst::BeginModal(ctx, "confirm", open, {.title = "Confirm"})) {
    fst::Label(ctx, "Are you sure?");
    if (fst::ModalButton(ctx, "OK", true)) open = false;
}
fst::EndModal(ctx);
```

## Docking

```cpp
fst::DockSpace(ctx, "main_dock", bounds);

DockableWindow(ctx, "properties", {.title = "Properties"}) {
    fst::Label(ctx, "Docked content");
}
```

## Profiling

```cpp
fst::ShowProfilerOverlay(ctx);
```

---
Next: THEMING.md
