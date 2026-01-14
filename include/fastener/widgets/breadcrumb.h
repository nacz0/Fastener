#pragma once

#include "fastener/core/types.h"
#include "fastener/ui/style.h"
#include <string>
#include <vector>

/**
 * @file breadcrumb.h
 * @brief Navigation path display with clickable segments
 * 
 * @ai_hint Breadcrumb displays a navigation path like "Home > Folder > File".
 *          Returns the index of the clicked segment, or -1 if none clicked.
 *          The last item is the current location and is not clickable.
 * 
 * @example
 *   std::vector<std::string> path = {"Home", "Documents", "Project"};
 *   int clicked = fst::Breadcrumb(ctx, path);
 *   if (clicked >= 0) {
 *       // Navigate to path[clicked]
 *       navigateTo(path[clicked]);
 *   }
 */

namespace fst {

class Context;

//=============================================================================
// Breadcrumb
//=============================================================================
struct BreadcrumbOptions {
    Style style;
    std::string separator = ">";  // Separator between items
};

/// Display breadcrumb navigation - returns clicked index or -1
[[nodiscard]] int Breadcrumb(Context& ctx, const std::vector<std::string>& items, const BreadcrumbOptions& options = {});

} // namespace fst
