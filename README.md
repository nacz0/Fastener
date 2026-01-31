# Fastener

High-performance C++ GUI library for building desktop applications.

Fastener is designed for creating polished end-user apps like IDEs, file explorers, and productivity tools. It combines the polish of retained-mode GUIs with the simplicity of immediate-mode rendering.

![License](https://img.shields.io/badge/license-MIT-blue.svg)
![C++](https://img.shields.io/badge/C%2B%2B-17-blue.svg)
![Platform](https://img.shields.io/badge/platform-Windows%20%7C%20Linux%20(X11)-lightgrey.svg)

## Features

- High performance: batched OpenGL rendering with minimal draw calls
- Modern UI: dark/light themes and flexible styling
- Minimal dependencies: stb_truetype and stb_image (bundled)
- Explicit context API for predictable state and multi-window use
- Tooling: docking, drag and drop, command palette, menus, and profiler overlay

### Widgets (highlights)

| Widget | Description |
|--------|-------------|
| `Button`, `Label`, `TextInput` | Core inputs and text |
| `DatePicker`, `TimePicker` | Date and time selection controls |
| `Checkbox`, `ToggleSwitch`, `RadioButton` | Boolean and choice controls |
| `Slider`, `InputNumber`, `ProgressBar` | Numeric and progress controls |
| `Panel`, `Card`, `ScrollArea`, `Splitter` | Containers and layout helpers |
| `TreeView`, `TabControl`, `Table` | Navigation and data display |
| `MenuBar`, `ContextMenu`, `CommandPalette` | Menus and command search |
| `DockSpace`, `DockableWindow` | Docking system |

See the full catalog in `docs/WIDGETS.md`.

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

        if (fst::Button(ctx, "Click Me")) {
            // handle click
        }

        ctx.endFrame();
        window.swapBuffers();
    }
    return 0;
}
```

Notes:
- `ctx.input()` and `ctx.window()` are valid only between `beginFrame()` and `endFrame()`.
- Renderer initialization and GL resource cleanup require a current OpenGL context.

## Building

### Requirements

- CMake 3.16+
- C++17 compiler (MSVC 2019+, GCC 9+, Clang 10+)
- OpenGL 3.3+

### Build Commands

```bash
# Configure
cmake -B build

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

## Roadmap

- [x] Core rendering (OpenGL 3.3)
- [x] Font rendering (stb_truetype)
- [x] Basic widgets (Button, Label, TextInput, Checkbox, Slider, etc.)
- [x] Advanced widgets (RadioButton, ColorPicker, Spinner, Table, etc.)
- [x] TreeView and TabControl
- [x] Drag and drop system (including cross-window)
- [x] Docking system (DockSpace, DockableWindow)
- [x] Menu bar and context menus
- [x] Command palette
- [ ] Built-in syntax highlighting presets for TextEditor
- [ ] macOS platform backend

## Documentation

Docs live in the `docs/` folder:

- `docs/GETTING_STARTED.md`
- `docs/ARCHITECTURE.md`
- `docs/API.md`
- `docs/WIDGETS.md`
- `docs/THEMING.md`
- `docs/AI_CONTEXT.md`

## License

MIT
