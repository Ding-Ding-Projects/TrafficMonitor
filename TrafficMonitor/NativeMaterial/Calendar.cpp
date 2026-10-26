#include "../stdafx.h"
#include "Calendar.h"
#include "PickerDraw.h"
#include "InputControls.Internal.h"
#include <algorithm>

namespace NativeMaterial
{
    using PickerDraw::Painter;
    using PickerDraw::TextStyle;

    namespace
    {
        CString Words(const wchar_t* english, const wchar_t* cantonese) { return NativePreferences::InlinePair(english, cantonese); }

        constexpr float HeaderHeight = 64.f, WeekdayHeight = 32.f, MinimumRow = 48.f, NavSize = 40.f, YearWidth = 104.f, NumberRadius = 14.f;

        const wchar_t* const WeekdayEnglish[7] = { L"Sunday", L"Monday", L"Tuesday", L"Wednesday", L"Thursday", L"Friday", L"Saturday" };
        const wchar_t* const WeekdayShort[7] = { L"S", L"M", L"T", L"W", L"T", L"F", L"S" };
        const wchar_t* const WeekdayChinese[7] = { L"日", L"一", L"二", L"三", L"四", L"五", L"六" };
        const wchar_t* const MonthEnglish[12] = { L"January", L"February", L"March", L"April", L"May", L"June", L"July", L"August",
            L"September", L"October", L"November", L"December" };

        // Days since 1970-01-01 for a civil date (proleptic Gregorian) and back.
        long long DaysFromCivil(int year, int month, int day)
        {
            year -= month <= 2;
            const long long era = (year >= 0 ? year : year - 399) / 400;
            const long long yoe = year - era * 400;
            const long long doy = (153 * (month + (month > 2 ? -3 : 9)) + 2) / 5 + day - 1;
            const long long doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
            return era * 146097 + doe - 719468;
        }
        CalendarDate CivilFromDays(long long z)
        {
            z += 719468;
            const long long era = (z >= 0 ? z : z - 146096) / 146097;
            const long long doe = z - era * 146097;
            const long long yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
            const long long y = yoe + era * 400;
            const long long doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
            const long long mp = (5 * doy + 2) / 153;
            const int day = static_cast<int>(doy - (153 * mp + 2) / 5 + 1);
            const int month = static_cast<int>(mp < 10 ? mp + 3 : mp - 9);
            return CalendarDate{ static_cast<int>(y + (month <= 2)), month, day };
        }
    }

