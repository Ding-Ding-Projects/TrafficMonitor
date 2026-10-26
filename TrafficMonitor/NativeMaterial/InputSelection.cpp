#include "../stdafx.h"
#include "InputControls.Internal.h"

// RadioGroup and SegmentedButton: one window, one tab stop, single selection among a few options.
namespace NativeMaterial
{
    namespace
    {
        void FillOptions(InputData& data, const std::vector<CString>& options, int selected, const CString& name)
        {
            data.items.clear();
            for (const auto& option : options) data.items.emplace_back(option.GetString());
            data.selected = selected;
            if (!name.IsEmpty()) data.name = name.GetString();
            data.text = selected >= 0 && selected < static_cast<int>(options.size()) ? options[selected].GetString() : L"";
        }
        int Clamp(int value, int low, int high) { return (std::max)(low, (std::min)(high, value)); }
    }

    // ------------------------------------------------------------ RadioGroup
    BOOL RadioGroup::Create(const RECT& bounds, CWnd* parent, UINT id)
    {
        return CreateInput(InputKind::RadioGroup, L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP, 0, bounds, parent, id);
    }
    void RadioGroup::SetOptions(const std::vector<CString>& options)
    {
        m_options = options;
        if (m_selected >= static_cast<int>(m_options.size())) m_selected = -1;
        Publish();
        if (m_hWnd) Invalidate(FALSE);
    }
    void RadioGroup::AddOption(const CString& option) { m_options.push_back(option); Publish(); if (m_hWnd) Invalidate(FALSE); }
    void RadioGroup::SetSelected(int index)
    {
        index = Clamp(index, -1, static_cast<int>(m_options.size()) - 1);
        if (index == m_selected) return;
        m_selected = index;
        Publish();
        if (m_hWnd) Invalidate(FALSE);
    }
    void RadioGroup::SetAccessibleName(const CString& name) { m_name = name; Publish(); }
    int RadioGroup::RowAt(CPoint point) const
    {
        if (point.x < 0 || point.y < 0) return -1;
        const int row = static_cast<int>(Dip(point.y) / InputMetrics::RowHeight);
        return row >= 0 && row < static_cast<int>(m_options.size()) ? row : -1;
    }
    void RadioGroup::Choose(int index)
    {
        if (index < 0 || index >= static_cast<int>(m_options.size()) || index == m_selected) return;
        m_selected = index;
        Publish();
        Invalidate(FALSE);
        Notify(CBN_SELCHANGE);
    }
    void RadioGroup::Paint(ID2D1RenderTarget* target, const Palette& palette, const InputState& state)
    {
        DrawRadioGroup(target, palette, m_options, m_selected, m_selected >= 0 ? m_selected : 0, m_hoverRow, state);
    }
    CSize RadioGroup::MinimumLogicalSize() const
    {
        int longest = 8;
        for (const auto& option : m_options) longest = (std::max)(longest, (std::min)(option.GetLength(), 40));
        return CSize(4 + 40 + 8 + longest * 8 + 4, static_cast<int>(InputMetrics::RowHeight) * (std::max)(1, static_cast<int>(m_options.size())));
    }
    void RadioGroup::FillAccess(InputData& data) const { FillOptions(data, m_options, m_selected, m_name); }
    void RadioGroup::PointerPressed(CPoint point) { m_pressedRow = RowAt(point); }
    void RadioGroup::PointerReleased(CPoint point, bool inside)
    {
        const int row = RowAt(point);
        if (inside && row >= 0 && row == m_pressedRow) Choose(row);
        m_pressedRow = -1;
    }
    void RadioGroup::PointerMoved(CPoint point) { m_hoverRow = RowAt(point); }
    void RadioGroup::PointerLeft() { m_hoverRow = -1; }
    bool RadioGroup::KeyPressed(UINT key, bool)
    {
        const int count = static_cast<int>(m_options.size());
        if (count == 0) return false;
        int target = -2;
        if (key == VK_UP || key == VK_LEFT) target = m_selected < 0 ? count - 1 : (m_selected + count - 1) % count;
        else if (key == VK_DOWN || key == VK_RIGHT) target = m_selected < 0 ? 0 : (m_selected + 1) % count;
        else if (key == VK_HOME) target = 0;
        else if (key == VK_END) target = count - 1;
        else if (key == VK_SPACE) target = m_selected < 0 ? 0 : m_selected;
        if (target == -2) return false;
        Choose(target);
        return true;
    }
    void RadioGroup::PerformAction(InputActionId action, int value) { if (action == InputActionId::Select) Choose(value); }

