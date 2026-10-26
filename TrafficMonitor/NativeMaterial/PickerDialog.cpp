#include "../stdafx.h"
#include "PickerDialog.h"
#include "PickerDraw.h"
#include "../NativePreferences/Text.h"
#include <UIAutomation.h>
#include <algorithm>
#include <atomic>
#pragma comment(lib, "uiautomationcore.lib")

namespace NativeMaterial
{
    namespace
    {
        const wchar_t* const InspectProperty = L"NativeMaterial.PickerDialog";
        const PickerTestHooks* g_hooks{};
        constexpr float TitleTop = 24.f, TitleHeight = 32.f, ActionHeight = 40.f, ActionMargin = 3.f, ActionWidth = 88.f, ActionGap = 8.f;
        constexpr float ContentTop = 68.f, ContentGap = 8.f, Padding = 24.f;

        D2D1_COLOR_F SurfaceColor(const Palette& p)
        {
            return p.highContrast ? p.surface : BlendColor(p.onSurface, p.surfaceContainer, .04f);
        }

        class DialogProvider final : public IRawElementProviderSimple
        {
        public:
            explicit DialogProvider(std::shared_ptr<std::mutex> lock, HWND* window, std::wstring* name) : m_lock(std::move(lock)), m_window(window), m_name(name) {}
            HRESULT STDMETHODCALLTYPE QueryInterface(REFIID id, void** value) override
            {
                if (!value) return E_POINTER;
                *value = nullptr;
                if (id != __uuidof(IUnknown) && id != __uuidof(IRawElementProviderSimple)) return E_NOINTERFACE;
                *value = static_cast<IRawElementProviderSimple*>(this);
                AddRef();
                return S_OK;
            }
            ULONG STDMETHODCALLTYPE AddRef() override { return ++m_references; }
            ULONG STDMETHODCALLTYPE Release() override { const auto remaining = --m_references; if (!remaining) delete this; return remaining; }
            HRESULT STDMETHODCALLTYPE get_ProviderOptions(ProviderOptions* value) override
            { if (!value) return E_POINTER; *value = ProviderOptions_ServerSideProvider; return S_OK; }
            HRESULT STDMETHODCALLTYPE GetPatternProvider(PATTERNID, IUnknown** value) override
            { if (!value) return E_POINTER; *value = nullptr; return S_OK; }
            HRESULT STDMETHODCALLTYPE GetPropertyValue(PROPERTYID id, VARIANT* value) override
            {
                if (!value) return E_POINTER;
                VariantInit(value);
                std::lock_guard<std::mutex> guard(*m_lock);
                if (!*m_window) return UIA_E_ELEMENTNOTAVAILABLE;
                if (id == UIA_ControlTypePropertyId) { value->vt = VT_I4; value->lVal = UIA_WindowControlTypeId; }
                else if (id == UIA_NamePropertyId) { value->vt = VT_BSTR; value->bstrVal = SysAllocString(m_name->c_str()); return value->bstrVal ? S_OK : E_OUTOFMEMORY; }
                else if (id == UIA_AutomationIdPropertyId) { value->vt = VT_BSTR; value->bstrVal = SysAllocString(L"NativeMaterial.PickerDialog"); return value->bstrVal ? S_OK : E_OUTOFMEMORY; }
                else if (id == UIA_IsDialogPropertyId || id == UIA_IsEnabledPropertyId || id == UIA_IsControlElementPropertyId || id == UIA_IsContentElementPropertyId)
                { value->vt = VT_BOOL; value->boolVal = VARIANT_TRUE; }
                return S_OK;
            }
            HRESULT STDMETHODCALLTYPE get_HostRawElementProvider(IRawElementProviderSimple** value) override
            {
                if (!value) return E_POINTER;
                *value = nullptr;
                std::lock_guard<std::mutex> guard(*m_lock);
                return *m_window ? UiaHostProviderFromHwnd(*m_window, value) : UIA_E_ELEMENTNOTAVAILABLE;
            }
        private:
            std::atomic<ULONG> m_references{ 1 };
            std::shared_ptr<std::mutex> m_lock;
            HWND* m_window;
            std::wstring* m_name;
        };

        HWND ResolveOwner(HWND owner)
        {
            if (!owner || !IsWindow(owner)) return nullptr;
            HWND root = GetAncestor(owner, GA_ROOT);
            if (!root) return nullptr;
            HWND popup = GetLastActivePopup(root);
            return popup && IsWindow(popup) && IsWindowVisible(popup) ? popup : root;
        }
    }

    void SetPickerTestHooks(const PickerTestHooks* hooks) { g_hooks = hooks; }

    PickerDialog::PickerDialog() = default;
    PickerDialog::~PickerDialog() = default;