    // ------------------------------------------------------------------ date math
    bool IsLeapYear(int year)
    {
        return year % 4 == 0 && (year % 100 != 0 || year % 400 == 0);
    }
    int DaysInMonth(int year, int month)
    {
        static const int days[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
        if (month < 1 || month > 12) return 0;
        return month == 2 && IsLeapYear(year) ? 29 : days[month - 1];
    }
    bool CalendarDate::Valid() const
    {
        return year >= 1 && year <= 9999 && month >= 1 && month <= 12 && day >= 1 && day <= DaysInMonth(year, month);
    }
    int DayOfWeek(int year, int month, int day)
    {
        const long long days = DaysFromCivil(year, month, day);
        return static_cast<int>(((days % 7) + 7 + 4) % 7);          // 1970-01-01 was a Thursday
    }
    CalendarDate AddDays(const CalendarDate& date, int days)
    {
        return CivilFromDays(DaysFromCivil(date.year, date.month, date.day) + days);
    }
    CalendarDate AddMonths(const CalendarDate& date, int months)
    {
        const int index = date.year * 12 + (date.month - 1) + months;
        const int year = index >= 0 ? index / 12 : -((-index + 11) / 12), month = index - year * 12 + 1;
        return CalendarDate{ year, month, (std::min)(date.day, DaysInMonth(year, month)) };
    }

    CalendarMonthLayout LayoutMonth(int year, int month, int firstDayOfWeek)
    {
        CalendarMonthLayout layout;
        layout.year = year;
        layout.month = month;
        layout.firstDayOfWeek = firstDayOfWeek ? 1 : 0;
        layout.days = DaysInMonth(year, month);
        if (layout.days == 0) return layout;
        layout.leading = (DayOfWeek(year, month, 1) - layout.firstDayOfWeek + 7) % 7;
        for (int d = 1; d <= layout.days; ++d)
        {
            const int index = layout.leading + d - 1;
            layout.day[index / CalendarColumns][index % CalendarColumns] = d;
        }
        return layout;
    }
    bool CalendarMonthLayout::Find(int dayNumber, int& row, int& column) const
    {
        if (dayNumber < 1 || dayNumber > days) return false;
        const int index = leading + dayNumber - 1;
        row = index / CalendarColumns;
        column = index % CalendarColumns;
        return true;
    }

    // ------------------------------------------------------------------ geometry and drawing
    CalendarLayout LayoutCalendar(float width, float height)
    {
        CalendarLayout l{};
        const float month = NavSize + 8.f, yearLeft = (std::max)(month + 40.f, width - NavSize - 8.f - YearWidth);
        l.prev = D2D1::RectF(0, (HeaderHeight - NavSize) / 2, NavSize, (HeaderHeight + NavSize) / 2);
        l.next = D2D1::RectF(width - NavSize, l.prev.top, width, l.prev.bottom);
        l.year = D2D1::RectF(yearLeft, 0, (std::max)(yearLeft + 40.f, width - NavSize - 8.f), HeaderHeight);
        l.month = D2D1::RectF(month, 0, (std::max)(month + 40.f, yearLeft - 8.f), HeaderHeight);
        l.body = D2D1::RectF(0, HeaderHeight, width, (std::max)(HeaderHeight + WeekdayHeight + 6 * MinimumRow, height));
        return l;
    }
    CalendarGridLayout LayoutGrid(float width, float height)
    {
        CalendarGridLayout l{};
        l.weekdays = D2D1::RectF(0, 0, width, WeekdayHeight);
        l.top = WeekdayHeight;
        l.cellWidth = width / CalendarColumns;
        l.cellHeight = (std::max)(MinimumRow, (height - WeekdayHeight) / CalendarRows);
        return l;
    }
    D2D1_RECT_F CalendarGridLayout::Cell(int row, int column) const
    {
        return D2D1::RectF(column * cellWidth, top + row * cellHeight, (column + 1) * cellWidth, top + (row + 1) * cellHeight);
    }

    void DrawCalendarGrid(ID2D1RenderTarget* target, const Palette& c, const CalendarPaintModel& model, const InputState& state)
    {
        Painter k(target, c);
        if (!k.Ok()) return;
        const auto size = target->GetSize();
        const auto layout = LayoutGrid(size.width, size.height);
        TextStyle head;
        head.size = 12.f;
        head.weight = DWRITE_FONT_WEIGHT_MEDIUM;
        head.align = DWRITE_TEXT_ALIGNMENT_CENTER;
        for (int column = 0; column < CalendarColumns; ++column)
        {
            const int dow = (model.month.firstDayOfWeek + column) % 7;
            const bool weekend = model.highlightWeekend && (dow == 0 || dow == 6);
            const CString label = column < static_cast<int>(model.weekdays.size()) ? model.weekdays[column] : CString();
            k.Text(label, D2D1::RectF(column * layout.cellWidth, 0, (column + 1) * layout.cellWidth, WeekdayHeight), head, weekend ? c.error : c.onSurfaceVariant);
        }
        for (int row = 0; row < CalendarRows; ++row)
        {
            for (int column = 0; column < CalendarColumns; ++column)
            {
                const int day = model.month.day[row][column];
                if (!day) continue;
                const auto cell = layout.Cell(row, column);
                const float cx = (cell.left + cell.right) / 2, cy = cell.top + 17.f;
                const float radius = (std::min)(NumberRadius, layout.cellWidth / 2 - 1.f);
                const bool selected = day == model.selectedDay, today = day == model.todayDay, focus = day == model.focusDay, hover = day == model.hoverDay;
                const int dow = (model.month.firstDayOfWeek + column) % 7;
                const bool weekend = model.highlightWeekend && (dow == 0 || dow == 6);
                const auto center = D2D1::Point2F(cx, cy);
                if (hover && !selected && state.enabled)
                    target->FillEllipse(D2D1::Ellipse(center, radius, radius), k(k.Layer(c.onSurface, .08f)));
                if (selected) target->FillEllipse(D2D1::Ellipse(center, radius, radius), k(c.primary));
                else if (today) target->DrawEllipse(D2D1::Ellipse(center, radius - .75f, radius - .75f), k(c.primary), 1.5f);
                TextStyle number;
                number.size = 14.f;
                number.align = DWRITE_TEXT_ALIGNMENT_CENTER;
                number.weight = (selected || today) ? DWRITE_FONT_WEIGHT_SEMI_BOLD : DWRITE_FONT_WEIGHT_NORMAL;
                CString text;
                text.Format(L"%d", day);
                k.Text(text, D2D1::RectF(cx - radius, cy - 10, cx + radius, cy + 10), number,
                    selected ? c.onPrimary : weekend ? c.error : c.onSurface);
                if (day - 1 < static_cast<int>(model.contents.size()))
                {
                    const auto& content = model.contents[day - 1];
                    if (!content.secondary.IsEmpty())
                    {
                        TextStyle note;
                        note.size = 10.f;
                        note.align = DWRITE_TEXT_ALIGNMENT_CENTER;
                        k.Text(content.secondary, D2D1::RectF(cell.left + 1, cell.top + 31.f, cell.right - 1, cell.top + 43.f), note, c.onSurfaceVariant);
                    }
                    if (content.hasIndicator)
                        k.FillRounded(D2D1::RectF(cx - 10.f, cell.bottom - 5.f, cx + 10.f, cell.bottom - 2.f), 1.5f, PickerDraw::FromRef(content.indicator));
                }
                if (state.focused && focus)
                    target->DrawEllipse(D2D1::Ellipse(center, radius + 3.f, radius + 3.f), k(c.primary), 2.f);
            }
        }
    }

    // ------------------------------------------------------------------ CalendarNavButton
    BOOL CalendarNavButton::Create(const CString& name, bool forward, const RECT& bounds, CWnd* parent, UINT id)
    {
        m_forward = forward;
        return CreateInput(InputKind::Button, name, WS_CHILD | WS_VISIBLE | WS_TABSTOP, 0, bounds, parent, id);
    }
    void CalendarNavButton::Paint(ID2D1RenderTarget* target, const Palette& c, const InputState& state)
    {
        Painter k(target, c);
        if (!k.Ok()) return;
        const auto size = target->GetSize();
        const auto center = D2D1::Point2F(size.width / 2, size.height / 2);
        if (state.enabled && (state.hover || state.pressed || state.focused))
            target->FillEllipse(D2D1::Ellipse(center, InputMetrics::StateLayer / 2, InputMetrics::StateLayer / 2), k(k.Layer(c.onSurface, state.pressed || state.focused ? .12f : .08f)));
        const auto ink = state.enabled ? c.onSurfaceVariant : k.Dim();
        const float direction = m_forward ? 1.f : -1.f;
        target->DrawLine(D2D1::Point2F(center.x - 3.f * direction, center.y - 6.f), D2D1::Point2F(center.x + 3.f * direction, center.y), k(ink), 2.f);
        target->DrawLine(D2D1::Point2F(center.x + 3.f * direction, center.y), D2D1::Point2F(center.x - 3.f * direction, center.y + 6.f), k(ink), 2.f);
        if (state.focused) target->DrawEllipse(D2D1::Ellipse(center, InputMetrics::StateLayer / 2 + 1.f, InputMetrics::StateLayer / 2 + 1.f), k(c.primary), 2.f);
    }
    void CalendarNavButton::PointerReleased(CPoint, bool inside) { if (inside) Notify(BN_CLICKED); }
    bool CalendarNavButton::KeyPressed(UINT key, bool) { if (key != VK_SPACE && key != VK_RETURN) return false; m_pressed = true; return true; }
    bool CalendarNavButton::KeyReleased(UINT key)
    {
        if ((key != VK_SPACE && key != VK_RETURN) || !m_pressed) return false;
        m_pressed = false;
        Notify(BN_CLICKED);
        return true;
    }
    void CalendarNavButton::PerformAction(InputActionId action, int) { if (action == InputActionId::Toggle) Notify(BN_CLICKED); }

    // ------------------------------------------------------------------ CalendarGrid
    BOOL CalendarGrid::Create(Calendar* owner, const RECT& bounds, CWnd* parent, UINT id)
    {
        m_owner = owner;
        return CreateInput(InputKind::Calendar, Words(L"Days", L"日子"), WS_CHILD | WS_VISIBLE | WS_TABSTOP, 0, bounds, parent, id);
    }
    void CalendarGrid::Paint(ID2D1RenderTarget* target, const Palette& palette, const InputState& state)
    {
        if (m_owner) DrawCalendarGrid(target, palette, m_owner->PaintModel(), state);
    }
    void CalendarGrid::PointerPressed(CPoint point) { if (m_owner) m_owner->GridPress(point); }
    void CalendarGrid::PointerReleased(CPoint point, bool inside) { if (m_owner && inside) m_owner->GridRelease(point); }
    void CalendarGrid::PointerMoved(CPoint point) { if (m_owner) m_owner->GridHover(point); }
    void CalendarGrid::PointerLeft() { if (m_owner) m_owner->GridHover(CPoint(-1, -1)); }
    bool CalendarGrid::KeyPressed(UINT key, bool alt)
    {
        if (alt || !m_owner) return false;
        return m_owner->GridKey(key, (GetKeyState(VK_CONTROL) & 0x8000) != 0);
    }
    bool CalendarGrid::MouseWheel(int delta) { return m_owner && m_owner->GridWheel(delta); }
    void CalendarGrid::PerformAction(InputActionId action, int value) { if (m_owner && action == InputActionId::Select) m_owner->GridPerform(value); }
    LRESULT CalendarGrid::WindowProc(UINT message, WPARAM w, LPARAM l)
    {
        if (message == WM_GETOBJECT && static_cast<LONG>(l) == static_cast<LONG>(UiaRootObjectId) && m_owner && m_owner->m_calendar)
        {
            auto* provider = new CalendarProvider(m_owner->m_calendar);
            const LRESULT result = UiaReturnRawElementProvider(m_hWnd, w, l, provider);
            provider->Release();
            return result;
        }
        return InputWindow::WindowProc(message, w, l);
    }

    // ------------------------------------------------------------------ Calendar
    Calendar::Calendar()
    {
        SYSTEMTIME now{};
        GetLocalTime(&now);
        m_today = CalendarDate{ now.wYear, now.wMonth, now.wDay };
        m_focus = m_today;
        m_year = m_today.year;
        m_month = m_today.month;
    }
    Calendar::~Calendar() = default;

    BOOL Calendar::Create(const CString& name, const RECT& bounds, CWnd* parent, UINT id)
    {
        if (!CreateInput(InputKind::Calendar, name, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN, WS_EX_CONTROLPARENT, bounds, parent, id)) return FALSE;
        const CRect none(0, 0, 10, 10);
        m_monthBox = std::make_unique<Dropdown>();
        m_yearBox = std::make_unique<Dropdown>();
        m_previous = std::make_unique<CalendarNavButton>();
        m_next = std::make_unique<CalendarNavButton>();
        m_grid = std::make_unique<CalendarGrid>();
        m_monthBox->Create(Words(L"Month", L"月"), none, this, IdMonth);
        m_yearBox->Create(Words(L"Year", L"年"), none, this, IdYear);
        m_previous->Create(Words(L"Previous month", L"上個月"), false, none, this, IdPrevious);
        m_next->Create(Words(L"Next month", L"下個月"), true, none, this, IdNext);
        m_grid->Create(this, none, this, IdGrid);
        std::vector<CString> months;
        for (int i = 0; i < 12; ++i)
        {
            CString zh;
            zh.Format(L"%d月", i + 1);
            months.push_back(NativePreferences::InlinePair(MonthEnglish[i], zh));
        }
        m_monthBox->SetItems(months);
        m_calendar = std::make_shared<CalendarAccess>();
        {
            std::lock_guard<std::mutex> lock(m_calendar->mutex);
            m_calendar->window = m_grid->GetSafeHwnd();
        }
        SetYearRange(m_minYear, m_maxYear);
        AppearanceChanged();
        LayoutChildren();
        return TRUE;
    }

    void Calendar::SetSurface(COLORREF surface)
    {
        SetSurfaceColor(surface);
        for (InputWindow* part : std::initializer_list<InputWindow*>{ m_monthBox.get(), m_yearBox.get(), m_previous.get(), m_next.get(), m_grid.get() })
            if (part) part->SetSurfaceColor(surface);
    }

    void Calendar::AppearanceChanged()
    {
        for (InputWindow* part : std::initializer_list<InputWindow*>{ m_monthBox.get(), m_yearBox.get(), m_previous.get(), m_next.get(), m_grid.get() })
            if (part && part->GetSafeHwnd()) part->SetAppearance(GetAppearance());
    }
    void Calendar::DpiChanged() { LayoutChildren(); }

    void Calendar::LayoutChildren()
    {
        if (!m_grid || !m_grid->GetSafeHwnd()) return;
        CRect client;
        GetClientRect(client);
        const UINT dpi = Dpi();
        const auto l = LayoutCalendar(Dip(client.Width()), Dip(client.Height()));
        auto px = [&](D2D1_RECT_F r) { return CRect(DipToPx(r.left, dpi), DipToPx(r.top, dpi), DipToPx(r.right, dpi), DipToPx(r.bottom, dpi)); };
        m_previous->MoveWindow(px(l.prev));
        m_next->MoveWindow(px(l.next));
        m_monthBox->MoveWindow(px(l.month));
        m_yearBox->MoveWindow(px(l.year));
        m_grid->MoveWindow(px(l.body));
    }

    void Calendar::SetFirstDayOfWeek(int firstDay) { m_firstDay = firstDay ? 1 : 0; Rebuild(false); }

    void Calendar::SetYearRange(int minimum, int maximum)
    {
        minimum = (std::max)(1, minimum);
        maximum = (std::min)(9999, (std::max)(minimum, maximum));
        m_minYear = minimum;
        m_maxYear = maximum;
        if (m_yearBox && m_yearBox->GetSafeHwnd())
        {
            std::vector<CString> years;
            for (int y = minimum; y <= maximum; ++y)
            {
                CString text;
                text.Format(L"%d", y);
                years.push_back(text);
            }
            m_yearBox->SetItems(years);
        }
        const CalendarDate clamped = Clamp(CalendarDate{ m_year, m_month, (std::max)(1, m_focus.day) });
        m_year = clamped.year;
        m_month = clamped.month;
        m_focus = clamped;
        Rebuild(false);
    }

    void Calendar::SetToday(const CalendarDate& today)
    {
        if (!today.Valid()) return;
        m_today = today;
        if (m_grid && m_grid->GetSafeHwnd()) m_grid->Invalidate(FALSE);
    }
    void Calendar::SetWeekendHighlight(bool highlight) { m_weekend = highlight; if (m_grid && m_grid->GetSafeHwnd()) m_grid->Invalidate(FALSE); }
    void Calendar::SetDaySource(CalendarDaySource source) { m_source = std::move(source); Rebuild(false); }
    void Calendar::Refresh() { Rebuild(false); }

    CalendarDate Calendar::Clamp(const CalendarDate& date) const
    {
        CalendarDate result = date;
        if (result.year < m_minYear) return CalendarDate{ m_minYear, 1, 1 };
        if (result.year > m_maxYear) return CalendarDate{ m_maxYear, 12, DaysInMonth(m_maxYear, 12) };
        result.month = (std::max)(1, (std::min)(12, result.month));
        result.day = (std::max)(1, (std::min)(DaysInMonth(result.year, result.month), result.day));
        return result;
    }
    bool Calendar::InRange(const CalendarDate& date) const
    {
        return date.Valid() && date.year >= m_minYear && date.year <= m_maxYear;
    }

    void Calendar::SetVisibleMonth(int year, int month)
    {
        const CalendarDate target = Clamp(CalendarDate{ year, month, (std::max)(1, m_focus.day) });
        m_year = target.year;
        m_month = target.month;
        m_focus = target;
        Rebuild(false);
    }

    void Calendar::SetSelectedDate(const CalendarDate& date)
    {
        if (!InRange(date)) return;
        m_selected = date;
        m_focus = date;
        m_year = date.year;
        m_month = date.month;
        Rebuild(false);
    }

    void Calendar::Rebuild(bool notifyMonth)
    {
        m_layout = LayoutMonth(m_year, m_month, m_firstDay);
        m_contents.assign(32, CalendarDayContent{});
        if (m_source)
            for (int d = 1; d <= m_layout.days; ++d) m_source(CalendarDate{ m_year, m_month, d }, m_contents[d - 1]);
        m_hoverDay = 0;
        UpdateHeader();
        PublishCalendar();
        if (m_grid && m_grid->GetSafeHwnd()) m_grid->Invalidate(FALSE);
        if (notifyMonth) Notify(CalendarMonthChanged);
    }

    void Calendar::UpdateHeader()
    {
        if (!m_monthBox || !m_monthBox->GetSafeHwnd()) return;
        m_building = true;
        m_monthBox->SetCurSel(m_month - 1);
        m_yearBox->SetCurSel(m_year - m_minYear);
        m_previous->EnableWindow(!(m_year <= m_minYear && m_month <= 1));
        m_next->EnableWindow(!(m_year >= m_maxYear && m_month >= 12));
        m_building = false;
    }

    CString Calendar::DateName(const CalendarDate& date) const
    {
        CString english, cantonese;
        const int dow = DayOfWeek(date.year, date.month, date.day);
        english.Format(L"%s, %d %s %d", WeekdayEnglish[dow], date.day, MonthEnglish[(std::max)(1, (std::min)(12, date.month)) - 1], date.year);
        cantonese.Format(L"%d年%d月%d日 星期%s", date.year, date.month, date.day, WeekdayChinese[dow]);
        return NativePreferences::InlinePair(english, cantonese);
    }

    CalendarDayContent Calendar::Content(const CalendarDate& date) const
    {
        if (date.year != m_year || date.month != m_month || date.day < 1 || date.day > m_layout.days) return CalendarDayContent{};
        return m_contents[date.day - 1];
    }

    void Calendar::PublishCalendar()
    {
        if (!m_calendar) return;
        std::lock_guard<std::mutex> lock(m_calendar->mutex);
        CString name;
        if (m_hWnd) GetWindowText(name);
        m_calendar->name = name.GetString();
        m_calendar->enabled = !m_hWnd || IsWindowEnabled();
        m_calendar->selectedCell = -1;
        for (int row = 0; row < CalendarRows; ++row)
        {
            for (int column = 0; column < CalendarColumns; ++column)
            {
                auto& cell = m_calendar->cells[row][column];
                cell.day = m_layout.day[row][column];
                cell.name.clear();
                if (!cell.day) continue;
                const CalendarDate date{ m_year, m_month, cell.day };
                CString text = DateName(date);
                const auto& content = m_contents[cell.day - 1];
                if (!content.description.IsEmpty()) text += L", " + content.description;
                cell.name = text.GetString();
                if (date == m_selected) m_calendar->selectedCell = row * CalendarColumns + column;
            }
        }
    }

    HRESULT Calendar::CalendarAutomation(IRawElementProviderSimple** provider) const
    {
        if (!provider) return E_POINTER;
        *provider = nullptr;
        if (!m_calendar) return E_UNEXPECTED;
        *provider = new CalendarProvider(m_calendar);
        return S_OK;
    }

    CalendarPaintModel Calendar::PaintModel() const
    {
        CalendarPaintModel model;
        model.month = m_layout;
        model.selectedDay = (m_selected.year == m_year && m_selected.month == m_month) ? m_selected.day : 0;
        model.focusDay = (m_focus.year == m_year && m_focus.month == m_month) ? m_focus.day : 0;
        model.todayDay = (m_today.year == m_year && m_today.month == m_month) ? m_today.day : 0;
        model.hoverDay = m_hoverDay;
        for (int column = 0; column < CalendarColumns; ++column)
        {
            const int dow = (m_firstDay + column) % 7;
            model.weekdays.push_back(NativePreferences::InlinePair(WeekdayShort[dow], WeekdayChinese[dow]));
        }
        model.contents = m_contents;
        model.highlightWeekend = m_weekend;
        return model;
    }

    void Calendar::ShowFocus(const CalendarDate& requested, bool notifyMonth)
    {
        const CalendarDate date = Clamp(requested);
        const bool monthChanged = date.year != m_year || date.month != m_month;
        m_focus = date;
        m_year = date.year;
        m_month = date.month;
        if (monthChanged) Rebuild(notifyMonth);
        else if (m_grid && m_grid->GetSafeHwnd()) m_grid->Invalidate(FALSE);
    }

    bool Calendar::Navigate(CalendarMove move)
    {
        const CalendarDate from = m_focus.Valid() ? m_focus : m_today;
        const int offset = (DayOfWeek(from.year, from.month, from.day) - m_firstDay + 7) % 7;
        CalendarDate target = from;
        switch (move)
        {
        case CalendarMove::PreviousDay: target = AddDays(from, -1); break;
        case CalendarMove::NextDay: target = AddDays(from, 1); break;
        case CalendarMove::PreviousWeek: target = AddDays(from, -7); break;
        case CalendarMove::NextWeek: target = AddDays(from, 7); break;
        case CalendarMove::StartOfWeek: target = AddDays(from, -offset); break;
        case CalendarMove::EndOfWeek: target = AddDays(from, 6 - offset); break;
        case CalendarMove::PreviousMonth: target = AddMonths(from, -1); break;
        case CalendarMove::NextMonth: target = AddMonths(from, 1); break;
        case CalendarMove::PreviousYear: target = AddMonths(from, -12); break;
        case CalendarMove::NextYear: target = AddMonths(from, 12); break;
        case CalendarMove::Today: target = m_today; break;
        }
        if (!InRange(target) || target == m_focus) return false;
        ShowFocus(target, true);
        return true;
    }

    bool Calendar::SelectDate(const CalendarDate& date, bool activate)
    {
        if (!InRange(date)) return false;
        ShowFocus(date, true);
        const bool changed = date != m_selected;
        m_selected = date;
        PublishCalendar();
        if (m_grid && m_grid->GetSafeHwnd()) m_grid->Invalidate(FALSE);
        if (changed) Notify(CalendarDateChanged);
        if (activate) Notify(CalendarDateActivated);
        return true;
    }
    bool Calendar::SelectFocused(bool activate) { return SelectDate(m_focus.Valid() ? m_focus : m_today, activate); }

    CalendarDate Calendar::DateAt(CPoint point) const
    {
        if (!m_grid || !m_grid->GetSafeHwnd()) return CalendarDate{};
        CRect client;
        m_grid->GetClientRect(client);
        const float scale = 96.f / static_cast<float>(GetDpiForWindow(m_grid->GetSafeHwnd()));
        const auto layout = LayoutGrid(client.Width() * scale, client.Height() * scale);
        const float x = point.x * scale, y = point.y * scale - layout.top;
        if (x < 0 || y < 0) return CalendarDate{};
        const int column = static_cast<int>(x / layout.cellWidth), row = static_cast<int>(y / layout.cellHeight);
        if (column >= CalendarColumns || row >= CalendarRows || !m_layout.day[row][column]) return CalendarDate{};
        return CalendarDate{ m_year, m_month, m_layout.day[row][column] };
    }
    CalendarDate Calendar::HoverDate() const
    {
        return m_hoverDay ? CalendarDate{ m_year, m_month, m_hoverDay } : CalendarDate{};
    }
    CRect Calendar::DayRect(const CalendarDate& date) const
    {
        int row = 0, column = 0;
        if (!m_grid || !m_grid->GetSafeHwnd() || date.year != m_year || date.month != m_month || !m_layout.Find(date.day, row, column)) return CRect();
        CRect client;
        m_grid->GetClientRect(client);
        const float scale = static_cast<float>(GetDpiForWindow(m_grid->GetSafeHwnd())) / 96.f;
        const auto layout = LayoutGrid(client.Width() / scale, client.Height() / scale);
        const auto cell = layout.Cell(row, column);
        return CRect(static_cast<int>(std::lround(cell.left * scale)), static_cast<int>(std::lround(cell.top * scale)),
            static_cast<int>(std::lround(cell.right * scale)), static_cast<int>(std::lround(cell.bottom * scale)));
    }

    bool Calendar::GridKey(UINT key, bool control)
    {
        switch (key)
        {
        case VK_LEFT: Navigate(CalendarMove::PreviousDay); return true;
        case VK_RIGHT: Navigate(CalendarMove::NextDay); return true;
        case VK_UP: Navigate(CalendarMove::PreviousWeek); return true;
        case VK_DOWN: Navigate(CalendarMove::NextWeek); return true;
        case VK_HOME: Navigate(control ? CalendarMove::Today : CalendarMove::StartOfWeek); return true;
        case VK_END: Navigate(CalendarMove::EndOfWeek); return true;
        case VK_PRIOR: Navigate(control ? CalendarMove::PreviousYear : CalendarMove::PreviousMonth); return true;
        case VK_NEXT: Navigate(control ? CalendarMove::NextYear : CalendarMove::NextMonth); return true;
        case VK_SPACE: SelectFocused(false); return true;
        case VK_RETURN: SelectFocused(true); return true;
        default: return false;
        }
    }
    void Calendar::GridPress(CPoint point)
    {
        m_pressDate = DateAt(point);
        if (m_pressDate.Valid()) ShowFocus(m_pressDate, false);
    }
    void Calendar::GridRelease(CPoint point)
    {
        const CalendarDate date = DateAt(point);
        if (!date.Valid() || date != m_pressDate) return;
        const ULONGLONG now = GetTickCount64();
        const bool second = m_clickDate == date && now - m_clickTime <= GetDoubleClickTime();
        SelectDate(date, second);
        m_clickDate = second ? CalendarDate{} : date;
        m_clickTime = now;
    }
    void Calendar::GridHover(CPoint point)
    {
        const int day = DateAt(point).day;
        if (day == m_hoverDay) return;
        m_hoverDay = day;
        if (m_grid && m_grid->GetSafeHwnd()) m_grid->Invalidate(FALSE);
    }
    void Calendar::GridPerform(int cell)
    {
        if (cell < 0 || cell >= CalendarRows * CalendarColumns) return;
        const int day = m_layout.day[cell / CalendarColumns][cell % CalendarColumns];
        if (day) SelectDate(CalendarDate{ m_year, m_month, day }, false);
    }
    bool Calendar::GridWheel(int delta)
    {
        if (!delta) return false;
        Navigate(delta > 0 ? CalendarMove::PreviousMonth : CalendarMove::NextMonth);
        return true;
    }

    LRESULT Calendar::WindowProc(UINT message, WPARAM wparam, LPARAM lparam)
    {
        if (message == WM_COMMAND && lparam)
        {
            const UINT id = LOWORD(wparam), code = HIWORD(wparam);
            if (!m_building && code == CBN_SELCHANGE && (id == IdMonth || id == IdYear))
            {
                const int month = id == IdMonth ? m_monthBox->GetCurSel() + 1 : m_month;
                const int year = id == IdYear ? m_minYear + m_yearBox->GetCurSel() : m_year;
                if (month >= 1 && month <= 12 && year >= m_minYear && year <= m_maxYear)
                    ShowFocus(CalendarDate{ year, month, (std::min)((std::max)(1, m_focus.day), DaysInMonth(year, month)) }, true);
                else UpdateHeader();
                return 0;
            }
            if (code == BN_CLICKED && (id == IdPrevious || id == IdNext))
            {
                Navigate(id == IdPrevious ? CalendarMove::PreviousMonth : CalendarMove::NextMonth);
                return 0;
            }
        }
        if (message == WM_SIZE) { const LRESULT result = InputWindow::WindowProc(message, wparam, lparam); LayoutChildren(); return result; }
        if (message == WM_DESTROY && m_calendar)
        {
            std::lock_guard<std::mutex> lock(m_calendar->mutex);
            m_calendar->window = nullptr;
        }
        return InputWindow::WindowProc(message, wparam, lparam);
    }
}
