# Fastener Documentation

Welcome to the official documentation for Fastener, a high-performance C++ GUI library designed for polished desktop applications.

Fastener combines the simple API of immediate-mode GUIs with the polish and performance of retained-mode frameworks.

## Navigation

- Getting Started: `GETTING_STARTED.md`
- Architecture: `ARCHITECTURE.md`
- API Reference: `API.md`
- Widget Catalog: `WIDGETS.md`
- Theming Guide: `THEMING.md`
- AI Context: `AI_CONTEXT.md`

## Key Features

- Performance-first batched OpenGL 3.3 rendering
- Explicit Context API for predictable widget state
- Dark and light themes with per-widget Style overrides
- Docking, drag and drop, menu system, and command palette
- Windows and Linux (X11) platform backends

## Project Structure

```text
Fastener/
|-- include/fastener/       # Public headers
|   |-- core/               # Fundamental types and Context
|   |-- graphics/           # Low-level rendering API
|   |-- ui/                 # Styling and layout systems
|   `-- widgets/            # High-level UI components
|-- src/                    # Implementation files
|-- docs/                   # Documentation (you are here)
`-- examples/               # Demo applications
```

## Basic Usage

```cpp
#include "fastener/fastener.h"

int main() {
    fst::Window window("Fastener App");
    fst::Context ctx;
    ctx.loadFont("path/to/font.ttf", 16.0f);
    ctx.setTheme(fst::Theme::dark());

    while (window.isOpen()) {
        window.pollEvents();
        ctx.beginFrame(window);

        if (fst::Button(ctx, "Click Me")) {
            // Handle click
        }

        ctx.endFrame();
        window.swapBuffers();
    }
    return 0;
}
```

---
Next: GETTING_STARTED.md
