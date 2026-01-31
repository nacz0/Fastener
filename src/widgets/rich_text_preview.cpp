/**
 * @file rich_text_preview.cpp
 * @brief Rich text preview widget implementation (Markdown/HTML).
 */

#include "fastener/widgets/rich_text_preview.h"
#include "fastener/core/context.h"
#include "fastener/graphics/draw_list.h"
#include "fastener/graphics/font.h"
#include "fastener/ui/layout.h"
#include "fastener/ui/theme.h"
#include "fastener/ui/widget_utils.h"
#include <algorithm>
#include <cctype>
#include <sstream>
#include <unordered_map>

namespace fst {
namespace rich_text {
namespace internal {

struct RenderSpan {
    std::string text;
    uint8_t style = SpanStyleNone;
    std::string link;
};

struct RenderLine {
    std::vector<RenderSpan> spans;
    std::string prefix;
    float indent = 0.0f;
    float textOffset = 0.0f;
    float lineHeight = 0.0f;
    bool isHeading = false;
    int headingLevel = 0;
    bool isCodeBlock = false;
    bool isQuote = false;
};

struct LayoutResult {
    std::vector<RenderLine> lines;
    float contentHeight = 0.0f;
    float contentWidth = 0.0f;
};

struct RichTextPreviewState {
    float scrollOffsetY = 0.0f;
};

static std::unordered_map<WidgetId, RichTextPreviewState> s_richTextStates;

static bool startsWith(std::string_view text, std::string_view prefix) {
    if (text.size() < prefix.size()) return false;
    return text.compare(0, prefix.size(), prefix) == 0;
}

static std::string_view trimLeft(std::string_view text) {
    while (!text.empty() && std::isspace(static_cast<unsigned char>(text.front())) != 0) {
        text.remove_prefix(1);
    }
    return text;
}

static std::string decodeHtmlEntities(std::string_view text) {
    std::string result;
    result.reserve(text.size());
    for (size_t i = 0; i < text.size(); ++i) {
        if (text[i] == '&') {
            if (text.substr(i, 5) == "&amp;") {
                result.push_back('&');
                i += 4;
            } else if (text.substr(i, 4) == "&lt;") {
                result.push_back('<');
                i += 3;
            } else if (text.substr(i, 4) == "&gt;") {
                result.push_back('>');
                i += 3;
            } else if (text.substr(i, 6) == "&quot;") {
                result.push_back('"');
                i += 5;
            } else if (text.substr(i, 6) == "&apos;") {
                result.push_back('\'');
                i += 5;
            } else {
                result.push_back('&');
            }
        } else {
            result.push_back(text[i]);
        }
    }
    return result;
}

static std::string stripControlChars(std::string_view text) {
    std::string result;
    result.reserve(text.size());
    for (unsigned char ch : text) {
        if (ch == '\n' || ch == '\t' || ch >= 32) {
            result.push_back(static_cast<char>(ch));
        }
    }
    return result;
}

static std::string parseRtfToPlain(std::string_view text) {
    std::string result;
    result.reserve(text.size());
    bool inControlWord = false;
    bool inEscape = false;
    for (size_t i = 0; i < text.size(); ++i) {
        char c = text[i];
        if (inEscape) {
            if (c == '\\' || c == '{' || c == '}') {
                result.push_back(c);
            }
            inEscape = false;
            continue;
        }
        if (c == '\\') {
            if (i + 1 < text.size() && (text[i + 1] == '\\' || text[i + 1] == '{' || text[i + 1] == '}')) {
                inEscape = true;
                continue;
            }
            if (startsWith(text.substr(i), "\\par")) {
                result.push_back('\n');
                i += 3;
                continue;
            }
            inControlWord = true;
            continue;
        }
        if (inControlWord) {
            if (std::isspace(static_cast<unsigned char>(c)) != 0) {
                inControlWord = false;
            }
            continue;
        }
        if (c == '{' || c == '}') {
            continue;
        }
        result.push_back(c);
    }
    std::string cleaned;
    cleaned.reserve(result.size());
    bool atLineStart = true;
    for (char ch : result) {
        if (ch == '\n') {
            cleaned.push_back(ch);
            atLineStart = true;
            continue;
        }
        if (atLineStart && std::isspace(static_cast<unsigned char>(ch)) != 0) {
            continue;
        }
        if (!cleaned.empty() && cleaned.back() == ' ' && std::isspace(static_cast<unsigned char>(ch)) != 0) {
            continue;
        }
        cleaned.push_back(ch);
        atLineStart = false;
    }
    return cleaned;
}

static std::vector<RichTextSpan> parseInlineMarkdown(std::string_view text, uint8_t baseStyle) {
    std::vector<RichTextSpan> spans;
    std::string buffer;

    auto flushBuffer = [&](uint8_t style) {
        if (buffer.empty()) return;
        RichTextSpan span;
        span.text = buffer;
        span.style = style;
        spans.push_back(span);
        buffer.clear();
    };

    for (size_t i = 0; i < text.size();) {
        char c = text[i];
        if (c == '`') {
            size_t end = text.find('`', i + 1);
            if (end != std::string_view::npos) {
                flushBuffer(baseStyle);
                RichTextSpan span;
                span.text = std::string(text.substr(i + 1, end - i - 1));
                span.style = static_cast<uint8_t>(baseStyle | SpanStyleCode);
                spans.push_back(span);
                i = end + 1;
                continue;
            }
        }

        if (c == '[') {
            size_t closeBracket = text.find(']', i + 1);
            if (closeBracket != std::string_view::npos && closeBracket + 1 < text.size() && text[closeBracket + 1] == '(') {
                size_t closeParen = text.find(')', closeBracket + 2);
                if (closeParen != std::string_view::npos) {
                    flushBuffer(baseStyle);
                    RichTextSpan span;
                    span.text = std::string(text.substr(i + 1, closeBracket - i - 1));
                    span.link = std::string(text.substr(closeBracket + 2, closeParen - closeBracket - 2));
                    span.style = static_cast<uint8_t>(baseStyle | SpanStyleLink);
                    spans.push_back(span);
                    i = closeParen + 1;
                    continue;
                }
            }
        }

        if ((c == '*' || c == '_') && i + 1 < text.size() && text[i + 1] == c) {
            size_t end = text.find(std::string_view(text.substr(i, 2)), i + 2);
            if (end != std::string_view::npos) {
                flushBuffer(baseStyle);
                RichTextSpan span;
                span.text = std::string(text.substr(i + 2, end - i - 2));
                span.style = static_cast<uint8_t>(baseStyle | SpanStyleBold);
                spans.push_back(span);
                i = end + 2;
                continue;
            }
        }

        if (c == '*' || c == '_') {
            size_t end = text.find(c, i + 1);
            if (end != std::string_view::npos) {
                flushBuffer(baseStyle);
                RichTextSpan span;
                span.text = std::string(text.substr(i + 1, end - i - 1));
                span.style = static_cast<uint8_t>(baseStyle | SpanStyleItalic);
                spans.push_back(span);
                i = end + 1;
                continue;
            }
        }

        buffer.push_back(c);
        i++;
    }

    flushBuffer(baseStyle);
    return spans;
}

struct HtmlTag {
    std::string name;
    bool closing = false;
    bool selfClosing = false;
    std::string href;
};

static HtmlTag parseHtmlTag(std::string_view text) {
    HtmlTag tag;
    std::string raw(text);
    size_t start = 0;
    while (start < raw.size() && std::isspace(static_cast<unsigned char>(raw[start])) != 0) {
        start++;
    }
    size_t end = raw.size();
    while (end > start && std::isspace(static_cast<unsigned char>(raw[end - 1])) != 0) {
        end--;
    }
    if (start >= end) {
        return tag;
    }
    if (raw[start] == '/') {
        tag.closing = true;
        start++;
    }

    if (end > start && raw[end - 1] == '/') {
        tag.selfClosing = true;
        end--;
    }

    size_t nameEnd = start;
    while (nameEnd < end && std::isspace(static_cast<unsigned char>(raw[nameEnd])) == 0) {
        nameEnd++;
    }
    tag.name = raw.substr(start, nameEnd - start);
    std::transform(tag.name.begin(), tag.name.end(), tag.name.begin(),
                   [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });

    if (tag.name == "a") {
        std::string attrs = raw.substr(nameEnd, end - nameEnd);
        std::string lowerAttrs = attrs;
        std::transform(lowerAttrs.begin(), lowerAttrs.end(), lowerAttrs.begin(),
                       [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
        size_t hrefPos = lowerAttrs.find("href=");
        if (hrefPos != std::string::npos) {
            hrefPos += 5;
            while (hrefPos < attrs.size() && std::isspace(static_cast<unsigned char>(attrs[hrefPos])) != 0) {
                hrefPos++;
            }
            if (hrefPos < attrs.size() && (attrs[hrefPos] == '"' || attrs[hrefPos] == '\'')) {
                char quote = attrs[hrefPos];
                size_t hrefEnd = attrs.find(quote, hrefPos + 1);
                if (hrefEnd != std::string::npos) {
                    tag.href = attrs.substr(hrefPos + 1, hrefEnd - hrefPos - 1);
                }
            } else {
                size_t hrefEnd = attrs.find_first_of(" \t\r\n", hrefPos);
                tag.href = attrs.substr(hrefPos, hrefEnd - hrefPos);
            }
        }
    }

    return tag;
}

static std::vector<std::string> splitTokens(const std::string& text) {
    std::vector<std::string> tokens;
    size_t i = 0;
    while (i < text.size()) {
        bool space = std::isspace(static_cast<unsigned char>(text[i])) != 0;
        size_t j = i + 1;
        while (j < text.size()) {
            bool nextSpace = std::isspace(static_cast<unsigned char>(text[j])) != 0;
            if (nextSpace != space) break;
            j++;
        }
        tokens.emplace_back(text.substr(i, j - i));
        i = j;
    }
    return tokens;
}

static float headingScale(int level) {
    switch (level) {
        case 1: return 1.6f;
        case 2: return 1.4f;
        case 3: return 1.25f;
        case 4: return 1.15f;
        default: return 1.05f;
    }
}

static LayoutResult layoutRichText(const std::vector<RichTextLine>& lines, const Font& font, float maxWidth,
                                   const RichTextPreviewOptions& options, const Theme& theme) {
    LayoutResult result;
    if (maxWidth <= 0.0f) return result;

    float baseLineHeight = font.lineHeight() * options.lineSpacing;
    float indentWidth = std::max(font.measureText("    ").x, theme.metrics.paddingMedium * 2.0f);

    for (const auto& line : lines) {
        float lineHeight = baseLineHeight;
        if (line.isHeading) {
            lineHeight = baseLineHeight * headingScale(line.headingLevel);
        } else if (line.isCodeBlock) {
            lineHeight = baseLineHeight * 1.1f;
        }

        RenderLine current;
        current.isHeading = line.isHeading;
        current.headingLevel = line.headingLevel;
        current.isCodeBlock = line.isCodeBlock;
        current.isQuote = line.isQuote;
        current.lineHeight = lineHeight;
        current.indent = indentWidth * static_cast<float>(line.indent);
        current.prefix = line.prefix;

        float prefixWidth = 0.0f;
        if (!line.prefix.empty()) {
            prefixWidth = font.measureText(line.prefix + " ").x;
        }

        float textOffset = current.indent + prefixWidth;
        current.textOffset = textOffset;
        float x = textOffset;

        auto pushLine = [&](bool forceEmpty) {
            if (!current.spans.empty() || !current.prefix.empty() || current.isHeading || current.isCodeBlock || current.isQuote || forceEmpty) {
                result.contentHeight += current.lineHeight;
                result.contentWidth = std::max(result.contentWidth, x);
                result.lines.push_back(current);
            }
            current.spans.clear();
            current.prefix.clear();
            current.textOffset = textOffset;
            current.lineHeight = lineHeight;
            current.isHeading = line.isHeading;
            current.headingLevel = line.headingLevel;
            current.isCodeBlock = line.isCodeBlock;
            current.isQuote = line.isQuote;
            current.indent = indentWidth * static_cast<float>(line.indent);
            x = textOffset;
        };

        if (line.spans.empty()) {
            pushLine(true);
            continue;
        }

        for (const auto& span : line.spans) {
            bool wrap = options.wordWrap;
            std::vector<std::string> tokens = splitTokens(span.text);
            for (const auto& token : tokens) {
                if (token.empty()) continue;
                bool isSpace = std::isspace(static_cast<unsigned char>(token.front())) != 0;
                if (!line.isCodeBlock && isSpace && x <= textOffset) {
                    continue;
                }

                float tokenWidth = font.measureText(token).x;
                if (wrap && tokenWidth > 0.0f && x + tokenWidth > maxWidth && x > textOffset) {
                    pushLine(false);
                }

                RenderSpan renderSpan;
                renderSpan.text = token;
                renderSpan.style = span.style;
                renderSpan.link = span.link;

                if (!current.spans.empty() &&
                    current.spans.back().style == renderSpan.style &&
                    current.spans.back().link == renderSpan.link) {
                    current.spans.back().text += renderSpan.text;
                } else {
                    current.spans.push_back(renderSpan);
                }

                x += tokenWidth;
            }
        }

        pushLine(false);
    }

    return result;
}

static Color spanColor(const Theme& theme, const RenderLine& line, const RenderSpan& span) {
    Color color = theme.colors.text;
    if (span.style & SpanStyleItalic) {
        color = theme.colors.textSecondary;
    }
    if (span.style & SpanStyleCode) {
        color = theme.colors.textSecondary;
    }
    if (span.style & SpanStyleLink) {
        color = theme.colors.primary;
    }
    if (line.isHeading || (span.style & SpanStyleHeading)) {
        if (line.headingLevel <= 1) color = theme.colors.primary;
        else if (line.headingLevel == 2) color = theme.colors.text;
        else color = theme.colors.textSecondary;
    }
    if (line.isQuote || (span.style & SpanStyleQuote)) {
        color = theme.colors.textSecondary;
    }
    return color;
}

std::vector<RichTextLine> parseMarkdown(std::string_view text) {
    std::vector<RichTextLine> lines;
    std::string textStr(text);
    std::stringstream ss(textStr);
    std::string line;
    bool inCodeBlock = false;
    std::string codeFence;

    while (std::getline(ss, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        std::string_view view(line);
        if (startsWith(view, "```") || startsWith(view, "~~~")) {
            if (!inCodeBlock) {
                inCodeBlock = true;
                codeFence = std::string(view.substr(0, 3));
            } else if (startsWith(view, codeFence)) {
                inCodeBlock = false;
            }
            continue;
        }

        if (inCodeBlock) {
            RichTextLine parsed;
            parsed.isCodeBlock = true;
            RichTextSpan span;
            span.text = line;
            span.style = SpanStyleCode;
            parsed.spans.push_back(span);
            lines.push_back(parsed);
            continue;
        }

        if (view.empty()) {
            lines.push_back(RichTextLine{});
            continue;
        }

        size_t hashCount = 0;
        while (hashCount < view.size() && view[hashCount] == '#') {
            hashCount++;
        }
        if (hashCount > 0 && hashCount <= 6 && (hashCount == view.size() || view[hashCount] == ' ')) {
            RichTextLine parsed;
            parsed.isHeading = true;
            parsed.headingLevel = static_cast<int>(hashCount);
            std::string_view content = trimLeft(view.substr(hashCount));
            parsed.spans = parseInlineMarkdown(content, SpanStyleHeading);
            lines.push_back(parsed);
            continue;
        }

        if (!view.empty() && view.front() == '>') {
            RichTextLine parsed;
            parsed.isQuote = true;
            parsed.indent = 1;
            std::string_view content = trimLeft(view.substr(1));
            parsed.spans = parseInlineMarkdown(content, SpanStyleQuote);
            lines.push_back(parsed);
            continue;
        }

        if (view.size() >= 2 && (view[0] == '-' || view[0] == '*' || view[0] == '+') && view[1] == ' ') {
            RichTextLine parsed;
            parsed.prefix = "-";
            parsed.indent = 1;
            parsed.spans = parseInlineMarkdown(view.substr(2), SpanStyleNone);
            lines.push_back(parsed);
            continue;
        }

        size_t digitCount = 0;
        while (digitCount < view.size() && std::isdigit(static_cast<unsigned char>(view[digitCount])) != 0) {
            digitCount++;
        }
        if (digitCount > 0 && digitCount + 1 < view.size() && view[digitCount] == '.' && view[digitCount + 1] == ' ') {
            RichTextLine parsed;
            parsed.prefix = std::string(view.substr(0, digitCount)) + ".";
            parsed.indent = 1;
            parsed.spans = parseInlineMarkdown(view.substr(digitCount + 2), SpanStyleNone);
            lines.push_back(parsed);
            continue;
        }

        RichTextLine parsed;
        parsed.spans = parseInlineMarkdown(view, SpanStyleNone);
        lines.push_back(parsed);
    }

    return lines;
}

std::vector<RichTextLine> parseHtml(std::string_view text) {
    std::vector<RichTextLine> lines;
    std::string textStr(text);

    uint8_t currentStyle = SpanStyleNone;
    std::string currentLink;
    bool inPre = false;
    bool inHeading = false;
    int headingLevel = 0;
    bool inQuote = false;
    bool orderedList = false;
    int listIndex = 0;
    bool listItemOpen = false;
    std::string pendingPrefix;
    std::string buffer;

    auto makeLineFromContext = [&]() -> RichTextLine {
        RichTextLine line;
        line.isHeading = inHeading;
        line.headingLevel = headingLevel;
        line.isCodeBlock = inPre;
        line.isQuote = inQuote;
        line.indent = 0;
        if (inQuote) line.indent += 1;
        if (listItemOpen) line.indent += 1;
        if (!pendingPrefix.empty()) {
            line.prefix = pendingPrefix;
            pendingPrefix.clear();
        }
        return line;
    };

    RichTextLine currentLine = makeLineFromContext();

    auto flushBuffer = [&]() {
        if (buffer.empty()) return;
        RichTextSpan span;
        span.text = decodeHtmlEntities(buffer);
        span.style = currentStyle;
        span.link = currentLink;
        if (inHeading) span.style |= SpanStyleHeading;
        if (inQuote) span.style |= SpanStyleQuote;
        if (inPre) span.style |= SpanStyleCode;
        currentLine.spans.push_back(span);
        buffer.clear();
    };

    auto pushLine = [&](bool forceEmpty) {
        flushBuffer();
        if (!currentLine.spans.empty() || !currentLine.prefix.empty() || currentLine.isHeading || currentLine.isCodeBlock || currentLine.isQuote || forceEmpty) {
            lines.push_back(currentLine);
        } else if (forceEmpty) {
            lines.push_back(RichTextLine{});
        }
        currentLine = makeLineFromContext();
    };

    auto pushStyle = [&](uint8_t style, const std::string& link = "") {
        currentStyle |= style;
        if (!link.empty()) {
            currentLink = link;
            currentStyle |= SpanStyleLink;
        }
    };

    auto popStyle = [&](uint8_t style) {
        currentStyle = static_cast<uint8_t>(currentStyle & ~style);
        if ((style & SpanStyleLink) != 0) {
            currentLink.clear();
        }
    };

    for (size_t i = 0; i < textStr.size();) {
        char c = textStr[i];
        if (c == '<') {
            size_t end = textStr.find('>', i + 1);
            if (end == std::string::npos) {
                buffer.push_back(c);
                i++;
                continue;
            }
            flushBuffer();
            HtmlTag tag = parseHtmlTag(std::string_view(textStr.data() + i + 1, end - i - 1));
            const std::string& name = tag.name;

            if (tag.closing) {
                if (name == "strong" || name == "b") popStyle(SpanStyleBold);
                else if (name == "em" || name == "i") popStyle(SpanStyleItalic);
                else if (name == "code") popStyle(SpanStyleCode);
                else if (name == "a") popStyle(SpanStyleLink);
                else if (name == "pre") { pushLine(false); inPre = false; }
                else if (name == "p") { pushLine(false); }
                else if (name == "blockquote") { pushLine(false); inQuote = false; }
                else if (name == "ul" || name == "ol") { orderedList = false; listIndex = 0; }
                else if (name == "li") {
                    pushLine(false);
                    listItemOpen = false;
                    currentLine = makeLineFromContext();
                }
                else if (!name.empty() && name[0] == 'h' && name.size() == 2 && std::isdigit(static_cast<unsigned char>(name[1])) != 0) {
                    pushLine(false);
                    inHeading = false;
                    headingLevel = 0;
                    pushLine(true);
                }
            } else {
                if (name == "br") {
                    pushLine(false);
                } else if (name == "strong" || name == "b") {
                    pushStyle(SpanStyleBold);
                } else if (name == "em" || name == "i") {
                    pushStyle(SpanStyleItalic);
                } else if (name == "code") {
                    pushStyle(SpanStyleCode);
                } else if (name == "a") {
                    pushStyle(SpanStyleLink, tag.href);
                } else if (name == "pre") {
                    pushLine(false);
                    inPre = true;
                } else if (name == "p") {
                    pushLine(false);
                } else if (name == "blockquote") {
                    pushLine(false);
                    inQuote = true;
                } else if (name == "ul") {
                    orderedList = false;
                    listIndex = 0;
                } else if (name == "ol") {
                    orderedList = true;
                    listIndex = 0;
                } else if (name == "li") {
                    if (!currentLine.spans.empty() || !currentLine.prefix.empty()) {
                        pushLine(false);
                    }
                    listItemOpen = true;
                    if (orderedList) {
                        listIndex++;
                        pendingPrefix = std::to_string(listIndex) + ".";
                    } else {
                        pendingPrefix = "-";
                    }
                    currentLine = makeLineFromContext();
                } else if (!name.empty() && name[0] == 'h' && name.size() == 2 && std::isdigit(static_cast<unsigned char>(name[1])) != 0) {
                    pushLine(false);
                    inHeading = true;
                    headingLevel = std::clamp(name[1] - '0', 1, 6);
                }
            }

            i = end + 1;
            continue;
        }

        if (inPre) {
            if (c == '\n') {
                pushLine(false);
            } else {
                buffer.push_back(c);
            }
            i++;
            continue;
        }

        if (std::isspace(static_cast<unsigned char>(c)) != 0) {
            if (!buffer.empty() && buffer.back() != ' ') {
                buffer.push_back(' ');
            }
            i++;
            continue;
        }

        buffer.push_back(c);
        i++;
    }

    pushLine(false);
    return lines;
}

std::vector<RichTextLine> parseRichText(std::string_view text, RichTextFormat format) {
    std::string normalized = stripControlChars(text);
    std::string_view view = normalized;
    std::string rtfPlain;
    if (startsWith(trimLeft(view), "{\\rtf") || startsWith(trimLeft(view), "{\\RTF")) {
        rtfPlain = parseRtfToPlain(view);
        view = rtfPlain;
    }

    if (format == RichTextFormat::Html) {
        return parseHtml(view);
    }
    if (format == RichTextFormat::Markdown) {
        return parseMarkdown(view);
    }
    if (view.find('<') != std::string_view::npos && view.find('>') != std::string_view::npos) {
        return parseHtml(view);
    }
    return parseMarkdown(view);
}

} // namespace internal
} // namespace rich_text

void RichTextPreview(Context& ctx, const char* id, std::string_view text, const RichTextPreviewOptions& options) {
    const Theme& theme = ctx.theme();
    IDrawList& dl = *ctx.activeDrawList();
    Font* font = ctx.font();

    float width = options.style.width > 0 ? options.style.width : options.width;
    if (width <= 0.0f) width = 320.0f;
    float height = options.style.height > 0 ? options.style.height : options.height;

    Rect bounds = allocateWidgetBounds(ctx, options.style, width, height);
    float radius = options.style.borderRadius > 0 ? options.style.borderRadius : theme.metrics.borderRadiusSmall;

    if (options.showBackground) {
        dl.addRectFilled(bounds, theme.colors.inputBackground, radius);
    }
    if (options.showBorder) {
        dl.addRect(bounds, theme.colors.inputBorder, radius);
    }
    if (!font) return;

    WidgetId widgetId = ctx.makeId(id);
    auto& state = rich_text::internal::s_richTextStates[widgetId];

    float padding = theme.metrics.paddingSmall;
    Rect contentRect = bounds.shrunk(padding);
    if (contentRect.width() <= 0.0f || contentRect.height() <= 0.0f) return;

    auto lines = rich_text::internal::parseRichText(text, options.format);
    rich_text::internal::LayoutResult layout = rich_text::internal::layoutRichText(
        lines, *font, contentRect.width(), options, theme);

    InputState& input = ctx.input();
    if (bounds.contains(input.mousePos()) && !ctx.isOccluded(input.mousePos())) {
        float scrollStep = font->lineHeight() * 3.0f;
        state.scrollOffsetY -= input.scrollDelta().y * scrollStep;
    }

    float maxScroll = std::max(0.0f, layout.contentHeight - contentRect.height());
    state.scrollOffsetY = std::clamp(state.scrollOffsetY, 0.0f, maxScroll);

    dl.pushClipRect(contentRect);

    float y = contentRect.y() - state.scrollOffsetY;
    for (const auto& line : layout.lines) {
        float lineBottom = y + line.lineHeight;
        if (lineBottom < contentRect.y()) {
            y += line.lineHeight;
            continue;
        }
        if (y > contentRect.bottom()) break;

        float textY = y + (line.lineHeight - font->lineHeight()) * 0.5f;
        float textX = contentRect.x() + line.textOffset;

        if (line.isCodeBlock) {
            Rect bgRect(contentRect.x() + line.indent, y + 1,
                        contentRect.width() - line.indent, line.lineHeight - 2);
            dl.addRectFilled(bgRect, theme.colors.panelBackground.darker(0.04f),
                             theme.metrics.borderRadiusSmall);
        }

        if (line.isQuote) {
            float barX = contentRect.x() + line.indent - theme.metrics.paddingSmall;
            Rect barRect(barX, y + 2, 3.0f, line.lineHeight - 4);
            dl.addRectFilled(barRect, theme.colors.borderHover, 1.0f);
        }

        if (!line.prefix.empty()) {
            Vec2 prefixPos(contentRect.x() + line.indent, textY);
            dl.addText(font, prefixPos, line.prefix, theme.colors.textSecondary);
        }

        float x = textX;
        for (const auto& span : line.spans) {
            if (span.text.empty()) continue;
            float textWidth = font->measureText(span.text).x;
            Color color = rich_text::internal::spanColor(theme, line, span);

            if (span.style & rich_text::internal::SpanStyleCode) {
                Rect codeRect(x - 2.0f, y + 2.0f, textWidth + 4.0f, line.lineHeight - 4.0f);
                dl.addRectFilled(codeRect, theme.colors.panelBackground.darker(0.08f),
                                 theme.metrics.borderRadiusSmall);
            }

            dl.addText(font, Vec2(x, textY), span.text, color);

            if (span.style & rich_text::internal::SpanStyleBold) {
                dl.addText(font, Vec2(x + 1.0f, textY), span.text, color);
            }

            if (span.style & rich_text::internal::SpanStyleLink) {
                float underlineY = textY + font->lineHeight() - 2.0f;
                dl.addLine(Vec2(x, underlineY), Vec2(x + textWidth, underlineY), color, 1.0f);
            }

            x += textWidth;
        }

        if (line.isHeading) {
            float underlineY = y + line.lineHeight - 3.0f;
            dl.addLine(Vec2(contentRect.x() + line.indent, underlineY),
                       Vec2(contentRect.right(), underlineY),
                       theme.colors.borderHover, 1.0f);
        }

        y += line.lineHeight;
    }

    dl.popClipRect();

    if (options.showScrollbar && layout.contentHeight > contentRect.height()) {
        float scrollbarWidth = 8.0f;
        Rect track(bounds.right() - scrollbarWidth - 2.0f, bounds.y() + 2.0f,
                   scrollbarWidth, bounds.height() - 4.0f);
        dl.addRectFilled(track, theme.colors.scrollbarTrack);

        float thumbHeight = std::max(20.0f, (contentRect.height() / layout.contentHeight) * track.height());
        float thumbY = track.y() + (state.scrollOffsetY / maxScroll) * (track.height() - thumbHeight);
        Rect thumb(track.x() + 1.0f, thumbY, track.width() - 2.0f, thumbHeight);
        Color thumbColor = track.contains(input.mousePos()) ? theme.colors.scrollbarThumbHover : theme.colors.scrollbarThumb;
        dl.addRectFilled(thumb, thumbColor, scrollbarWidth / 2);
    }
}

void RichTextPreview(Context& ctx, const std::string& id, std::string_view text,
                     const RichTextPreviewOptions& options) {
    RichTextPreview(ctx, id.c_str(), text, options);
}

} // namespace fst