    PickerDialog* PickerDialog::FromWindow(HWND window)
    {
        return window ? static_cast<PickerDialog*>(GetPropW(window, InspectProperty)) : nullptr;
    }

    int PickerDialog::Px(float dip) const { return static_cast<int>(std::lround(dip * m_dpi / 96.0)); }
    float PickerDialog::Dip(int px) const { return static_cast<float>(px) * 96.f / static_cast<float>(m_dpi ? m_dpi : 96); }
    COLORREF PickerDialog::SurfaceRef() const { return PickerDraw::ToRef(SurfaceColor(Palette::Resolve(m_appearance))); }

    CRect PickerDialog::ContentRectPx() const
    {
        const float bottom = HeightDip() - Padding - ActionHeight - ContentGap;
        return CRect(Px(Padding), Px(ContentTop), Px(WidthDip() - Padding), Px(bottom));
    }
    D2D1_RECT_F PickerDialog::ContentRectDip() const
    {
        return D2D1::RectF(Padding, ContentTop, WidthDip() - Padding, HeightDip() - Padding - ActionHeight - ContentGap);
    }

    void PickerDialog::MovePx(CWnd& child, float x, float y, float w, float h)
    {
        if (child.GetSafeHwnd()) child.MoveWindow(Px(x), Px(y), Px(w), Px(h), TRUE);
    }

    void PickerDialog::SetOkEnabled(bool enabled)
    {
        m_okEnabled = enabled;
        if (m_ok && m_ok->GetSafeHwnd()) m_ok->EnableWindow(enabled);
    }
    bool PickerDialog::IsOkEnabled() const { return m_okEnabled; }

    void PickerDialog::Finish(int result) { m_result = result; m_done = true; }

    void PickerDialog::LayoutAll(bool keepCenter)
    {
        if (!m_hWnd) return;
        CRect rect;
        GetWindowRect(rect);
        const int cx = (rect.left + rect.right) / 2, cy = (rect.top + rect.bottom) / 2;
        int width = Px(static_cast<float>(m_widthDip)), height = Px(static_cast<float>(m_heightDip));
        const int workWidth = m_work.right - m_work.left, workHeight = m_work.bottom - m_work.top;
        width = (std::min)(width, workWidth);
        height = (std::min)(height, workHeight);
        int x = keepCenter ? cx - width / 2 : rect.left, y = keepCenter ? cy - height / 2 : rect.top;
        x = (std::max)(static_cast<int>(m_work.left), (std::min)(x, static_cast<int>(m_work.right) - width));
        y = (std::max)(static_cast<int>(m_work.top), (std::min)(y, static_cast<int>(m_work.bottom) - height));
        SetWindowPos(nullptr, x, y, width, height, SWP_NOZORDER | SWP_NOACTIVATE);
        const int radius = Px(28.f);
        SetWindowRgn(CreateRoundRectRgn(0, 0, width + 1, height + 1, radius * 2, radius * 2), TRUE);
        const float actionsTop = HeightDip() - Padding - ActionHeight;
        const float okLeft = WidthDip() - Padding - ActionWidth, cancelLeft = okLeft - ActionGap - ActionWidth;
        if (m_ok) MovePx(*m_ok, okLeft - ActionMargin, actionsTop - ActionMargin, ActionWidth + 2 * ActionMargin, ActionHeight + 2 * ActionMargin);
        if (m_cancel) MovePx(*m_cancel, cancelLeft - ActionMargin, actionsTop - ActionMargin, ActionWidth + 2 * ActionMargin, ActionHeight + 2 * ActionMargin);
        LayoutContent();
    }

    std::vector<HWND> PickerDialog::FocusOrder() const
    {
        std::vector<HWND> order;
        auto usable = [](HWND window) { return window && ::IsWindowVisible(window) && ::IsWindowEnabled(window); };
        for (HWND window : ContentFocusOrder()) if (usable(window)) order.push_back(window);
        if (usable(CancelWindow())) order.push_back(CancelWindow());
        if (usable(OkWindow())) order.push_back(OkWindow());
        return order;
    }

    void PickerDialog::MoveFocus(int step)
    {
        const auto order = FocusOrder();
        if (order.empty()) return;
        const HWND focus = ::GetFocus();
        int index = -1;
        for (size_t i = 0; i < order.size(); ++i)
            if (focus && (focus == order[i] || ::IsChild(order[i], focus))) { index = static_cast<int>(i); break; }
        const int count = static_cast<int>(order.size());
        const int next = index < 0 ? (step > 0 ? 0 : count - 1) : ((index + step) % count + count) % count;
        ::SetFocus(order[next]);
    }

