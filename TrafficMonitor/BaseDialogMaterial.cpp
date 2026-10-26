// BaseDialogMaterial.cpp : the opt-in Material shell of CBaseDialog.
//
// A legacy dialog calls EnableMaterialShell() in its constructor. CBaseDialog then attaches the
// Material custom title bar, paints the surface color of the shared appearance preference and
// offers the helpers below to replace each stock control by a Material component with the same
// control ID. Dialogs that never opt in do not reach any of this code.

#include "stdafx.h"
#include "BaseDialog.h"
#include "TrafficMonitor.h"
#include "NativeMaterial/Controls.h"
#include "NativeMaterial/InputControls.h"
#include "NativeMaterial/ScrollableTextField.h"
#include "NativePreferences/Preferences.h"
#include "NativePreferences/Text.h"

namespace
{
    COLORREF ToColorRef(const D2D1_COLOR_F& c)
    {
        auto channel = [](float v) { return static_cast<BYTE>((std::max)(0.f, (std::min)(1.f, v)) * 255.f + 0.5f); };
        return RGB(channel(c.r), channel(c.g), channel(c.b));
    }

    // Everything a replacement needs to know about the stock control it takes over.
    struct StockInfo
    {
        CRect rect;
        HWND prev{};
        DWORD style{};
        bool visible{}, enabled{};
        UINT id{};
        CString text;
    };

    bool CaptureStock(CWnd& dialog, CWnd* stock, StockInfo& info)
    {
        if (stock == nullptr || stock->GetSafeHwnd() == nullptr)
            return false;
        stock->GetWindowRect(info.rect);
        dialog.ScreenToClient(info.rect);
        info.prev = ::GetWindow(stock->GetSafeHwnd(), GW_HWNDPREV);
        info.style = stock->GetStyle();
        info.visible = (info.style & WS_VISIBLE) != 0;
        info.enabled = (info.style & WS_DISABLED) == 0;
        info.id = static_cast<UINT>(stock->GetDlgCtrlID());
        stock->GetWindowText(info.text);
        return true;
    }

    CString ClassNameOf(HWND hwnd)
    {
        wchar_t buffer[256]{};
        ::GetClassNameW(hwnd, buffer, 255);
        return buffer;
    }

    bool IsTextStatic(DWORD style)
    {
        switch (style & SS_TYPEMASK)
        {
        case SS_LEFT: case SS_CENTER: case SS_RIGHT: case SS_SIMPLE: case SS_LEFTNOWORDWRAP:
            return true;
        default:
            return false;
        }
    }
}

NativeMaterial::Appearance CBaseDialog::MaterialAppearance() const
{
    return NativePreferences::EffectiveAppearance();
}

COLORREF CBaseDialog::MaterialSurfaceColor() const
{
    return ToColorRef(NativeMaterial::Palette::Resolve(MaterialAppearance()).surface);
}

CRect CBaseDialog::MaterialContentRect() const
{
    CRect client;
    GetClientRect(client);
    client.top = (std::min)(client.bottom, static_cast<LONG>(MaterialContentTop()));
    return client;
}

NativeMaterial::TitleBar::Labels CBaseDialog::MaterialCaptions() const
{
    NativeMaterial::TitleBar::Labels labels;
    labels.minimize = NativePreferences::InlinePair(L"Minimize", L"最小化");
    labels.maximize = NativePreferences::InlinePair(L"Maximize", L"最大化");
    labels.restore = NativePreferences::InlinePair(L"Restore", L"還原");
    const std::wstring& close = theApp.m_str_table.LoadText(TXT_CLOSE);
    labels.close = close.empty() ? NativePreferences::InlinePair(L"Close", L"關閉") : CString(close.c_str());
    return labels;
}

