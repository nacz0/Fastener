#pragma once

// Fastener - High-Performance C++ GUI Library
// Main header - includes all public API

#include "fastener/core/types.h"
#include "fastener/core/context.h"
#include "fastener/core/input.h"

#include "fastener/platform/window.h"

#include "fastener/graphics/renderer.h"
#include "fastener/graphics/draw_list.h"
#include "fastener/graphics/font.h"
#include "fastener/graphics/texture.h"

#include "fastener/ui/widget.h"
#include "fastener/ui/layout.h"
#include "fastener/ui/theme.h"
#include "fastener/ui/style.h"

#include "fastener/widgets/button.h"
#include "fastener/widgets/label.h"
#include "fastener/widgets/panel.h"
#include "fastener/widgets/splitter.h"
#include "fastener/widgets/text_input.h"
#include "fastener/widgets/checkbox.h"
#include "fastener/widgets/slider.h"
#include "fastener/widgets/color_picker.h"
#include "fastener/widgets/tree_view.h"
#include "fastener/widgets/tab_control.h"
#include "fastener/widgets/menu.h"
#include "fastener/widgets/scroll_area.h"
#include "fastener/widgets/combo_box.h"
#include "fastener/widgets/progress_bar.h"
#include "fastener/widgets/tooltip.h"
#include "fastener/widgets/text_editor.h"
#include "fastener/widgets/listbox.h"
#include "fastener/widgets/spinner.h"
#include "fastener/widgets/selectable.h"
#include "fastener/widgets/text_area.h"
#include "fastener/widgets/radio_button.h"
#include "fastener/widgets/input_number.h"
#include "fastener/widgets/collapsing_header.h"
#include "fastener/widgets/image.h"
#include "fastener/widgets/separator.h"
#include "fastener/widgets/table.h"

// Docking
#include "fastener/ui/dock_node.h"
#include "fastener/ui/dock_context.h"
#include "fastener/ui/dock_builder.h"
#include "fastener/widgets/dock_space.h"
#include "fastener/widgets/dockable_window.h"
#include "fastener/widgets/dock_preview.h"

namespace fst {
    // Version info
    constexpr int VERSION_MAJOR = 0;
    constexpr int VERSION_MINOR = 1;
    constexpr int VERSION_PATCH = 0;
    
    constexpr const char* VERSION_STRING = "0.1.0";
}
