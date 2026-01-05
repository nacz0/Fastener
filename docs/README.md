# Fastener Documentation

Welcome to the official documentation for **Fastener**, a high-performance C++ GUI library designed for building modern, polished desktop applications.

Fastener combines the simple API of immediate-mode GUIs with the polish and performance of traditional retained-mode frameworks.

## 📖 Navigation

- [**Getting Started**](GETTING_STARTED.md) - Install and build your first application in minutes.
- [**Architecture**](ARCHITECTURE.md) - Learn how Fastener works under the hood (Context, DrawList, Windows).
- [**API Reference**](API.md) - Detailed technical documentation for all core classes and functions.
- [**Widget Catalog**](WIDGETS.md) - Visual guide and usage examples for all 15+ built-in widgets.
- [**Theming Guide**](THEMING.md) - Customize colors, spacing, and styling to match your brand.
- [**AI Context**](AI_CONTEXT.md) - Specialized documentation for AI assistants (helpful for LLM code generation).

## 🚀 Key Features

*   **Performance First**: Batched OpenGL 3.3 rendering with minimal draw calls.
*   **Modern Aesthetics**: Premium dark/light themes out of the box with HSL-aware color manipulation.
*   **Ease of Use**: Procedural API that keeps UI logic where it belongs.
*   **Zero Bloat**: Minimal external dependencies (uses `stb` libraries for fonts and images).
*   **Production Ready**: Built-in Tooltips, Menus, TreeViews, and a high-performance TextEditor.

## 📦 Project Structure

```text
Fastener/
├── include/fastener/       # Public headers
│   ├── core/               # Fundamental types and Context
│   ├── graphics/           # Low-level rendering API
│   ├── ui/                 # Styling and layout systems
│   └── widgets/            # High-level UI components
├── src/                    # Implementation files
├── docs/                   # Documentation (you are here)
└── examples/               # Demo applications
```

## 🛠 Basic Usage

```cpp
#include "fastener/fastener.h"

int main() {
    fst::Window window("Fastener App");
    fst::Context ctx;
    ctx.loadFont("path/to/font.ttf", 16.0f);

    while (window.isOpen()) {
        window.pollEvents();
        ctx.beginFrame(window);

        if (fst::Button("Click Me")) {
            // Handle click
        }

        ctx.endFrame();
        window.swapBuffers();
    }
    return 0;
}
```

---
[Next: Getting Started →](GETTING_STARTED.md)
