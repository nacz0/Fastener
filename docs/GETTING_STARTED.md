# Getting Started with Fastener

This guide helps you set up and build your first Fastener application.

## Prerequisites

- C++17 compiler (MSVC 2019+, GCC 9+, or Clang 10+)
- CMake 3.16 or newer
- OpenGL 3.3 compatible graphics drivers

Supported platforms: Windows and Linux (X11).

## Building the Library

Fastener integrates cleanly into CMake projects.

### 1. Clone the repository

```bash
git clone https://github.com/nacz0/Fastener.git
```

### 2. Configure and build

```bash
cd Fastener
cmake -B build
cmake --build build --config Release
```

The library builds as a static library (`fastener.lib` on Windows).

## Integration with Your Project

```cmake
# Your project's CMakeLists.txt
add_subdirectory(path/to/Fastener)

add_executable(my_app main.cpp)
target_link_libraries(my_app PRIVATE fastener)
```

## Minimal Working Example

```cpp
#include "fastener/fastener.h"

int main() {
    // 1. Create a window
    fst::WindowConfig config;
    config.title = "My First Fastener App";
    config.width = 1280;
    config.height = 720;

    fst::Window window(config);

    // 2. Initialize the GUI context
    fst::Context ctx;
    ctx.setTheme(fst::Theme::dark());

    // 3. Load a font (required for text rendering)
    ctx.loadFont("C:/Windows/Fonts/segoeui.ttf", 16.0f);

    // 4. Application loop
    while (window.isOpen()) {
        window.pollEvents();
        ctx.beginFrame(window);

        fst::Label(ctx, "Welcome to Fastener!");
        if (fst::Button(ctx, "Interactive Button")) {
            // This runs only on the click frame
        }

        ctx.endFrame();
        window.swapBuffers();
    }

    return 0;
}
```

## Important Concepts

- Coordinates: screen space with (0,0) at top-left.
- Theme: set a theme or configure colors explicitly.
- Fonts: at least one font must be loaded before text widgets.
- Event loop: always call `window.pollEvents()` before `beginFrame()`.
- Frame scope: `ctx.input()` and `ctx.window()` are only valid between `beginFrame()` and `endFrame()`.
- GL context: renderer initialization and GL resource cleanup require a current context; keep a window/context alive when destroying Fastener resources.

---
Next: ARCHITECTURE.md
