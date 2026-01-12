# Fastener

**High-performance C++ GUI library for building desktop applications.**

Fastener is designed for creating polished end-user applications like IDEs, file explorers, and productivity tools. It combines the polish of traditional retained-mode GUIs with the simplicity of immediate-mode rendering.

![License](https://img.shields.io/badge/license-MIT-blue.svg)
![C++](https://img.shields.io/badge/C%2B%2B-17-blue.svg)
![Platform](https://img.shields.io/badge/platform-Windows-lightgrey.svg)

## Features

- 🚀 **High Performance** - Batched OpenGL rendering, minimal draw calls
- 🎨 **Modern UI** - Flat design with dark/light themes
- 📦 **Minimal Dependencies** - Only stb_truetype and stb_image (bundled)
- 🔧 **Easy to Use** - Simple, intuitive API
- 🎯 **Focused** - Built for real applications, not just debugging

### Widgets

| Widget | Description |
|--------|-------------|
| `Button` | Standard and primary buttons |
| `Label` | Text display with styling |
| `Panel` | Container with optional title |
| `TextInput` | Single-line text input |
| `Checkbox` | Boolean toggle |
| `Slider` | Numeric value slider |
| `TreeView` | Hierarchical list (file explorer) |
| `TabControl` | Tabbed interface |

## Quick Start

```cpp
#include "fastener/fastener.h"

int main() {
    fst::Window window("My App", 1280, 720);
    fst::Context ctx;
    ctx.setTheme(fst::Theme::dark());
    ctx.loadFont("C:/Windows/Fonts/arial.ttf", 14.0f);
    
    while (window.isOpen()) {
        window.pollEvents();
        ctx.beginFrame(window);
        
        // Your UI code here
        fst::DrawList& dl = ctx.drawList();
        dl.addText(ctx.font(), {100, 100}, "Hello, Fastener!", 
                   ctx.theme().colors.text);
        
        ctx.endFrame();
        window.swapBuffers();
    }
    return 0;
}
```

## Building

### Requirements

- CMake 3.16+
- C++17 compiler (MSVC 2019+, GCC 9+, Clang 10+)
- OpenGL 3.3+

### Build Commands

```bash
# Configure
cmake -B build -G "Visual Studio 17 2022"

# Build
cmake --build build --config Release

# Run demo
./build/Release/fastener_demo.exe
```

### Using in Your Project

```cmake
add_subdirectory(path/to/Fastener)
target_link_libraries(your_app PRIVATE fastener)
```

## Project Structure

```
Fastener/
├── include/fastener/
│   ├── fastener.h          # Main header (include this)
│   ├── core/               # Types, Context, Input
│   ├── platform/           # Window abstraction
│   ├── graphics/           # Renderer, Font, Texture, DrawList
│   ├── ui/                 # Theme, Style, Layout
│   └── widgets/            # Button, TreeView, TabControl, etc.
├── src/                    # Implementation
├── thirdparty/             # stb_truetype.h, stb_image.h
├── examples/demo/          # Demo application
└── CMakeLists.txt
```

## Theming

```cpp
// Use built-in themes
ctx.setTheme(fst::Theme::dark());
ctx.setTheme(fst::Theme::light());

// Or customize
fst::Theme theme = fst::Theme::dark();
theme.colors.primary = fst::Color::fromHex(0x6366f1);  // Indigo
theme.metrics.borderRadius = 8.0f;
ctx.setTheme(theme);
```

## TreeView Example

```cpp
fst::TreeView tree;
auto root = tree.root();

auto src = root->addChild("src", "src");
src->addChild("main.cpp", "main.cpp", true);  // true = leaf node

fst::TreeViewEvents events;
events.onDoubleClick = [](fst::TreeNode* node) {
    if (node->isLeaf) {
        // Open file
    }
};

tree.render("explorer", bounds, {}, events);
```

## TabControl Example

```cpp
fst::TabControl tabs;
tabs.addTab("file1.cpp", "file1.cpp");
tabs.addTab("file2.cpp", "file2.cpp");

fst::TabControlEvents events;
events.onClose = [&](int index, const fst::TabItem& tab) {
    tabs.removeTab(index);
};

fst::Rect contentArea = tabs.render("tabs", bounds, {}, events);
// Draw content in contentArea
```

## Roadmap

- [x] Core rendering (OpenGL 3.3)
- [x] Font rendering (stb_truetype)
- [x] Basic widgets (Button, Label, TextInput, Checkbox, Slider, etc.)
- [x] Advanced widgets (RadioButton, ColorPicker, Spinner, Table, etc.)
- [x] TreeView
- [x] TabControl
- [x] Drag and Drop system (including cross-window)
- [x] Docking system (DockSpace, DockableWindow)
- [ ] TextEditor (syntax highlighting)
- [ ] Menu & ContextMenu (in progress)
- [ ] Cross-platform (Linux, macOS)

## 📚 Documentation

Comprehensive documentation is available in the [**docs/**](docs/README.md) folder:

- [**Quick Start Guide**](docs/GETTING_STARTED.md)
- [**Architecture Overview**](docs/ARCHITECTURE.md)
- [**API Reference**](docs/API.md)
- [**Widget Catalog**](docs/WIDGETS.md)
- [**Theming Guide**](docs/THEMING.md)
- [**AI Context**](docs/AI_CONTEXT.md) (for use with AI assistants)

## License

---

**Fastener** - Build beautiful desktop applications in C++.
