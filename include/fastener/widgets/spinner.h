#pragma once

#include "fastener/core/types.h"
#include "fastener/ui/style.h"
#include <string>

namespace fst {

class Context;

struct SpinnerOptions {
    Style style;
    float size = 24.0f;         ///< Diameter of the spinner
    float thickness = 3.0f;     ///< Line thickness
    float speed = 1.0f;         ///< Rotation speed multiplier
    Color color = Color(0, 0, 0, 0);  ///< 0 = use theme primary color
};

/// Explicit DI versions
void Spinner(Context& ctx, const std::string& id, const SpinnerOptions& options = {});
void SpinnerWithLabel(Context& ctx, const std::string& id, const std::string& label, 
                      const SpinnerOptions& options = {});
void LoadingDots(Context& ctx, const std::string& id, const SpinnerOptions& options = {});

/// Uses context stack
void Spinner(const std::string& id, const SpinnerOptions& options = {});
void Spinner(const char* id, const SpinnerOptions& options = {});
void SpinnerWithLabel(const std::string& id, const std::string& label, 
                      const SpinnerOptions& options = {});
void LoadingDots(const std::string& id, const SpinnerOptions& options = {});

} // namespace fst