    bool PickerDialog::HandleKey(UINT key, bool shift)
    {
        switch (key)
        {
        case VK_ESCAPE:
            Finish(IDCANCEL);
            return true;
        case VK_RETURN:
        {
            const HWND focus = ::GetFocus();
            if (focus && focus == CancelWindow()) Finish(IDCANCEL);
            else if (m_okEnabled && CanAccept() && CommitPending()) Finish(IDOK);
            return true;
        }
        case VK_TAB:
            MoveFocus(shift ? -1 : 1);
            return true;
        default:
            return false;
        }
    }

    bool PickerDialog::PretranslateKey(const MSG& msg)
    {
        if ((msg.message != WM_KEYDOWN && msg.message != WM_SYSKEYDOWN) || (msg.hwnd != m_hWnd && !::IsChild(m_hWnd, msg.hwnd))) return false;
        if (msg.message == WM_SYSKEYDOWN) return false;
        if ((GetKeyState(VK_CONTROL) & 0x8000) || (GetKeyState(VK_MENU) & 0x8000)) return false;
        const bool repeat = (msg.lParam & 0x40000000) != 0;
        if ((msg.wParam == VK_ESCAPE || msg.wParam == VK_RETURN) && repeat) return true;   // a held key that opened the dialog must not also close it
        return HandleKey(static_cast<UINT>(msg.wParam), (GetKeyState(VK_SHIFT) & 0x8000) != 0);
    }

