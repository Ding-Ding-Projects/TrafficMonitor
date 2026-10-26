#pragma once
#include "InputControls.h"
#include "Calendar.Accessibility.h"
#include <functional>
#include <memory>
#include <vector>

// Self-drawn Material 3 month calendar: previous / next icon buttons, month and year dropdowns, a weekday
// row and a 6 x 7 day grid with a selected day, a today marker, a focus ring and a per-day content hook (a short
// secondary text, a colored indicator and an accessible description). No stock month calendar, button or combo
// box is involved. The grid is the UI Automation Calendar element with the Grid and Selection patterns.
//
// Keyboard (the grid is one tab stop; the header buttons and dropdowns are their own): arrows move the focused
// day by one day / one week, Home / End go to the start / end of the week, PageUp / PageDown change the month,
// Ctrl+PageUp / Ctrl+PageDown change the year, Ctrl+Home goes to today, Space and Enter select the focused day
// (Enter also activates it). Moving across a month boundary changes the shown month.
//
// Notifications reach the parent as WM_COMMAND with the control id and one of the codes below. Programmatic
// setters never notify.
namespace NativeMaterial
{
    constexpr UINT CalendarDateChanged = 0x4301;     // the user selected a day; Selected()
    constexpr UINT CalendarMonthChanged = 0x4302;    // the user changed the shown month; VisibleYear(), VisibleMonth()
    constexpr UINT CalendarDateActivated = 0x4303;   // Enter or a second click on the selected day

    struct CalendarDate
    {
        int year{}, month{}, day{};
        bool Valid() const;
        bool operator==(const CalendarDate& o) const { return year == o.year && month == o.month && day == o.day; }
        bool operator!=(const CalendarDate& o) const { return !(*this == o); }
        bool operator<(const CalendarDate& o) const { return year != o.year ? year < o.year : month != o.month ? month < o.month : day < o.day; }
    };
    bool IsLeapYear(int year);
    int DaysInMonth(int year, int month);                         // 0 for a month outside 1..12
    int DayOfWeek(int year, int month, int day);                  // 0 = Sunday .. 6 = Saturday
    CalendarDate AddDays(const CalendarDate& date, int days);
    CalendarDate AddMonths(const CalendarDate& date, int months); // the day is clamped to the target month

    // The 6 x 7 month grid; cells outside the month are 0. firstDayOfWeek is 0 (Sunday) or 1 (Monday).
    struct CalendarMonthLayout
    {
        int year{}, month{}, firstDayOfWeek{}, leading{}, days{};
        int day[CalendarRows][CalendarColumns]{};
        bool Find(int dayNumber, int& row, int& column) const;
    };
    CalendarMonthLayout LayoutMonth(int year, int month, int firstDayOfWeek);

    // What the owner shows for one day. `description` is appended to the accessible name.
    struct CalendarDayContent
    {
        CString secondary;               // short text under the day number (for example a traffic total)
        bool hasIndicator{};
        COLORREF indicator{};            // small bar at the bottom of the cell
        CString description;
    };
    using CalendarDaySource = std::function<void(const CalendarDate& date, CalendarDayContent& content)>;

    // Geometry in 96-dpi units. The container holds the header controls and the grid window (body).
    struct CalendarLayout
    {
        D2D1_RECT_F prev, month, year, next, body;
    };
    CalendarLayout LayoutCalendar(float width, float height);
    // Inside the grid window: the weekday row on top, then six rows of days.
    struct CalendarGridLayout
    {
        D2D1_RECT_F weekdays;
        float top{}, cellWidth{}, cellHeight{};
        D2D1_RECT_F Cell(int row, int column) const;
    };
    CalendarGridLayout LayoutGrid(float width, float height);

    struct CalendarPaintModel
    {
        CalendarMonthLayout month;
        int selectedDay{}, focusDay{}, todayDay{}, hoverDay{};
        std::vector<CString> weekdays;              // seven labels in display order
        std::vector<CalendarDayContent> contents;   // index = day - 1
        bool highlightWeekend{};
    };
    void DrawCalendarGrid(ID2D1RenderTarget*, const Palette&, const CalendarPaintModel&, const InputState&);

    enum class CalendarMove { PreviousDay, NextDay, PreviousWeek, NextWeek, StartOfWeek, EndOfWeek, PreviousMonth, NextMonth, PreviousYear, NextYear, Today };

    // Previous / next month icon button. Notifies the parent with BN_CLICKED.
    class CalendarNavButton final : public InputWindow
    {
    public:
        BOOL Create(const CString& name, bool forward, const RECT& bounds, CWnd* parent, UINT id);
        bool IsForward() const { return m_forward; }
    protected:
        void Paint(ID2D1RenderTarget*, const Palette&, const InputState&) override;
        CSize MinimumLogicalSize() const override { return CSize(40, 40); }
        void FillAccess(InputData&) const override {}
        void PointerReleased(CPoint, bool) override;
        bool KeyPressed(UINT, bool) override;
        bool KeyReleased(UINT) override;
        UINT DialogCode() const override { return DLGC_BUTTON; }
        void PerformAction(InputActionId, int) override;
    private:
        bool m_forward{};
    };

