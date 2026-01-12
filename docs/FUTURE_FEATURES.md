# Future Features

A collection of potential enhancements and new features for the Fastener library.

---

## 🚀 High Priority (From Roadmap)

These are already on the roadmap and would significantly enhance the library:

1. **TextEditor with Syntax Highlighting** - Already in progress, add language-specific highlighting (C++, Python, JSON, etc.)
2. **Complete Menu & ContextMenu system** - Full implementation with shortcuts, submenus, and icons
3. **Cross-platform support** - Linux and macOS support (currently Windows-only)

---

## 🧩 New Widgets

| Widget | Description |
|--------|-------------|
| **DatePicker / TimePicker** | Calendar-based date selection and time input |
| **Breadcrumb** | Navigation path display (e.g., `Home > Folder > Subfolder`) |
| **TagInput** | Multi-value input with removable chips/badges |
| **SearchBar** | Integrated search with history, suggestions, and clear button |
| **Stepper / Wizard** | Multi-step form with progress indicator |
| **Accordion** | Multiple collapsible sections (only one open at a time) |
| **Toolbar** | Horizontal button/icon group with separators |
| **StatusBar** | Bottom bar with multiple sections for status info |
| **Notification / Toast** | Non-blocking alerts that auto-dismiss |
| **Modal / Dialog** | Centered popup with backdrop |
| **Card** | Content container with shadow, image, and actions |
| **Badge** | Small count indicator (e.g., notification count) |
| **Toggle Switch** | iOS-style boolean switch (alternative to checkbox) |
| **RangeSlider** | Two-handle slider for min/max range selection |
| **RichTextPreview** | Markdown/HTML preview renderer |

---

## 🎨 Theming & Styling

1. **Theme Presets** - Additional built-in themes (Nord, Solarized, Dracula, etc.)
2. **Theme Import/Export** - JSON-based theme serialization
3. **Icon System** - Built-in icon font or SVG support with `Icon("save")`
4. **Animation Framework** - Tweening system for smooth widget transitions
5. **Accessibility Options** - High contrast mode, larger fonts, reduced motion

---

## ⚙️ Core Enhancements

1. **Layout System** - Flexbox-like automatic layout (`HStack`, `VStack`, `Grid`)
2. **Constraint-based Sizing** - `minWidth`, `maxHeight`, percentage widths
3. **Focus Management** - Keyboard navigation (Tab/Shift+Tab between widgets)
4. **Clipboard Integration** - Copy/paste support across the system
5. **Undo/Redo Framework** - Command pattern for undoable actions
6. **Localization Support** - Translation strings with `i18n()` helper
7. **Custom Cursors** - User-defined cursor images
8. **Hot-reload** - Runtime theme/font changes without restart

---

## 📊 Data Widgets

1. **DataGrid** - Enhanced Table with sorting, filtering, pagination
2. **Chart Widget** - Line, bar, pie charts (basic charting)
3. **PropertyGrid** - Key-value editor for settings (like Unity Inspector)
4. **VirtualList** - Efficient rendering for 10,000+ items

---

## 🖼️ Graphics & Effects

1. **Blur/Frosted Glass** - Background blur for panels
2. **Shadows & Depth** - Elevation system for layered UI
3. **SVG Rendering** - Vector graphics support
4. **Image Cropper** - Interactive image crop tool
5. **Canvas Widget** - Custom drawing area with user callbacks

---

## 🛠️ Developer Experience

1. **Debug Overlay** - Show widget bounds, IDs, render stats
2. **Performance Profiler** - Frame time breakdown per widget
3. **Hot Key Manager** - Global shortcut registration system
4. **Command Palette** - VSCode-style Ctrl+Shift+P command search
5. **Unit Testing Helpers** - Simulated input for widget testing