    HRESULT PickerDialog::PaintTo(HDC dc)
    {
        Microsoft::WRL::ComPtr<ID2D1Factory> factory;
        Microsoft::WRL::ComPtr<ID2D1DCRenderTarget> target;
        HRESULT result = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, factory.GetAddressOf());
        const float dpi = static_cast<float>(m_dpi);
        auto properties = D2D1::RenderTargetProperties(D2D1_RENDER_TARGET_TYPE_DEFAULT,
            D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE), dpi, dpi);
        if (SUCCEEDED(result)) result = factory->CreateDCRenderTarget(&properties, target.GetAddressOf());
        CRect bounds;
        GetClientRect(bounds);
        if (SUCCEEDED(result)) result = target->BindDC(dc, bounds);
        if (SUCCEEDED(result))
        {
            const auto palette = Palette::Resolve(m_appearance);
            target->BeginDraw();
            target->Clear(SurfaceColor(palette));
            PickerDraw::Painter painter(target.Get(), palette);
            if (painter.Ok())
            {
                PickerDraw::TextStyle headline;
                headline.size = 24.f;
                painter.Text(m_title, D2D1::RectF(Padding, TitleTop, WidthDip() - Padding, TitleTop + TitleHeight), headline, palette.onSurface);
            }
            PaintContent(target.Get(), palette);
            result = target->EndDraw();
        }
        return result;
    }

    LRESULT PickerDialog::WindowProc(UINT message, WPARAM wparam, LPARAM lparam)
    {
        switch (message)
        {
        case WM_PAINT:
        {
            PAINTSTRUCT ps{};
            ::BeginPaint(m_hWnd, &ps);
            PaintTo(ps.hdc);
            ::EndPaint(m_hWnd, &ps);
            return 0;
        }
        case WM_ERASEBKGND: return 1;
        case WM_PRINTCLIENT:
            if (wparam) PaintTo(reinterpret_cast<HDC>(wparam));
            return 0;
        case WM_COMMAND:
        {
            const UINT id = LOWORD(wparam), code = HIWORD(wparam);
            if (code == BN_CLICKED && id == IDOK && lparam) { if (m_okEnabled && CanAccept() && CommitPending()) Finish(IDOK); return 0; }
            if (code == BN_CLICKED && id == IDCANCEL && lparam) { Finish(IDCANCEL); return 0; }
            if (ContentCommand(id, code)) return 0;
            break;
        }
        case WM_CLOSE: Finish(IDCANCEL); return 0;
        case WM_MOUSEACTIVATE: return MA_ACTIVATE;
        case WM_DPICHANGED:
            m_dpi = HIWORD(wparam);
            LayoutAll(true);
            Invalidate(FALSE);
            return 0;
        case WM_GETOBJECT:
            if (static_cast<LONG>(lparam) == static_cast<LONG>(UiaRootObjectId) && m_access)
            {
                auto* provider = new DialogProvider(std::shared_ptr<std::mutex>(m_access, &m_access->mutex), &m_access->window, &m_access->name);
                const LRESULT result = UiaReturnRawElementProvider(m_hWnd, wparam, lparam, provider);
                provider->Release();
                return result;
            }
            break;
        case WM_DESTROY:
            RemovePropW(m_hWnd, InspectProperty);
            if (m_access) { std::lock_guard<std::mutex> lock(m_access->mutex); m_access->window = nullptr; }
            UiaReturnRawElementProvider(m_hWnd, 0, 0, nullptr);
            break;
        }
        return CWnd::WindowProc(message, wparam, lparam);
    }

    int PickerDialog::RunModal(HWND ownerWindow, const CString& title, int widthDip, int heightDip)
    {
        m_title = title;
        m_widthDip = widthDip;
        m_heightDip = heightDip;
        m_done = false;
        m_result = IDCANCEL;
        m_appearance = g_hooks && g_hooks->forceAppearance ? g_hooks->appearance : NativePreferences::EffectiveAppearance();

        const HWND previousFocus = ::GetFocus();
        const HWND owner = ResolveOwner(ownerWindow);
        const bool ownerShown = owner && ::IsWindowVisible(owner) && !::IsIconic(owner);
        m_dpi = owner ? GetDpiForWindow(owner) : GetDpiForSystem();
        if (!m_dpi) m_dpi = 96;
        MONITORINFO monitor{ sizeof(monitor) };
        const HMONITOR handle = ownerShown ? MonitorFromWindow(owner, MONITOR_DEFAULTTONEAREST) : MonitorFromPoint({ 0, 0 }, MONITOR_DEFAULTTOPRIMARY);
        if (!GetMonitorInfoW(handle, &monitor)) monitor.rcWork = { 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN) };
        m_work = monitor.rcWork;

        const int width = (std::min)(Px(static_cast<float>(widthDip)), static_cast<int>(m_work.right - m_work.left));
        const int height = (std::min)(Px(static_cast<float>(heightDip)), static_cast<int>(m_work.bottom - m_work.top));
        RECT anchor = m_work;
        if (ownerShown) ::GetWindowRect(owner, &anchor);
        const int x = (std::max)(static_cast<int>(m_work.left), (std::min)(static_cast<int>((anchor.left + anchor.right - width) / 2), static_cast<int>(m_work.right) - width));
        const int y = (std::max)(static_cast<int>(m_work.top), (std::min)(static_cast<int>((anchor.top + anchor.bottom - height) / 2), static_cast<int>(m_work.bottom) - height));

        const auto className = AfxRegisterWndClass(CS_DROPSHADOW | CS_HREDRAW | CS_VREDRAW, LoadCursor(nullptr, IDC_ARROW), nullptr);
        const DWORD exStyle = WS_EX_CONTROLPARENT | (owner ? WS_EX_TOOLWINDOW : WS_EX_APPWINDOW);
        if (!CWnd::CreateEx(exStyle, className, m_title, WS_POPUP | WS_CLIPCHILDREN, x, y, width, height, owner, nullptr))
            return IDCANCEL;
        SetPropW(m_hWnd, InspectProperty, this);
        m_access = std::make_shared<Access>();
        m_access->window = m_hWnd;
        m_access->name = m_title.GetString();

        m_cancel = std::make_unique<Button>();
        m_ok = std::make_unique<Button>();
        m_cancel->Create(NativePreferences::Pair(L"Cancel", L"取消"), CRect(0, 0, 10, 10), this, IDCANCEL);
        m_ok->Create(NativePreferences::Pair(L"OK", L"確定"), CRect(0, 0, 10, 10), this, IDOK);
        Style(*m_cancel);
        Style(*m_ok);
        m_cancel->SetVariant(ButtonVariant::Text);
        m_ok->SetVariant(ButtonVariant::Primary);
        CreateContent();
        LayoutAll(false);
        m_ok->EnableWindow(m_okEnabled);

        bool disabledOwner = false;
        if (owner && ::IsWindowEnabled(owner)) { ::EnableWindow(owner, FALSE); disabledOwner = true; }
        ShowWindow(SW_SHOW);
        SetForegroundWindow();
        SetActiveWindow();
        HWND focus = InitialFocus();
        if (!focus || !::IsWindowEnabled(focus)) focus = OkWindow();
        if (focus) ::SetFocus(focus);
        UpdateWindow();
        ::NotifyWinEvent(EVENT_SYSTEM_DIALOGSTART, m_hWnd, OBJID_CLIENT, CHILDID_SELF);
        if (g_hooks && g_hooks->shown) g_hooks->shown(m_hWnd, g_hooks->context);

        MSG msg{};
        while (!m_done)
        {
            const BOOL got = GetMessage(&msg, nullptr, 0, 0);
            if (got == 0) { PostQuitMessage(static_cast<int>(msg.wParam)); Finish(IDCANCEL); break; }
            if (got < 0) break;
            if (PretranslateKey(msg)) continue;
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        if (disabledOwner) ::EnableWindow(owner, TRUE);
        ::NotifyWinEvent(EVENT_SYSTEM_DIALOGEND, m_hWnd, OBJID_CLIENT, CHILDID_SELF);
        const int result = m_result;
        DestroyWindow();
        m_ok.reset();
        m_cancel.reset();
        if (previousFocus && IsWindow(previousFocus) && GetWindowThreadProcessId(previousFocus, nullptr) == GetCurrentThreadId()) ::SetFocus(previousFocus);
        return result;
    }
}
