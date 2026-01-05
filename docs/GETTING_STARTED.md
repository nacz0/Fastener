# Getting Started with Fastener

This guide will help you set up your environment and build your first application using Fastener.

## 📋 Prerequisites

Before you begin, ensure you have the following installed:

- **C++17 Compiler** (MSVC 2019+, GCC 9+, or Clang 10+)
- **CMake 3.16** or newer
- **OpenGL 3.3** compatible graphics drivers

Fastener currently supports **Windows** out of the box.

## ⚙️ Building the Library

Fastener is designed to be easily integrated into CMake projects.

### 1. Clone the repository
```bash
git clone https://github.com/nacz0/Fastener.git
```

### 2. Configure and Build
```bash
cd Fastener
cmake -B build
cmake --build build --config Release
```

The library will be built as a static library (`fastener.lib` on Windows).

## 🔨 Integration with Your Project

The most common way to use Fastener is via `add_subdirectory` in your `CMakeLists.txt`:

```cmake
# Your project's CMakeLists.txt
add_subdirectory(path/to/Fastener)

add_executable(my_app main.cpp)
target_link_libraries(my_app PRIVATE fastener)
```

## 🏗 Minimal Working Example

Creating a basic window with a button and a label:

```cpp
#include "fastener/fastener.h"

int main() {
    // 1. Configure and create a window
    fst::WindowConfig config;
    config.title = "My First Fastener App";
    config.width = 1280;
    config.height = 720;
    
    fst::Window window(config);
    
    // 2. Initialize the GUI context
    fst::Context ctx;
    ctx.setTheme(fst::Theme::dark());
    
    // 3. Load a font (Required for text rendering)
    // Note: Use an absolute path or ensure it's in your working directory
    ctx.loadFont("C:/Windows/Fonts/segoeui.ttf", 16.0f);
    
    // 4. Application Loop
    while (window.isOpen()) {
        // Poll OS events (keyboard, mouse, resize)
        window.pollEvents();
        
        // Start a new UI frame
        ctx.beginFrame(window);
        
        // --- DRAW YOUR UI HERE ---
        fst::Label("Welcome to Fastener!");
        
        if (fst::Button("Interactive Button")) {
            // This block executes if the button was clicked this frame
        }
        
        // End the frame (renders everything collected)
        ctx.endFrame();
        
        // Swap front/back buffers
        window.swapBuffers();
    }
    
    return 0;
}
```

## 💡 Important Concepts

- **Coordinates**: Fastener uses a screenspace coordinate system (0,0 is top-left).
- **Theme**: You must set a theme or colors explicitly for widgets to look correct.
- **Fonts**: At least one font must be loaded before rendering any text-based widgets.
- **Polling**: Always call `window.pollEvents()` at the start of your loop to keep the UI interactive.

---
[Next: Architecture Overview →](ARCHITECTURE.md)