    class Calendar;
    // The focusable day grid. It owns no state: the calendar that created it does.
    class CalendarGrid final : public InputWindow
    {
    public:
        BOOL Create(Calendar* owner, const RECT& bounds, CWnd* parent, UINT id);
    protected:
        void Paint(ID2D1RenderTarget*, const Palette&, const InputState&) override;
        CSize MinimumLogicalSize() const override { return CSize(308, 320); }
        void FillAccess(InputData&) const override {}
        void PointerPressed(CPoint) override;
        void PointerReleased(CPoint, bool) override;
        void PointerMoved(CPoint) override;
        void PointerLeft() override;
        bool KeyPressed(UINT, bool) override;
        bool MouseWheel(int) override;
        UINT DialogCode() const override { return DLGC_WANTARROWS; }
        void PerformAction(InputActionId, int) override;
        LRESULT WindowProc(UINT, WPARAM, LPARAM) override;
    private:
        Calendar* m_owner{};
    };

    class Calendar final : public InputWindow
    {
    public:
        Calendar();
        ~Calendar() override;
        enum ControlId : UINT { IdMonth = 1, IdYear, IdPrevious, IdNext, IdGrid };
        BOOL Create(const CString& name, const RECT& bounds, CWnd* parent, UINT id);

        void SetFirstDayOfWeek(int firstDay);                 // 0 Sunday, 1 Monday
        int GetFirstDayOfWeek() const { return m_firstDay; }
        void SetYearRange(int minimum, int maximum);
        int YearMinimum() const { return m_minYear; }
        int YearMaximum() const { return m_maxYear; }
        void SetToday(const CalendarDate& today);             // the default is the system date
        CalendarDate Today() const { return m_today; }
        void SetWeekendHighlight(bool highlight);
        void SetSurface(COLORREF surface);                    // the surface color of the container and all its parts
        void SetDaySource(CalendarDaySource source);
        void Refresh();                                       // the day source's data changed

        void SetVisibleMonth(int year, int month);            // keeps the focus inside the month
        int VisibleYear() const { return m_year; }
        int VisibleMonth() const { return m_month; }
        void SetSelectedDate(const CalendarDate& date);       // also focuses it and shows its month
        CalendarDate Selected() const { return m_selected; }
        CalendarDate Focused() const { return m_focus; }

        // User-style actions: they notify like the keyboard and the mouse do.
        bool Navigate(CalendarMove move);
        bool SelectFocused(bool activate);
        bool SelectDate(const CalendarDate& date, bool activate);

        const CalendarMonthLayout& Month() const { return m_layout; }
        CalendarDayContent Content(const CalendarDate& date) const;
        CString DateName(const CalendarDate& date) const;     // the accessible name of a day cell
        CRect DayRect(const CalendarDate& date) const;        // grid client pixels; empty when not in the shown month
        CalendarDate DateAt(CPoint gridPoint) const;          // invalid date when the point is not on a day
        CalendarDate HoverDate() const;                       // the day under the pointer, if any
        CalendarGrid& Grid() { return *m_grid; }
        Dropdown& MonthDropdown() { return *m_monthBox; }
        Dropdown& YearDropdown() { return *m_yearBox; }
        CalendarNavButton& PreviousButton() { return *m_previous; }
        CalendarNavButton& NextButton() { return *m_next; }
        // The Grid / Selection root provider bound to the grid window (the caller releases it).
        HRESULT CalendarAutomation(IRawElementProviderSimple** provider) const;
        CalendarPaintModel PaintModel() const;
    protected:
        void Paint(ID2D1RenderTarget*, const Palette&, const InputState&) override {}
        CSize MinimumLogicalSize() const override { return CSize(308, 384); }
        void FillAccess(InputData&) const override {}
        UINT DialogCode() const override { return 0; }
        void DpiChanged() override;
        void AppearanceChanged() override;
        LRESULT WindowProc(UINT, WPARAM, LPARAM) override;
    private:
        friend class CalendarGrid;
        void Rebuild(bool notifyMonth);
        void LayoutChildren();
        void PublishCalendar();
        void ShowFocus(const CalendarDate& date, bool notifyMonth);
        void UpdateHeader();
        CalendarDate Clamp(const CalendarDate& date) const;
        bool InRange(const CalendarDate& date) const;
        bool GridKey(UINT key, bool control);
        void GridPress(CPoint);
        void GridRelease(CPoint);
        void GridHover(CPoint);
        void GridPerform(int cell);
        bool GridWheel(int delta);

        std::unique_ptr<Dropdown> m_monthBox, m_yearBox;
        std::unique_ptr<CalendarNavButton> m_previous, m_next;
        std::unique_ptr<CalendarGrid> m_grid;
        CalendarDaySource m_source;
        std::vector<CalendarDayContent> m_contents;
        CalendarMonthLayout m_layout;
        CalendarDate m_today, m_selected, m_focus, m_clickDate, m_pressDate;
        int m_year{}, m_month{}, m_firstDay{}, m_minYear{ 1970 }, m_maxYear{ 2100 }, m_hoverDay{};
        ULONGLONG m_clickTime{};
        bool m_weekend{}, m_building{};
        std::shared_ptr<CalendarAccess> m_calendar;
    };
}