void CBaseDialog::AttachMaterialShell()
{
    // The shell positions everything itself, so the resource based dynamic layout stays off.
    EnableDynamicLayout(FALSE);

    CRect old_window, old_client;
    GetWindowRect(old_window);
    GetClientRect(old_client);

    const auto appearance = MaterialAppearance();
    if (FAILED(m_title_bar.Attach(this, appearance, MaterialCaptions())))
        return;

    // The caption area now belongs to the client area: push every template control below the bar and grow
    // the window by the bar so the original content keeps its size.
    const int bar = m_title_bar.HeightPx();
    const HWND bar_window = m_title_bar.BarWindow();
    for (CWnd* child = GetWindow(GW_CHILD); child != nullptr; child = child->GetWindow(GW_HWNDNEXT))
    {
        if (child->GetSafeHwnd() == bar_window)
            continue;
        CRect rect;
        child->GetWindowRect(rect);
        ScreenToClient(rect);
        child->SetWindowPos(nullptr, rect.left, rect.top + bar, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
    }
    const SIZE outer = m_title_bar.OuterSizeForContent(old_client.Width(), old_client.Height());
    SetWindowPos(nullptr, 0, 0, outer.cx, outer.cy, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);

    // The minimum size was taken from the template window; it grows by what the shell added.
    CRect new_window;
    GetWindowRect(new_window);
    m_min_size.cx += (new_window.Width() - old_window.Width()) * 96 / theApp.GetDpi();
    m_min_size.cy += (new_window.Height() - old_window.Height()) * 96 / theApp.GetDpi();

    SetBackgroundColor(MaterialSurfaceColor(), FALSE);
    NativeMaterial::ApplyNativeFrameAppearance(m_hWnd, appearance);
    m_material_preferences.Attach(m_hWnd);
}

void CBaseDialog::AttachMaterialPage()
{
    // A page is a child window of its host: no title bar and no frame attributes, only the surface color and the
    // shared appearance preference. The host lays the page out, so the resource based dynamic layout stays off.
    EnableDynamicLayout(FALSE);
    SetBackgroundColor(MaterialSurfaceColor(), FALSE);
    m_material_preferences.Attach(m_hWnd);
}

void CBaseDialog::ApplyMaterialAppearance()
{
    const auto appearance = MaterialAppearance();
    const COLORREF surface = MaterialSurfaceColor();
    if (!m_material_page)
        NativeMaterial::ApplyNativeFrameAppearance(m_hWnd, appearance);
    if (m_title_bar.IsAttached())
    {
        m_title_bar.SetAppearance(appearance);
        m_title_bar.SetLabels(MaterialCaptions());
    }
    SetBackgroundColor(surface, FALSE);
    for (const auto& apply : m_material_appliers)
        apply(appearance, surface);
    OnMaterialAppearanceChanged();
    RedrawWindow(nullptr, nullptr, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN);
}

LRESULT CBaseDialog::OnMaterialPreferences(WPARAM generation, LPARAM)
{
    if ((m_material_shell || m_material_page) && m_material_preferences.Accepts(generation))
        ApplyMaterialAppearance();
    return 0;
}

CWnd* CBaseDialog::AdoptMaterialControl(std::unique_ptr<CWnd> control, std::function<void(NativeMaterial::Appearance, COLORREF)> applier,
    HWND insert_after, DWORD stock_style, bool visible, bool enabled)
{
    CWnd* raw = control.get();
    if (m_dlg_font.GetSafeHandle() != NULL)
        raw->SetFont(&m_dlg_font);
    if (stock_style & WS_GROUP)
        raw->ModifyStyle(0, WS_GROUP);
    // Same z-order slot as the stock control, which is what tab order follows.
    raw->SetWindowPos(insert_after != nullptr ? CWnd::FromHandle(insert_after) : &CWnd::wndTop, 0, 0, 0, 0,
        SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    raw->ShowWindow(visible ? SW_SHOWNA : SW_HIDE);
    raw->EnableWindow(enabled);
    applier(MaterialAppearance(), MaterialSurfaceColor());
    m_material_appliers.push_back(std::move(applier));
    m_material_controls.push_back(std::move(control));
    return raw;
}

NativeMaterial::Label* CBaseDialog::ReplaceWithLabel(UINT id, NativeMaterial::LabelRole role)
{
    return ReplaceWithLabel(GetDlgItem(id), role);
}

NativeMaterial::Label* CBaseDialog::ReplaceWithLabel(CWnd* stock, NativeMaterial::LabelRole role)
{
    StockInfo info;
    if (!CaptureStock(*this, stock, info))
        return nullptr;
    stock->DestroyWindow();
    auto label = std::make_unique<NativeMaterial::Label>();
    if (!label->Create(info.text, WS_CHILD, info.rect, this, info.id))
        return nullptr;
    NativeMaterial::Label* raw = label.get();
    raw->SetRole(role);
    AdoptMaterialControl(std::move(label), [raw](NativeMaterial::Appearance a, COLORREF s) { raw->SetAppearance(a); raw->SetSurfaceColor(s); },
        info.prev, info.style, info.visible, info.enabled);
    return raw;
}

NativeMaterial::Label* CBaseDialog::AddMaterialLabel(const CString& text, NativeMaterial::LabelRole role, UINT id)
{
    auto label = std::make_unique<NativeMaterial::Label>();
    if (!label->Create(text, WS_CHILD, CRect(0, 0, 10, 10), this, id))
        return nullptr;
    NativeMaterial::Label* raw = label.get();
    raw->SetRole(role);
    // Insert after the last child so the label never changes the tab order of the template controls.
    HWND last = ::GetWindow(m_hWnd, GW_CHILD);
    while (last != nullptr && ::GetWindow(last, GW_HWNDNEXT) != nullptr)
        last = ::GetWindow(last, GW_HWNDNEXT);
    AdoptMaterialControl(std::move(label), [raw](NativeMaterial::Appearance a, COLORREF s) { raw->SetAppearance(a); raw->SetSurfaceColor(s); },
        last == raw->GetSafeHwnd() ? nullptr : last, 0, true, true);
    return raw;
}

NativeMaterial::Label* CBaseDialog::ReplaceGroupBoxWithHeading(CWnd* stock)
{
    StockInfo info;
    if (!CaptureStock(*this, stock, info))
        return nullptr;
    // The group box frame goes away; its caption becomes a section heading at the top edge of the group.
    info.rect.bottom = info.rect.top + DPI(24);
    stock->DestroyWindow();
    auto label = std::make_unique<NativeMaterial::Label>();
    if (!label->Create(info.text, WS_CHILD, info.rect, this, info.id))
        return nullptr;
    NativeMaterial::Label* raw = label.get();
    raw->SetRole(NativeMaterial::LabelRole::Secondary);
    AdoptMaterialControl(std::move(label), [raw](NativeMaterial::Appearance a, COLORREF s) { raw->SetAppearance(a); raw->SetSurfaceColor(s); },
        info.prev, info.style, info.visible, info.enabled);
    return raw;
}

NativeMaterial::Button* CBaseDialog::ReplaceWithButton(UINT id, NativeMaterial::ButtonVariant variant)
{
    return ReplaceWithButton(GetDlgItem(id), variant);
}

NativeMaterial::Button* CBaseDialog::ReplaceWithButton(CWnd* stock, NativeMaterial::ButtonVariant variant)
{
    StockInfo info;
    if (!CaptureStock(*this, stock, info))
        return nullptr;
    stock->DestroyWindow();
    auto button = std::make_unique<NativeMaterial::Button>();
    if (!button->Create(info.text, info.rect, this, info.id))
        return nullptr;
    NativeMaterial::Button* raw = button.get();
    raw->SetVariant(variant);
    AdoptMaterialControl(std::move(button), [raw](NativeMaterial::Appearance a, COLORREF s) { raw->SetAppearance(a); raw->SetSurfaceColor(s); },
        info.prev, info.style, info.visible, info.enabled);
    return raw;
}

NativeMaterial::Checkbox* CBaseDialog::ReplaceWithCheckbox(UINT id)
{
    return ReplaceWithCheckbox(GetDlgItem(id));
}

NativeMaterial::Checkbox* CBaseDialog::ReplaceWithCheckbox(CWnd* stock)
{
    StockInfo info;
    if (!CaptureStock(*this, stock, info))
        return nullptr;
    const bool checked = stock->SendMessage(BM_GETCHECK) == BST_CHECKED;
    stock->DestroyWindow();
    auto box = std::make_unique<NativeMaterial::Checkbox>();
    if (!box->Create(info.text, info.rect, this, info.id))
        return nullptr;
    NativeMaterial::Checkbox* raw = box.get();
    raw->SetChecked(checked);
    AdoptMaterialControl(std::move(box), [raw](NativeMaterial::Appearance a, COLORREF s) { raw->SetAppearance(a); raw->SetSurfaceColor(s); },
        info.prev, info.style, info.visible, info.enabled);
    return raw;
}

NativeMaterial::RadioButton* CBaseDialog::ReplaceWithRadioButton(UINT id, bool starts_group)
{
    StockInfo info;
    CWnd* stock = GetDlgItem(id);
    if (!CaptureStock(*this, stock, info))
        return nullptr;
    const bool checked = stock->SendMessage(BM_GETCHECK) == BST_CHECKED;
    stock->DestroyWindow();
    auto radio = std::make_unique<NativeMaterial::RadioButton>();
    if (!radio->Create(info.text, info.rect, this, info.id, starts_group))
        return nullptr;
    NativeMaterial::RadioButton* raw = radio.get();
    // The group boundary is decided by the caller, not by whatever the template control happened to carry.
    AdoptMaterialControl(std::move(radio), [raw](NativeMaterial::Appearance a, COLORREF s) { raw->SetAppearance(a); raw->SetSurfaceColor(s); },
        info.prev, starts_group ? (info.style | WS_GROUP) : (info.style & ~static_cast<DWORD>(WS_GROUP)), info.visible, info.enabled);
    if (!starts_group)
        raw->ModifyStyle(WS_GROUP, 0);
    if (checked)
        raw->SetChecked(true);
    return raw;
}

NativeMaterial::Dropdown* CBaseDialog::ReplaceWithDropdown(UINT id, const CString& label)
{
    StockInfo info;
    if (!CaptureStock(*this, GetDlgItem(id), info))
        return nullptr;
    GetDlgItem(id)->DestroyWindow();
    auto dropdown = std::make_unique<NativeMaterial::Dropdown>();
    if (!dropdown->Create(label, info.rect, this, info.id))
        return nullptr;
    NativeMaterial::Dropdown* raw = dropdown.get();
    AdoptMaterialControl(std::move(dropdown), [raw](NativeMaterial::Appearance a, COLORREF s) { raw->SetAppearance(a); raw->SetSurfaceColor(s); },
        info.prev, info.style, info.visible, info.enabled);
    return raw;
}

NativeMaterial::NumberStepper* CBaseDialog::ReplaceWithNumberStepper(UINT id, const CString& label, const CString& supporting)
{
    StockInfo info;
    if (!CaptureStock(*this, GetDlgItem(id), info))
        return nullptr;
    GetDlgItem(id)->DestroyWindow();
    auto stepper = std::make_unique<NativeMaterial::NumberStepper>();
    if (!stepper->Create(label, info.rect, this, info.id))
        return nullptr;
    NativeMaterial::NumberStepper* raw = stepper.get();
    raw->SetSupportingText(supporting);
    AdoptMaterialControl(std::move(stepper), [raw](NativeMaterial::Appearance a, COLORREF s) { raw->SetAppearance(a); raw->SetSurfaceColor(s); },
        info.prev, info.style, info.visible, info.enabled);
    return raw;
}

NativeMaterial::ScrollableTextField* CBaseDialog::ReplaceWithReadOnlyText(UINT id)
{
    StockInfo info;
    CWnd* stock = GetDlgItem(id);
    if (!CaptureStock(*this, stock, info))
        return nullptr;
    stock->DestroyWindow();
    auto field = std::make_unique<NativeMaterial::ScrollableTextField>();
    // No WS_VSCROLL: the Material scroll rail replaces the system scroll bar.
    if (!field->Create(WS_CHILD | WS_TABSTOP | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL | WS_CLIPCHILDREN, info.rect, this, info.id))
        return nullptr;
    NativeMaterial::ScrollableTextField* raw = field.get();
    field->SetLimitText(0);
    field->SetWindowText(info.text);
    AdoptMaterialControl(std::move(field), [raw](NativeMaterial::Appearance a, COLORREF) { raw->SetAppearance(a); },
        info.prev, info.style | WS_TABSTOP, info.visible, info.enabled);
    return raw;
}

void CBaseDialog::RemoveStockControl(UINT id)
{
    CWnd* stock = GetDlgItem(id);
    if (stock != nullptr)
        stock->DestroyWindow();
}

void CBaseDialog::ConvertRemainingStockControls()
{
    // Collect handles first: replacing a control changes the sibling list being walked.
    std::vector<HWND> children;
    const HWND bar = m_title_bar.BarWindow();
    for (HWND child = ::GetWindow(m_hWnd, GW_CHILD); child != nullptr; child = ::GetWindow(child, GW_HWNDNEXT))
    {
        if (child != bar)
            children.push_back(child);
    }
    for (HWND child : children)
    {
        if (!::IsWindow(child))
            continue;
        CWnd* stock = CWnd::FromHandle(child);
        const CString cls = ClassNameOf(child);
        const DWORD style = static_cast<DWORD>(::GetWindowLongPtrW(child, GWL_STYLE));
        if (cls.CompareNoCase(L"Static") == 0)
        {
            if (IsTextStatic(style))
                ReplaceWithLabel(stock);
        }
        else if (cls.CompareNoCase(L"Button") == 0)
        {
            const DWORD type = style & BS_TYPEMASK;
            const UINT id = static_cast<UINT>(::GetDlgCtrlID(child));
            if (type == BS_PUSHBUTTON || type == BS_DEFPUSHBUTTON)
            {
                using Variant = NativeMaterial::ButtonVariant;
                const Variant variant = (type == BS_DEFPUSHBUTTON || id == IDOK) ? Variant::Primary
                    : (id == IDCANCEL || id == IDCLOSE) ? Variant::Text : Variant::Tonal;
                ReplaceWithButton(stock, variant);
            }
            else if (type == BS_CHECKBOX || type == BS_AUTOCHECKBOX || type == BS_3STATE || type == BS_AUTO3STATE)
                ReplaceWithCheckbox(stock);
            else if (type == BS_GROUPBOX)
                ReplaceGroupBoxWithHeading(stock);
        }
    }
}

CString CBaseDialog::RemainingStockControls() const
{
    static const wchar_t* const stock_classes[] = {
        L"Static", L"Button", L"Edit", L"ComboBox", L"ListBox", L"SysListView32", L"SysTreeView32", L"SysTabControl32",
        L"msctls_trackbar32", L"msctls_updown32", L"msctls_progress32", L"msctls_statusbar32", L"ScrollBar", L"SysLink",
        L"tooltips_class32", L"ComboLBox", L"RichEdit20W", L"SysDateTimePick32", L"SysMonthCal32" };
    const HWND bar = m_title_bar.BarWindow();
    CString result;
    for (HWND child = ::GetWindow(m_hWnd, GW_CHILD); child != nullptr; child = ::GetWindow(child, GW_HWNDNEXT))
    {
        if (child == bar)
            continue;
        const CString cls = ClassNameOf(child);
        for (const wchar_t* name : stock_classes)
        {
            if (cls.CompareNoCase(name) == 0)
            {
                CString entry;
                entry.Format(L"%s#%d ", cls.GetString(), ::GetDlgCtrlID(child));
                result += entry;
                break;
            }
        }
    }
    return result;
}

void CBaseDialog::SetDlgCheck(UINT id, bool checked)
{
    if (auto* box = dynamic_cast<NativeMaterial::Checkbox*>(GetDlgItem(id)))
        box->SetChecked(checked);
    else if (auto* radio = dynamic_cast<NativeMaterial::RadioButton*>(GetDlgItem(id)))
        radio->SetChecked(checked);     // choosing a radio clears the other radios of its group
    else
        CheckDlgButton(id, checked ? BST_CHECKED : BST_UNCHECKED);
}

bool CBaseDialog::GetDlgCheck(UINT id) const
{
    if (auto* box = dynamic_cast<NativeMaterial::Checkbox*>(const_cast<CBaseDialog*>(this)->GetDlgItem(id)))
        return box->IsChecked();
    if (auto* radio = dynamic_cast<NativeMaterial::RadioButton*>(const_cast<CBaseDialog*>(this)->GetDlgItem(id)))
        return radio->IsChecked();
    return IsDlgButtonChecked(id) != 0;
}

void CBaseDialog::SetControlLabel(UINT id, const CString& label)
{
    CWnd* control = GetDlgItem(id);
    if (auto* button = dynamic_cast<NativeMaterial::Button*>(control))
        button->SetLabel(label);
    else if (auto* box = dynamic_cast<NativeMaterial::Checkbox*>(control))
        box->SetLabel(label);
    else if (control != nullptr)
        control->SetWindowText(label);
}

int CBaseDialog::MeasureMaterialButtonWidth(const CString& label, NativeMaterial::ButtonVariant variant, int minimum_dip) const
{
    // The Material button draws its label in Segoe UI Medium, 14 dip, with a horizontal inset per variant.
    LOGFONTW face{};
    face.lfHeight = -MulDiv(14, m_dpi, 96);
    face.lfWeight = FW_MEDIUM;
    face.lfCharSet = DEFAULT_CHARSET;
    wcscpy_s(face.lfFaceName, L"Segoe UI");
    CFont font;
    font.CreateFontIndirectW(&face);
    CClientDC dc(const_cast<CBaseDialog*>(this));
    CFont* previous = dc.SelectObject(&font);
    const int text = dc.GetTextExtent(label).cx;
    dc.SelectObject(previous);
    const bool compact = variant == NativeMaterial::ButtonVariant::Text || variant == NativeMaterial::ButtonVariant::Destructive;
    return (std::max)(DPI(minimum_dip), text + DPI((compact ? 12 : 24) * 2 + 6 + 8));
}

void CBaseDialog::FitMaterialContent(int content_width_dip)
{
    if (!m_material_shell || !m_title_bar.IsAttached())
        return;
    const int width = DPI(content_width_dip);
    const int height = LayoutMaterialControls(CRect(0, MaterialContentTop(), width, MaterialContentTop()), false);
    const SIZE outer = m_title_bar.OuterSizeForContent(width, height);
    m_min_size.cx = outer.cx * 96 / theApp.GetDpi();
    m_min_size.cy = outer.cy * 96 / theApp.GetDpi();

    CRect current;
    GetWindowRect(current);
    // A size the user saved earlier is kept unless it would cut the content.
    const int cx = m_has_saved_size ? (std::max)(static_cast<int>(current.Width()), static_cast<int>(outer.cx)) : outer.cx;
    const int cy = m_has_saved_size ? (std::max)(static_cast<int>(current.Height()), static_cast<int>(outer.cy)) : outer.cy;
    SetWindowPos(nullptr, 0, 0, cx, cy, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
}

void CBaseDialog::RelayoutMaterial()
{
    if (!m_material_shell || !m_material_ready || m_hWnd == nullptr)
        return;
    LayoutMaterialControls(MaterialContentRect(), true);
}

void CBaseDialog::FinishMaterialShell()
{
    if (!m_material_shell)
        return;
    ConvertRemainingStockControls();
    const CString remaining = RemainingStockControls();
    if (!remaining.IsEmpty())
        TRACE(L"Material shell: stock controls remain: %s\n", remaining.GetString());
    m_material_ready = true;
    RelayoutMaterial();
    CenterWindow();
}
