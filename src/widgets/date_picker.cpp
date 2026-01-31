/**
 * @file date_picker.cpp
 * @brief DatePicker widget implementation.
 */

#include "fastener/widgets/date_picker.h"
#include "fastener/core/context.h"
#include "fastener/graphics/draw_list.h"
#include "fastener/graphics/font.h"
#include "fastener/ui/widget.h"
#include "fastener/ui/widget_utils.h"
#include "fastener/ui/theme.h"
#include "fastener/ui/layout.h"
#include <algorithm>
#include <array>
#include <cstdio>
#include <ctime>
#include <unordered_map>

namespace fst {

//=============================================================================
// Date Utilities
//=============================================================================

namespace date_utils {

bool isLeapYear(int year) {
    if (year % 400 == 0) return true;
    if (year % 100 == 0) return false;
    return (year % 4 == 0);
}

int daysInMonth(int year, int month) {
    static constexpr int kDays[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (month < 1 || month > 12) return 0;
    if (month == 2 && isLeapYear(year)) return 29;
    return kDays[month - 1];
}

int dayOfWeek(int year, int month, int day) {
    static constexpr int kOffsets[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
    if (month < 1 || month > 12) return 0;
    if (month < 3) year -= 1;
    return (year + year / 4 - year / 100 + year / 400 + kOffsets[month - 1] + day) % 7;
}

bool isValidDate(const Date& date) {
    if (date.year < 1) return false;
    if (date.month < 1 || date.month > 12) return false;
    int dim = daysInMonth(date.year, date.month);
    return date.day >= 1 && date.day <= dim;
}

Date clampDate(const Date& date) {
    Date result = date;
    if (result.year < 1) result.year = 1;
    result.month = std::clamp(result.month, 1, 12);
    int dim = daysInMonth(result.year, result.month);
    result.day = std::clamp(result.day, 1, dim);
    return result;
}

int compareDate(const Date& a, const Date& b) {
    if (a.year != b.year) return (a.year < b.year) ? -1 : 1;
    if (a.month != b.month) return (a.month < b.month) ? -1 : 1;
    if (a.day != b.day) return (a.day < b.day) ? -1 : 1;
    return 0;
}

bool isBefore(const Date& a, const Date& b) {
    return compareDate(a, b) < 0;
}

bool isAfter(const Date& a, const Date& b) {
    return compareDate(a, b) > 0;
}

bool isWithinRange(const Date& date, const Date& minDate, const Date& maxDate) {
    if (isValidDate(minDate) && compareDate(date, minDate) < 0) return false;
    if (isValidDate(maxDate) && compareDate(date, maxDate) > 0) return false;
    return true;
}

std::string formatDate(const Date& date, DateFormat format) {
    char buffer[32];
    switch (format) {
    case DateFormat::MDY:
        std::snprintf(buffer, sizeof(buffer), "%02d/%02d/%04d", date.month, date.day, date.year);
        break;
    case DateFormat::DMY:
        std::snprintf(buffer, sizeof(buffer), "%02d/%02d/%04d", date.day, date.month, date.year);
        break;
    case DateFormat::ISO:
    default:
        std::snprintf(buffer, sizeof(buffer), "%04d-%02d-%02d", date.year, date.month, date.day);
        break;
    }
    return buffer;
}

} // namespace date_utils

//=============================================================================
// DatePicker State
//=============================================================================

struct DatePickerState {
    bool isOpen = false;
    int displayYear = 0;
    int displayMonth = 0;
    int hoveredCell = -1;
};

struct CalendarCell {
    Date date{};
    bool inCurrentMonth = false;
    bool visible = false;
    bool inRange = true;
};

static std::unordered_map<WidgetId, DatePickerState> s_datePickerStates;

//=============================================================================
// Helpers
//=============================================================================

static Date getTodayLocal() {
    std::time_t t = std::time(nullptr);
    std::tm localTime{};
#ifdef _WIN32
    localtime_s(&localTime, &t);
#else
    localTime = *std::localtime(&t);
#endif
    return Date{localTime.tm_year + 1900, localTime.tm_mon + 1, localTime.tm_mday};
}

static void stepMonth(DatePickerState& state, int delta) {
    int month = state.displayMonth + delta;
    int year = state.displayYear;
    if (month < 1) {
        month = 12;
        year -= 1;
    } else if (month > 12) {
        month = 1;
        year += 1;
    }
    if (year < 1) year = 1;
    state.displayMonth = month;
    state.displayYear = year;
}

static std::array<CalendarCell, 42> buildCells(const DatePickerOptions& options, int year, int month) {
    std::array<CalendarCell, 42> cells{};
    int prevMonth = month == 1 ? 12 : month - 1;
    int nextMonth = month == 12 ? 1 : month + 1;
    int prevYear = month == 1 ? year - 1 : year;
    int nextYear = month == 12 ? year + 1 : year;
    if (prevYear < 1) prevYear = 1;

    int daysInCurrent = date_utils::daysInMonth(year, month);
    int daysInPrev = date_utils::daysInMonth(prevYear, prevMonth);
    int firstDow = date_utils::dayOfWeek(year, month, 1);
    int firstDayOfWeek = options.firstDayOfWeek % 7;
    if (firstDayOfWeek < 0) firstDayOfWeek += 7;
    int offset = (firstDow - firstDayOfWeek + 7) % 7;
    int startDay = 1 - offset;

    for (int i = 0; i < 42; ++i) {
        int dayNum = startDay + i;
        Date cellDate{};
        bool inCurrent = false;
        if (dayNum < 1) {
            cellDate = Date{prevYear, prevMonth, daysInPrev + dayNum};
        } else if (dayNum > daysInCurrent) {
            cellDate = Date{nextYear, nextMonth, dayNum - daysInCurrent};
        } else {
            cellDate = Date{year, month, dayNum};
            inCurrent = true;
        }
        bool visible = inCurrent || options.showOutsideDays;
        bool inRange = date_utils::isWithinRange(cellDate, options.minDate, options.maxDate);
        cells[i] = CalendarCell{cellDate, inCurrent, visible, inRange};
    }

    return cells;
}

//=============================================================================
// DatePicker Implementation
//=============================================================================

bool DatePicker(Context& ctx, std::string_view label, Date& date, const DatePickerOptions& options) {
    auto wc = WidgetContext::make(ctx);
    const Theme& theme = *wc.theme;
    IDrawList& dl = *wc.dl;
    Font* font = wc.font;
    InputState& input = ctx.input();

    bool changed = false;

    Date normalized = date_utils::clampDate(date);
    if (!date_utils::isWithinRange(normalized, options.minDate, options.maxDate)) {
        if (date_utils::isValidDate(options.minDate) &&
            date_utils::compareDate(normalized, options.minDate) < 0) {
            normalized = options.minDate;
        }
        if (date_utils::isValidDate(options.maxDate) &&
            date_utils::compareDate(normalized, options.maxDate) > 0) {
            normalized = options.maxDate;
        }
    }
    if (normalized != date) {
        date = normalized;
        changed = true;
    }

    WidgetId id = ctx.makeId(label);
    DatePickerState& state = s_datePickerStates[id];
    if (state.displayYear == 0 || state.displayMonth == 0 || !state.isOpen) {
        state.displayYear = date.year;
        state.displayMonth = date.month;
    }

    float width = options.style.width > 0 ? options.style.width : 160.0f;
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
        if (state.isOpen) {
            state.displayYear = date.year;
            state.displayMonth = date.month;
        }
    }

    // Layout for popup (used for hit-testing and rendering)
    float padding = theme.metrics.paddingSmall;
    float headerHeight = font ? font->lineHeight() + padding * 2.0f : 24.0f;
    float weekdayHeight = options.showWeekdayHeader ? headerHeight : 0.0f;
    float cellSize = options.cellSize > 0.0f ? options.cellSize : 28.0f;
    float gridWidth = cellSize * 7.0f;
    float gridHeight = cellSize * 6.0f;
    float footerHeight = options.showTodayButton ? (theme.metrics.inputHeight + padding) : 0.0f;
    float popupWidth = options.popupWidth > 0.0f ? options.popupWidth : std::max(width, gridWidth + padding * 2.0f);
    float popupHeight = padding * 2.0f + headerHeight + weekdayHeight + gridHeight + footerHeight;
    Rect popupRect(inputRect.x(), inputRect.bottom() + 2.0f, popupWidth, popupHeight);

    if (state.isOpen && input.isMousePressed(MouseButton::Left)) {
        if (!inputRect.contains(input.mousePos()) && !popupRect.contains(input.mousePos())) {
            state.isOpen = false;
        }
    }

    std::array<CalendarCell, 42> cells = buildCells(options, state.displayYear, state.displayMonth);

    if (state.isOpen) {
        if (popupRect.contains(input.mousePos())) {
            input.consumeMouse();
        }

        Rect headerRect(popupRect.x() + padding, popupRect.y() + padding,
                        popupWidth - padding * 2.0f, headerHeight);
        float navSize = headerHeight - padding;
        Rect prevRect(headerRect.x(), headerRect.y() + (headerHeight - navSize) * 0.5f, navSize, navSize);
        Rect nextRect(headerRect.right() - navSize, headerRect.y() + (headerHeight - navSize) * 0.5f, navSize, navSize);
        WidgetId prevId = combineIds(id, hashString("prev"));
        WidgetId nextId = combineIds(id, hashString("next"));
        WidgetInteraction prevInteract = handleWidgetInteraction(ctx, prevId, prevRect, false);
        WidgetInteraction nextInteract = handleWidgetInteraction(ctx, nextId, nextRect, false);

        if (!options.disabled) {
            if (prevInteract.clicked) {
                stepMonth(state, -1);
            }
            if (nextInteract.clicked) {
                stepMonth(state, 1);
            }
        }

        float gridX = popupRect.x() + (popupWidth - gridWidth) * 0.5f;
        float gridY = popupRect.y() + padding + headerHeight + weekdayHeight;
        Rect gridRect(gridX, gridY, gridWidth, gridHeight);

        // Rebuild cells if month changed from navigation
        cells = buildCells(options, state.displayYear, state.displayMonth);

        state.hoveredCell = -1;
        if (gridRect.contains(input.mousePos())) {
            int col = static_cast<int>((input.mousePos().x - gridRect.x()) / cellSize);
            int row = static_cast<int>((input.mousePos().y - gridRect.y()) / cellSize);
            if (col >= 0 && col < 7 && row >= 0 && row < 6) {
                int idx = row * 7 + col;
                if (idx >= 0 && idx < static_cast<int>(cells.size())) {
                    state.hoveredCell = idx;
                    const CalendarCell& cell = cells[idx];
                    if (!options.disabled && cell.visible && cell.inRange &&
                        input.isMousePressed(MouseButton::Left)) {
                        date = cell.date;
                        changed = true;
                        state.displayYear = date.year;
                        state.displayMonth = date.month;
                        if (options.closeOnSelect) {
                            state.isOpen = false;
                        }
                        cells = buildCells(options, state.displayYear, state.displayMonth);
                    }
                }
            }
        }

        if (options.showTodayButton) {
            float footerY = gridRect.bottom() + padding;
            Rect todayRect(popupRect.x() + padding, footerY, popupWidth - padding * 2.0f,
                           theme.metrics.inputHeight);
            WidgetId todayId = combineIds(id, hashString("today"));
            WidgetInteraction todayInteract = handleWidgetInteraction(ctx, todayId, todayRect, true);
            if (!options.disabled && todayInteract.clicked) {
                Date today = getTodayLocal();
                if (date_utils::isWithinRange(today, options.minDate, options.maxDate)) {
                    date = today;
                    changed = true;
                    state.displayYear = date.year;
                    state.displayMonth = date.month;
                    if (options.closeOnSelect) {
                        state.isOpen = false;
                    }
                    cells = buildCells(options, state.displayYear, state.displayMonth);
                }
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

    // Draw date text
    if (font) {
        std::string dateText = date_utils::formatDate(date, options.format);
        Vec2 textPos(
            inputRect.x() + theme.metrics.paddingSmall,
            inputRect.y() + (height - font->lineHeight()) * 0.5f
        );
        Color textColor = options.disabled ? theme.colors.textDisabled : theme.colors.text;
        dl.pushClipRect(Rect(inputRect.x(), inputRect.y(), inputRect.width() - 20, inputRect.height()));
        dl.addText(font, textPos, dateText, textColor);
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

    // Deferred popup rendering
    if (state.isOpen) {
        Date dateCopy = date;
        int hoveredCell = state.hoveredCell;
        int displayYear = state.displayYear;
        int displayMonth = state.displayMonth;
        int firstDayOfWeek = options.firstDayOfWeek;
        bool showWeekdayHeader = options.showWeekdayHeader;
        bool showTodayButton = options.showTodayButton;
        bool disabled = options.disabled;
        std::array<CalendarCell, 42> cellsCopy = cells;

        ctx.deferRender([=, &ctx]() {
            IDrawList& dl = *ctx.activeDrawList();
            Font* font = ctx.font();
            const Theme& theme = ctx.theme();

            float padding = theme.metrics.paddingSmall;
            float headerHeight = font ? font->lineHeight() + padding * 2.0f : 24.0f;
            float weekdayHeight = showWeekdayHeader ? headerHeight : 0.0f;
            float cellSize = options.cellSize > 0.0f ? options.cellSize : 28.0f;
            float gridWidth = cellSize * 7.0f;
            float gridHeight = cellSize * 6.0f;
            float footerHeight = showTodayButton ? (theme.metrics.inputHeight + padding) : 0.0f;
            float popupWidth = options.popupWidth > 0.0f ? options.popupWidth : std::max(width, gridWidth + padding * 2.0f);
            float popupHeight = padding * 2.0f + headerHeight + weekdayHeight + gridHeight + footerHeight;
            Rect popupRect(inputRect.x(), inputRect.bottom() + 2.0f, popupWidth, popupHeight);

            ctx.addFloatingWindowRect(popupRect);

            dl.addShadow(popupRect, theme.colors.shadow, 8.0f, 4.0f);
            dl.addRectFilled(popupRect, theme.colors.panelBackground, 4.0f);
            dl.addRect(popupRect, theme.colors.border, 4.0f);

            Rect headerRect(popupRect.x() + padding, popupRect.y() + padding,
                            popupWidth - padding * 2.0f, headerHeight);
            float navSize = headerHeight - padding;
            Rect prevRect(headerRect.x(), headerRect.y() + (headerHeight - navSize) * 0.5f, navSize, navSize);
            Rect nextRect(headerRect.right() - navSize, headerRect.y() + (headerHeight - navSize) * 0.5f, navSize, navSize);
            WidgetId prevId = combineIds(id, hashString("prev"));
            WidgetId nextId = combineIds(id, hashString("next"));

            WidgetState prevState = getWidgetState(ctx, prevId);
            WidgetState nextState = getWidgetState(ctx, nextId);
            prevState.disabled = disabled;
            nextState.disabled = disabled;

            Color prevBg = getStateColor(theme.colors.buttonBackground, theme.colors.buttonHover,
                                         theme.colors.buttonActive, prevState);
            Color nextBg = getStateColor(theme.colors.buttonBackground, theme.colors.buttonHover,
                                         theme.colors.buttonActive, nextState);
            dl.addRectFilled(prevRect, prevBg, theme.metrics.borderRadiusSmall);
            dl.addRect(prevRect, theme.colors.border, theme.metrics.borderRadiusSmall);
            dl.addRectFilled(nextRect, nextBg, theme.metrics.borderRadiusSmall);
            dl.addRect(nextRect, theme.colors.border, theme.metrics.borderRadiusSmall);

            Color arrowColor = disabled ? theme.colors.textDisabled : theme.colors.text;
            dl.addTriangleFilled(
                Vec2(prevRect.center().x + 2, prevRect.center().y - 4),
                Vec2(prevRect.center().x + 2, prevRect.center().y + 4),
                Vec2(prevRect.center().x - 3, prevRect.center().y),
                arrowColor
            );
            dl.addTriangleFilled(
                Vec2(nextRect.center().x - 2, nextRect.center().y - 4),
                Vec2(nextRect.center().x - 2, nextRect.center().y + 4),
                Vec2(nextRect.center().x + 3, nextRect.center().y),
                arrowColor
            );

            static const char* kMonthNames[] = {
                "January", "February", "March", "April", "May", "June",
                "July", "August", "September", "October", "November", "December"
            };
            if (font && displayMonth >= 1 && displayMonth <= 12) {
                std::string title = std::string(kMonthNames[displayMonth - 1]) + " " + std::to_string(displayYear);
                Vec2 titleSize = font->measureText(title);
                Vec2 titlePos(
                    headerRect.center().x - titleSize.x * 0.5f,
                    headerRect.y() + (headerHeight - font->lineHeight()) * 0.5f
                );
                dl.addText(font, titlePos, title, theme.colors.text);
            }

            float gridX = popupRect.x() + (popupWidth - gridWidth) * 0.5f;
            float gridY = popupRect.y() + padding + headerHeight + weekdayHeight;
            Rect gridRect(gridX, gridY, gridWidth, gridHeight);

            if (showWeekdayHeader && font) {
                static const char* kWeekdays[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
                int start = firstDayOfWeek % 7;
                if (start < 0) start += 7;
                for (int i = 0; i < 7; ++i) {
                    const char* dayLabel = kWeekdays[(start + i) % 7];
                    Vec2 textSize = font->measureText(dayLabel);
                    float x = gridRect.x() + i * cellSize + (cellSize - textSize.x) * 0.5f;
                    float y = popupRect.y() + padding + headerHeight + (weekdayHeight - font->lineHeight()) * 0.5f;
                    dl.addText(font, Vec2(x, y), dayLabel, theme.colors.textSecondary);
                }
            }

            for (int i = 0; i < 42; ++i) {
                int row = i / 7;
                int col = i % 7;
                Rect cellRect(
                    gridRect.x() + col * cellSize,
                    gridRect.y() + row * cellSize,
                    cellSize,
                    cellSize
                );
                const CalendarCell& cell = cellsCopy[i];
                if (!cell.visible) continue;

                bool selected = cell.date == dateCopy;
                bool hovered = (i == hoveredCell) && cell.visible && cell.inRange && !disabled;
                if (selected) {
                    dl.addRectFilled(cellRect, theme.colors.selection, 4.0f);
                } else if (hovered) {
                    dl.addRectFilled(cellRect, theme.colors.selection.withAlpha((uint8_t)80), 4.0f);
                }

                if (font) {
                    std::string dayText = std::to_string(cell.date.day);
                    Vec2 textSize = font->measureText(dayText);
                    Vec2 textPos(
                        cellRect.x() + (cellRect.width() - textSize.x) * 0.5f,
                        cellRect.y() + (cellRect.height() - font->lineHeight()) * 0.5f
                    );

                    Color textColor = theme.colors.text;
                    if (!cell.inCurrentMonth) textColor = theme.colors.textSecondary;
                    if (!cell.inRange) textColor = theme.colors.textDisabled;
                    if (selected) textColor = theme.colors.selectionText;
                    if (disabled) textColor = theme.colors.textDisabled;

                    dl.addText(font, textPos, dayText, textColor);
                }
            }

            if (showTodayButton) {
                Rect todayRect(popupRect.x() + padding, gridRect.bottom() + padding,
                               popupWidth - padding * 2.0f, theme.metrics.inputHeight);
                WidgetId todayId = combineIds(id, hashString("today"));
                WidgetState todayState = getWidgetState(ctx, todayId);
                todayState.disabled = disabled;

                Color todayBg = getStateColor(theme.colors.buttonBackground, theme.colors.buttonHover,
                                              theme.colors.buttonActive, todayState);
                dl.addRectFilled(todayRect, todayBg, theme.metrics.borderRadiusSmall);
                dl.addRect(todayRect, theme.colors.border, theme.metrics.borderRadiusSmall);

                if (font) {
                    Vec2 textSize = font->measureText("Today");
                    Vec2 textPos(
                        todayRect.center().x - textSize.x * 0.5f,
                        todayRect.y() + (todayRect.height() - font->lineHeight()) * 0.5f
                    );
                    Color textColor = todayState.disabled ? theme.colors.textDisabled : theme.colors.text;
                    dl.addText(font, textPos, "Today", textColor);
                }
            }
        });
    }

    return changed;
}

} // namespace fst
