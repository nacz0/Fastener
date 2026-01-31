#pragma once

#include "fastener/core/types.h"
#include "fastener/ui/style.h"
#include <string>
#include <string_view>
#include <vector>

namespace fst {

class Context;

//=============================================================================
// RichTextPreview
//=============================================================================

enum class RichTextFormat : uint8_t {
    Auto,
    Markdown,
    Html
};

struct RichTextPreviewOptions {
    Style style;
    RichTextFormat format = RichTextFormat::Auto;
    float width = 0.0f;
    float height = 200.0f;
    float lineSpacing = 1.2f;
    bool wordWrap = true;
    bool showBackground = true;
    bool showBorder = true;
    bool showScrollbar = true;
};

void RichTextPreview(Context& ctx, const char* id, std::string_view text,
                     const RichTextPreviewOptions& options = {});

void RichTextPreview(Context& ctx, const std::string& id, std::string_view text,
                     const RichTextPreviewOptions& options = {});

//=============================================================================
// Internal Parsing Helpers (primarily for tests)
//=============================================================================
namespace rich_text {
namespace internal {

constexpr uint8_t SpanStyleNone = 0;
constexpr uint8_t SpanStyleBold = 1 << 0;
constexpr uint8_t SpanStyleItalic = 1 << 1;
constexpr uint8_t SpanStyleCode = 1 << 2;
constexpr uint8_t SpanStyleLink = 1 << 3;
constexpr uint8_t SpanStyleHeading = 1 << 4;
constexpr uint8_t SpanStyleQuote = 1 << 5;

struct RichTextSpan {
    std::string text;
    uint8_t style = SpanStyleNone;
    std::string link;
};

struct RichTextLine {
    std::vector<RichTextSpan> spans;
    std::string prefix;
    int indent = 0;
    bool isHeading = false;
    int headingLevel = 0;
    bool isCodeBlock = false;
    bool isQuote = false;
};

std::vector<RichTextLine> parseMarkdown(std::string_view text);
std::vector<RichTextLine> parseHtml(std::string_view text);
std::vector<RichTextLine> parseRichText(std::string_view text, RichTextFormat format);

} // namespace internal
} // namespace rich_text

} // namespace fst