    // -------------------------------------------------------- SegmentedButton
    BOOL SegmentedButton::Create(const RECT& bounds, CWnd* parent, UINT id)
    {
        return CreateInput(InputKind::Segmented, L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP, 0, bounds, parent, id);
    }
    bool SegmentedButton::SetSegments(const std::vector<CString>& labels)
    {
        if (labels.size() < 2 || labels.size() > 5) return false;
        m_segments = labels;
        if (m_selected < 0 || m_selected >= static_cast<int>(m_segments.size())) m_selected = 0;
        Publish();
        if (m_hWnd) Invalidate(FALSE);
        return true;
    }
    void SegmentedButton::SetSelected(int index)
    {
        index = Clamp(index, -1, static_cast<int>(m_segments.size()) - 1);
        if (index == m_selected) return;
        m_selected = index;
        Publish();
        if (m_hWnd) Invalidate(FALSE);
    }
    void SegmentedButton::SetAccessibleName(const CString& name) { m_name = name; Publish(); }
    int SegmentedButton::SegmentAt(CPoint point) const
    {
        CRect client;
        GetClientRect(client);
        const auto rects = SegmentRects(Dip(client.Width()), Dip(client.Height()), static_cast<int>(m_segments.size()));
        const float x = Dip(point.x), y = Dip(point.y);
        for (size_t i = 0; i < rects.size(); ++i)
            if (x >= rects[i].left && x < rects[i].right && y >= (std::min)(rects[i].top, rects[i].bottom - 1) - 4 && y <= rects[i].bottom + 4) return static_cast<int>(i);
        return -1;
    }
    void SegmentedButton::Choose(int index)
    {
        if (index < 0 || index >= static_cast<int>(m_segments.size()) || index == m_selected) return;
        m_selected = index;
        Publish();
        Invalidate(FALSE);
        Notify(CBN_SELCHANGE);
    }
    void SegmentedButton::Paint(ID2D1RenderTarget* target, const Palette& palette, const InputState& state)
    {
        DrawSegments(target, palette, m_segments, m_selected, m_hoverSegment, state);
    }
    CSize SegmentedButton::MinimumLogicalSize() const
    {
        const int count = (std::max)(2, static_cast<int>(m_segments.size()));
        int each = static_cast<int>(InputMetrics::SegmentMinimumWidth);
        for (const auto& segment : m_segments) each = (std::max)(each, 52 + (std::min)(segment.GetLength(), 24) * 8);
        return CSize(6 + count * each, static_cast<int>(InputMetrics::MinimumTarget));
    }
    void SegmentedButton::FillAccess(InputData& data) const { FillOptions(data, m_segments, m_selected, m_name); }
    void SegmentedButton::PointerPressed(CPoint point) { m_pressedSegment = SegmentAt(point); }
    void SegmentedButton::PointerReleased(CPoint point, bool inside)
    {
        const int segment = SegmentAt(point);
        if (inside && segment >= 0 && segment == m_pressedSegment) Choose(segment);
        m_pressedSegment = -1;
    }
    void SegmentedButton::PointerMoved(CPoint point) { m_hoverSegment = SegmentAt(point); }
    void SegmentedButton::PointerLeft() { m_hoverSegment = -1; }
    bool SegmentedButton::KeyPressed(UINT key, bool)
    {
        const int count = static_cast<int>(m_segments.size());
        if (count == 0) return false;
        int target = -2;
        if (key == VK_LEFT || key == VK_UP) target = (std::max)(0, m_selected - 1);
        else if (key == VK_RIGHT || key == VK_DOWN) target = (std::min)(count - 1, m_selected + 1);
        else if (key == VK_HOME) target = 0;
        else if (key == VK_END) target = count - 1;
        if (target == -2) return false;
        Choose(target);
        return true;
    }
    void SegmentedButton::PerformAction(InputActionId action, int value) { if (action == InputActionId::Select) Choose(value); }
}
