/**
 * @file time_picker.cpp
 * @brief TimePicker widget implementation.
 */

#include "fastener/widgets/time_picker.h"
#include "fastener/core/context.h"
#include "fastener/graphics/draw_list.h"
#include "fastener/graphics/font.h"
#include "fastener/ui/widget.h"
#include "fastener/ui/widget_utils.h"
#include "fastener/ui/theme.h"
#include "fastener/ui/layout.h"
#include <algorithm>
#include <cstdio>
#include <ctime>
#include <unordered_map>

namespace fst {

//=============================================================================
// Time Utilities
//=============================================================================

namespace time_utils {

bool isValidTime(const TimeOfDay& time) {
    return time.hour >= 0 && time.hour <= 23 &&
           time.minute >= 0 && time.minute <= 59 &&
           time.second >= 0 && time.second <= 59;
}

TimeOfDay clampTime(const TimeOfDay& time) {
    TimeOfDay result = time;
    result.hour = std::clamp(result.hour, 0, 23);
    result.minute = std::clamp(result.minute, 0, 59);
    result.second = std::clamp(result.second, 0, 59);
    return result;
}

std::string formatTime(const TimeOfDay& time, TimeFormat format, bool showSeconds) {
    char buffer[32];
    if (format == TimeFormat::H12) {
        bool isPm = time.hour >= 12;
        int displayHour = time.hour % 12;
        if (displayHour == 0) displayHour = 12;
        if (showSeconds) {
            std::snprintf(buffer, sizeof(buffer), "%02d:%02d:%02d %s",
                          displayHour, time.minute, time.second, isPm ? "PM" : "AM");
        } else {
            std::snprintf(buffer, sizeof(buffer), "%02d:%02d %s",
                          displayHour, time.minute, isPm ? "PM" : "AM");
        }
    } else {
        if (showSeconds) {
            std::snprintf(buffer, sizeof(buffer), "%02d:%02d:%02d",
                          time.hour, time.minute, time.second);
        } else {
            std::snprintf(buffer, sizeof(buffer), "%02d:%02d",
                          time.hour, time.minute);
        }
    }
    return buffer;
}

} // namespace time_utils

//=============================================================================
// TimePicker State
//=============================================================================

struct TimePickerState {
    bool isOpen = false;
};

static std::unordered_map<WidgetId, TimePickerState> s_timePickerStates;

//=============================================================================
// Helpers
//=============================================================================

static TimeOfDay getNowLocal() {
    std::time_t t = std::time(nullptr);
    std::tm localTime{};
#ifdef _WIN32
    localtime_s(&localTime, &t);
#else
    localTime = *std::localtime(&t);
#endif
    return TimeOfDay{localTime.tm_hour, localTime.tm_min, localTime.tm_sec};
}

static int wrapAdd(int value, int delta, int minValue, int maxValue) {
    int range = maxValue - minValue + 1;
    int offset = (value - minValue + delta) % range;
    if (offset < 0) offset += range;
    return minValue + offset;
}

//=============================================================================
// TimePicker Implementation
//=============================================================================

bool TimePicker(Context& ctx, std::string_view label, TimeOfDay& time, const TimePickerOptions& options) {
    auto wc = WidgetContext::make(ctx);
    const Theme& theme = *wc.theme;
    IDrawList& dl = *wc.dl;
    Font* font = wc.font;
    InputState& input = ctx.input();

    bool changed = false;

    TimeOfDay normalized = time_utils::clampTime(time);
    if (normalized != time) {
        time = normalized;
        changed = true;
    }

    WidgetId id = ctx.makeId(label);
    TimePickerState& state = s_timePickerStates[id];

    float width = options.style.width > 0 ? options.style.width : 120.0f;
    float height = options.style.height > 0 ? options.style.height : theme.metrics.inputHeight;
    float labelWidth = 0.0f;
    if (font && !label.empty()) {
        labelWidth = font->measureText(label).x + theme.metrics.paddingMedium;
    }

    Rect bounds = allocateWidgetBounds(ctx, options.style, labelWidth + width, height);
    Rect inputRect(bounds.x() + labelWidth, bounds.y(), width, height);

    WidgetInteraction interaction = handleWidgetInteraction(ctx, id, inputRect, true);
    WidgetState widgetState = getWidgetState(ctx, id);
    widgetState.disabled = options.disabled;

    if (interaction.clicked && !options.disabled) {
        state.isOpen = !state.isOpen;
    }

    // Popup layout
    float padding = theme.metrics.paddingSmall;
    float headerHeight = font ? font->lineHeight() + padding * 2.0f : 24.0f;
    float controlHeight = theme.metrics.inputHeight;
    float buttonHeight = std::max(12.0f, controlHeight * 0.4f);
    float valueHeight = controlHeight;
    float columnHeight = buttonHeight * 2.0f + valueHeight;
    int timeColumns = options.showSeconds ? 3 : 2;
    bool showAmPm = !options.use24Hour;
    int totalColumns = timeColumns + (showAmPm ? 1 : 0);
    float gap = padding;
    float columnWidth = 52.0f;
    float contentWidth = totalColumns * columnWidth + (totalColumns - 1) * gap;
    float popupWidth = options.popupWidth > 0.0f ? options.popupWidth : std::max(width, contentWidth + padding * 2.0f);
    float footerHeight = options.showNowButton ? (controlHeight + padding) : 0.0f;
    float popupHeight = padding * 2.0f + headerHeight + columnHeight + footerHeight;
    Rect popupRect(inputRect.x(), inputRect.bottom() + 2.0f, popupWidth, popupHeight);

    if (state.isOpen && input.isMousePressed(MouseButton::Left)) {
        if (!inputRect.contains(input.mousePos()) && !popupRect.contains(input.mousePos())) {
            state.isOpen = false;
        }
    }

    int hourStep = options.hourStep > 0 ? options.hourStep : 1;
    int minuteStep = options.minuteStep > 0 ? options.minuteStep : 1;
    int secondStep = options.secondStep > 0 ? options.secondStep : 1;

    WidgetId hourUpId = combineIds(id, hashString("hour_up"));
    WidgetId hourDownId = combineIds(id, hashString("hour_down"));
    WidgetId minUpId = combineIds(id, hashString("min_up"));
    WidgetId minDownId = combineIds(id, hashString("min_down"));
    WidgetId secUpId = combineIds(id, hashString("sec_up"));
    WidgetId secDownId = combineIds(id, hashString("sec_down"));
    WidgetId ampmId = combineIds(id, hashString("ampm"));
    WidgetId nowId = combineIds(id, hashString("now"));

    if (state.isOpen) {
        if (popupRect.contains(input.mousePos())) {
            input.consumeMouse();
        }

        float columnsX = popupRect.x() + (popupWidth - contentWidth) * 0.5f;
        float columnsY = popupRect.y() + padding + headerHeight;

        auto columnRect = [&](int index) {
            float x = columnsX + index * (columnWidth + gap);
            return Rect(x, columnsY, columnWidth, columnHeight);
        };

        auto upRect = [&](const Rect& col) {
            return Rect(col.x(), col.y(), col.width(), buttonHeight);
        };
        auto valueRect = [&](const Rect& col) {
            return Rect(col.x(), col.y() + buttonHeight, col.width(), valueHeight);
        };
        auto downRect = [&](const Rect& col) {
            return Rect(col.x(), col.y() + buttonHeight + valueHeight, col.width(), buttonHeight);
        };

        int columnIndex = 0;
        Rect hourCol = columnRect(columnIndex++);
        Rect minCol = columnRect(columnIndex++);
        Rect secCol;
        if (options.showSeconds) {
            secCol = columnRect(columnIndex++);
        }
        Rect ampmCol;
        if (showAmPm) {
            ampmCol = columnRect(columnIndex++);
        }

        WidgetInteraction hourUp = handleWidgetInteraction(ctx, hourUpId, upRect(hourCol), false);
        WidgetInteraction hourDown = handleWidgetInteraction(ctx, hourDownId, downRect(hourCol), false);
        WidgetInteraction minUp = handleWidgetInteraction(ctx, minUpId, upRect(minCol), false);
        WidgetInteraction minDown = handleWidgetInteraction(ctx, minDownId, downRect(minCol), false);

        WidgetInteraction secUp{};
        WidgetInteraction secDown{};
        if (options.showSeconds) {
            secUp = handleWidgetInteraction(ctx, secUpId, upRect(secCol), false);
            secDown = handleWidgetInteraction(ctx, secDownId, downRect(secCol), false);
        }

        WidgetInteraction ampmInteract{};
        if (showAmPm) {
            ampmInteract = handleWidgetInteraction(ctx, ampmId, valueRect(ampmCol), false);
        }

        if (!options.disabled) {
            if (hourUp.clicked) {
                time.hour = wrapAdd(time.hour, hourStep, 0, 23);
                changed = true;
            }
            if (hourDown.clicked) {
                time.hour = wrapAdd(time.hour, -hourStep, 0, 23);
                changed = true;
            }
            if (minUp.clicked) {
                time.minute = wrapAdd(time.minute, minuteStep, 0, 59);
                changed = true;
            }
            if (minDown.clicked) {
                time.minute = wrapAdd(time.minute, -minuteStep, 0, 59);
                changed = true;
            }
            if (options.showSeconds) {
                if (secUp.clicked) {
                    time.second = wrapAdd(time.second, secondStep, 0, 59);
                    changed = true;
                }
                if (secDown.clicked) {
                    time.second = wrapAdd(time.second, -secondStep, 0, 59);
                    changed = true;
                }
            }
            if (showAmPm && ampmInteract.clicked) {
                time.hour = (time.hour + 12) % 24;
                changed = true;
            }
        }

        if (options.showNowButton) {
            Rect nowRect(popupRect.x() + padding, popupRect.y() + padding + headerHeight + columnHeight + padding,
                         popupWidth - padding * 2.0f, controlHeight);
            WidgetInteraction nowInteract = handleWidgetInteraction(ctx, nowId, nowRect, true);
            if (!options.disabled && nowInteract.clicked) {
                time = getNowLocal();
                changed = true;
            }
        }
    }

    // Draw label
    if (font && !label.empty()) {
        float textY = layout_utils::verticalCenterY(bounds.y(), height, font->lineHeight());
        Color labelColor = options.disabled ? theme.colors.textDisabled : theme.colors.text;
        dl.addText(font, Vec2(bounds.x(), textY), std::string(label), labelColor);
    }

    // Draw input box
    Color bgColor;
    if (options.disabled) {
        bgColor = theme.colors.inputBackground.withAlpha(0.5f);
    } else if (state.isOpen || widgetState.active) {
        bgColor = theme.colors.inputBackground.lighter(0.1f);
    } else if (widgetState.hovered) {
        bgColor = theme.colors.inputBackground.lighter(0.05f);
    } else {
        bgColor = theme.colors.inputBackground;
    }

    float radius = theme.metrics.borderRadiusSmall;
    dl.addRectFilled(inputRect, bgColor, radius);

    Color borderColor = state.isOpen ? theme.colors.borderFocused : theme.colors.inputBorder;
    dl.addRect(inputRect, borderColor, radius);

    if (font) {
        TimeFormat fmt = options.use24Hour ? TimeFormat::H24 : TimeFormat::H12;
        std::string timeText = time_utils::formatTime(time, fmt, options.showSeconds);
        Vec2 textPos(
            inputRect.x() + theme.metrics.paddingSmall,
            inputRect.y() + (height - font->lineHeight()) * 0.5f
        );
        Color textColor = options.disabled ? theme.colors.textDisabled : theme.colors.text;
        dl.pushClipRect(Rect(inputRect.x(), inputRect.y(), inputRect.width() - 20, inputRect.height()));
        dl.addText(font, textPos, timeText, textColor);
        dl.popClipRect();
    }

    // Dropdown arrow
    {
        float arrowSize = 6.0f;
        Vec2 arrowCenter(inputRect.right() - 12, inputRect.center().y);
        Color arrowColor = options.disabled ? theme.colors.textDisabled : theme.colors.textSecondary;
        if (state.isOpen) {
            dl.addTriangleFilled(
                Vec2(arrowCenter.x - arrowSize, arrowCenter.y + arrowSize * 0.5f),
                Vec2(arrowCenter.x + arrowSize, arrowCenter.y + arrowSize * 0.5f),
                Vec2(arrowCenter.x, arrowCenter.y - arrowSize * 0.5f),
                arrowColor
            );
        } else {
            dl.addTriangleFilled(
                Vec2(arrowCenter.x - arrowSize, arrowCenter.y - arrowSize * 0.5f),
                Vec2(arrowCenter.x + arrowSize, arrowCenter.y - arrowSize * 0.5f),
                Vec2(arrowCenter.x, arrowCenter.y + arrowSize * 0.5f),
                arrowColor
            );
        }
    }

    if (state.isOpen) {
        TimeOfDay timeCopy = time;
        bool showSeconds = options.showSeconds;
        bool showAmPm = !options.use24Hour;
        bool disabled = options.disabled;

        ctx.deferRender([=, &ctx]() {
            IDrawList& dl = *ctx.activeDrawList();
            Font* font = ctx.font();
            const Theme& theme = ctx.theme();

            float padding = theme.metrics.paddingSmall;
            float headerHeight = font ? font->lineHeight() + padding * 2.0f : 24.0f;
            float controlHeight = theme.metrics.inputHeight;
            float buttonHeight = std::max(12.0f, controlHeight * 0.4f);
            float valueHeight = controlHeight;
            float columnHeight = buttonHeight * 2.0f + valueHeight;
            int timeColumns = showSeconds ? 3 : 2;
            int totalColumns = timeColumns + (showAmPm ? 1 : 0);
            float gap = padding;
            float columnWidth = 52.0f;
            float contentWidth = totalColumns * columnWidth + (totalColumns - 1) * gap;
            float popupWidth = options.popupWidth > 0.0f ? options.popupWidth : std::max(width, contentWidth + padding * 2.0f);
            float footerHeight = options.showNowButton ? (controlHeight + padding) : 0.0f;
            float popupHeight = padding * 2.0f + headerHeight + columnHeight + footerHeight;
            Rect popupRect(inputRect.x(), inputRect.bottom() + 2.0f, popupWidth, popupHeight);

            ctx.addFloatingWindowRect(popupRect);
            dl.addShadow(popupRect, theme.colors.shadow, 8.0f, 4.0f);
            dl.addRectFilled(popupRect, theme.colors.panelBackground, 4.0f);
            dl.addRect(popupRect, theme.colors.border, 4.0f);

            float columnsX = popupRect.x() + (popupWidth - contentWidth) * 0.5f;
            float columnsY = popupRect.y() + padding + headerHeight;

            auto columnRect = [&](int index) {
                float x = columnsX + index * (columnWidth + gap);
                return Rect(x, columnsY, columnWidth, columnHeight);
            };
            auto upRect = [&](const Rect& col) {
                return Rect(col.x(), col.y(), col.width(), buttonHeight);
            };
            auto valueRect = [&](const Rect& col) {
                return Rect(col.x(), col.y() + buttonHeight, col.width(), valueHeight);
            };
            auto downRect = [&](const Rect& col) {
                return Rect(col.x(), col.y() + buttonHeight + valueHeight, col.width(), buttonHeight);
            };

            int columnIndex = 0;
            Rect hourCol = columnRect(columnIndex++);
            Rect minCol = columnRect(columnIndex++);
            Rect secCol;
            if (showSeconds) {
                secCol = columnRect(columnIndex++);
            }
            Rect ampmCol;
            if (showAmPm) {
                ampmCol = columnRect(columnIndex++);
            }

            auto drawButton = [&](const Rect& rect, WidgetId btnId, bool drawUp) {
                WidgetState state = getWidgetState(ctx, btnId);
                state.disabled = disabled;
                Color bg = getStateColor(theme.colors.buttonBackground, theme.colors.buttonHover,
                                         theme.colors.buttonActive, state);
                dl.addRectFilled(rect, bg, theme.metrics.borderRadiusSmall);
                dl.addRect(rect, theme.colors.border, theme.metrics.borderRadiusSmall);

                Color arrowColor = state.disabled ? theme.colors.textDisabled : theme.colors.text;
                if (drawUp) {
                    dl.addTriangleFilled(
                        Vec2(rect.center().x - 4, rect.center().y + 2),
                        Vec2(rect.center().x + 4, rect.center().y + 2),
                        Vec2(rect.center().x, rect.center().y - 3),
                        arrowColor
                    );
                } else {
                    dl.addTriangleFilled(
                        Vec2(rect.center().x - 4, rect.center().y - 2),
                        Vec2(rect.center().x + 4, rect.center().y - 2),
                        Vec2(rect.center().x, rect.center().y + 3),
                        arrowColor
                    );
                }
            };

            drawButton(upRect(hourCol), hourUpId, true);
            drawButton(downRect(hourCol), hourDownId, false);
            drawButton(upRect(minCol), minUpId, true);
            drawButton(downRect(minCol), minDownId, false);
            if (showSeconds) {
                drawButton(upRect(secCol), secUpId, true);
                drawButton(downRect(secCol), secDownId, false);
            }

            auto drawValueBox = [&](const Rect& rect) {
                dl.addRectFilled(rect, theme.colors.inputBackground, theme.metrics.borderRadiusSmall);
                dl.addRect(rect, theme.colors.border, theme.metrics.borderRadiusSmall);
            };

            drawValueBox(valueRect(hourCol));
            drawValueBox(valueRect(minCol));
            if (showSeconds) {
                drawValueBox(valueRect(secCol));
            }

            if (font) {
                auto drawValue = [&](const Rect& rect, const std::string& value) {
                    Vec2 textSize = font->measureText(value);
                    Vec2 textPos(
                        rect.center().x - textSize.x * 0.5f,
                        rect.y() + (rect.height() - font->lineHeight()) * 0.5f
                    );
                    Color textColor = disabled ? theme.colors.textDisabled : theme.colors.text;
                    dl.addText(font, textPos, value, textColor);
                };

                char buffer[16];
                int displayHour = timeCopy.hour;
                if (showAmPm) {
                    displayHour = timeCopy.hour % 12;
                    if (displayHour == 0) displayHour = 12;
                }
                std::snprintf(buffer, sizeof(buffer), "%02d", displayHour);
                drawValue(valueRect(hourCol), buffer);

                std::snprintf(buffer, sizeof(buffer), "%02d", timeCopy.minute);
                drawValue(valueRect(minCol), buffer);

                if (showSeconds) {
                    std::snprintf(buffer, sizeof(buffer), "%02d", timeCopy.second);
                    drawValue(valueRect(secCol), buffer);
                }

                if (showAmPm) {
                    Rect ampmValue = valueRect(ampmCol);
                    WidgetState ampmState = getWidgetState(ctx, ampmId);
                    ampmState.disabled = disabled;
                    Color ampmBg = getStateColor(theme.colors.buttonBackground, theme.colors.buttonHover,
                                                 theme.colors.buttonActive, ampmState);
                    dl.addRectFilled(ampmValue, ampmBg, theme.metrics.borderRadiusSmall);
                    dl.addRect(ampmValue, theme.colors.border, theme.metrics.borderRadiusSmall);
                    const char* label = (timeCopy.hour >= 12) ? "PM" : "AM";
                    Vec2 textSize = font->measureText(label);
                    Vec2 textPos(
                        ampmValue.center().x - textSize.x * 0.5f,
                        ampmValue.y() + (ampmValue.height() - font->lineHeight()) * 0.5f
                    );
                    Color textColor = ampmState.disabled ? theme.colors.textDisabled : theme.colors.text;
                    dl.addText(font, textPos, label, textColor);
                }
            }

            if (options.showNowButton) {
                Rect nowRect(popupRect.x() + padding, popupRect.y() + padding + headerHeight + columnHeight + padding,
                             popupWidth - padding * 2.0f, controlHeight);
                WidgetState nowState = getWidgetState(ctx, nowId);
                nowState.disabled = disabled;
                Color nowBg = getStateColor(theme.colors.buttonBackground, theme.colors.buttonHover,
                                            theme.colors.buttonActive, nowState);
                dl.addRectFilled(nowRect, nowBg, theme.metrics.borderRadiusSmall);
                dl.addRect(nowRect, theme.colors.border, theme.metrics.borderRadiusSmall);

                if (font) {
                    Vec2 textSize = font->measureText("Now");
                    Vec2 textPos(
                        nowRect.center().x - textSize.x * 0.5f,
                        nowRect.y() + (nowRect.height() - font->lineHeight()) * 0.5f
                    );
                    Color textColor = nowState.disabled ? theme.colors.textDisabled : theme.colors.text;
                    dl.addText(font, textPos, "Now", textColor);
                }
            }

            if (font) {
                auto drawHeader = [&](const Rect& col, const char* label) {
                    Vec2 textSize = font->measureText(label);
                    float x = col.x() + (col.width() - textSize.x) * 0.5f;
                    float y = popupRect.y() + padding + (headerHeight - font->lineHeight()) * 0.5f;
                    dl.addText(font, Vec2(x, y), label, theme.colors.textSecondary);
                };
                drawHeader(hourCol, "Hour");
                drawHeader(minCol, "Min");
                if (showSeconds) drawHeader(secCol, "Sec");
                if (showAmPm) drawHeader(ampmCol, "AM/PM");
            }
        });
    }

    return changed;
}

} // namespace fst
