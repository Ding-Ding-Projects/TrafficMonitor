#include "../stdafx.h"
#include "InputControls.Internal.h"
#include "LayoutObservations.h"
#include <oleacc.h>
#pragma comment(lib, "uiautomationcore.lib")
#pragma comment(lib, "oleacc.lib")
#pragma comment(lib, "uuid.lib")

namespace NativeMaterial
{
    namespace
    {
        // Values of CLSID_AccPropServices and PROPID_ACC_NAME from oleacc.h; the import library does not export them.
        const CLSID AccPropServicesClass = { 0xb5f8350b, 0x0548, 0x48b1, { 0xa6, 0xee, 0x88, 0xbd, 0x00, 0xb4, 0xa5, 0xe7 } };
        const MSAAPROPID AccNameProperty = { 0x608d3df8, 0x8128, 0x4aa7, { 0xa4, 0x28, 0xf5, 0x5e, 0x49, 0x26, 0x72, 0x91 } };
    }
    void AnnotateAccessibleName(HWND window, const CString& name)
    {
        if (!window) return;
        // Needs COM on the calling thread; when it is unavailable the control keeps its default accessible object.
        Microsoft::WRL::ComPtr<IAccPropServices> services;
        if (FAILED(CoCreateInstance(AccPropServicesClass, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(services.GetAddressOf())))) return;
        services->SetHwndPropStr(window, OBJID_CLIENT, CHILDID_SELF, AccNameProperty, name.GetString());
    }

    void CreateInputFont(CFont& font, UINT dpi)
    {
        font.DeleteObject();
        LOGFONTW face{};
        face.lfHeight = -DipToPx(16.f, dpi);
        face.lfWeight = FW_NORMAL;
        face.lfCharSet = DEFAULT_CHARSET;
        face.lfQuality = CLEARTYPE_QUALITY;
        wcscpy_s(face.lfFaceName, L"Segoe UI");
        font.CreateFontIndirectW(&face);
    }

    void PlaceCenteredEdit(CWnd& edit, CFont& font, D2D1_RECT_F dipRect, UINT dpi)
    {
        if (!edit.GetSafeHwnd()) return;
        edit.SetFont(&font);
        CClientDC dc(&edit);
        auto* previous = dc.SelectObject(&font);
        TEXTMETRICW metrics{};
        dc.GetTextMetrics(&metrics);
        dc.SelectObject(previous);
        const LONG left = DipToPx(dipRect.left, dpi), right = DipToPx(dipRect.right, dpi);
        const LONG middle = DipToPx((dipRect.top + dipRect.bottom) / 2, dpi);
        const LONG height = metrics.tmHeight + 2;
        edit.SetWindowPos(nullptr, left, middle - height / 2, (std::max)(1L, right - left), height, SWP_NOZORDER | SWP_NOACTIVATE);
    }

    BEGIN_MESSAGE_MAP(InputWindow, CWnd)
        ON_WM_PAINT()
        ON_WM_ERASEBKGND()
        ON_WM_LBUTTONDOWN()
        ON_WM_LBUTTONUP()
        ON_WM_MOUSEMOVE()
        ON_WM_CAPTURECHANGED()
        ON_MESSAGE(WM_MOUSELEAVE, &InputWindow::OnMouseLeave)
        ON_WM_KEYDOWN()
        ON_WM_SYSKEYDOWN()
        ON_WM_KEYUP()
        ON_WM_CHAR()
        ON_WM_MOUSEWHEEL()
        ON_WM_SETFOCUS()
        ON_WM_KILLFOCUS()
        ON_WM_ENABLE()
        ON_WM_DESTROY()
        ON_WM_GETDLGCODE()
        ON_MESSAGE(WM_GETOBJECT, &InputWindow::OnGetObject)
        ON_MESSAGE(InputAction, &InputWindow::OnInputAction)
        ON_MESSAGE(WM_PRINTCLIENT, &InputWindow::OnPrintClient)
        ON_MESSAGE(WM_PRINT, &InputWindow::OnPrintClient)
        ON_MESSAGE(WM_DPICHANGED_AFTERPARENT, &InputWindow::OnDpiAfterParent)
    END_MESSAGE_MAP()

    InputWindow::InputWindow() = default;
    InputWindow::~InputWindow() = default;

    BOOL InputWindow::CreateInput(InputKind kind, const CString& text, DWORD style, DWORD exStyle, const RECT& bounds, CWnd* parent, UINT id)
    {
        m_kind = kind;
        m_access = std::make_shared<InputAccess>();
        m_access->data.kind = kind;
        const auto className = AfxRegisterWndClass(CS_HREDRAW | CS_VREDRAW, LoadCursor(nullptr, IDC_ARROW));
        if (!CWnd::CreateEx(exStyle, className, text, style, bounds, parent, id)) return FALSE;
        { std::lock_guard<std::mutex> lock(m_access->mutex); m_access->window = m_hWnd; }
        Publish();
        return TRUE;
    }

    UINT InputWindow::Dpi() const
    {
        const UINT dpi = m_hWnd ? GetDpiForWindow(m_hWnd) : 96;
        return dpi ? dpi : 96;
    }
    int InputWindow::Px(float dip) const { return static_cast<int>(DipToPx(dip, Dpi())); }
    float InputWindow::Dip(int px) const { return static_cast<float>(px) * 96.f / static_cast<float>(Dpi()); }
    CSize InputWindow::MinimumSize() const { const CSize s = MinimumLogicalSize(); return CSize(Px(static_cast<float>(s.cx)), Px(static_cast<float>(s.cy))); }

    InputState InputWindow::State() const
    {
        InputState s;
        s.enabled = m_hWnd && IsWindowEnabled();
        s.hover = m_hover; s.pressed = m_pressed; s.focused = m_hWnd && IsFocused(); s.error = m_error;
        return s;
    }

    void InputWindow::SetAppearance(Appearance appearance)
    {
        m_appearance = appearance;
        AppearanceChanged();
        NativeLayoutObservation::Invalidate(m_hWnd);
        if (m_hWnd) Invalidate(FALSE);
    }
    void InputWindow::SetSurfaceColor(COLORREF color)
    {
        m_surfaceColor = color; m_customSurface = true;
        NativeLayoutObservation::Invalidate(m_hWnd);
        if (m_hWnd) Invalidate(FALSE);
    }
    void InputWindow::SetError(bool error) { m_error = error; }

    void InputWindow::Notify(UINT code)
    {
        if (m_hWnd && IsWindowEnabled() && GetParent())
            GetParent()->SendMessage(WM_COMMAND, MAKEWPARAM(GetDlgCtrlID(), code), reinterpret_cast<LPARAM>(m_hWnd));
    }

    void InputWindow::FillAccess(InputData&) const {}

    HRESULT InputWindow::AutomationProvider(IRawElementProviderSimple** provider) const
    {
        if (!provider) return E_POINTER;
        *provider = nullptr;
        if (!m_access) return E_UNEXPECTED;
        *provider = new InputProvider(m_access);
        return S_OK;
    }

    void InputWindow::Publish()
    {
        if (!m_access || !m_hWnd) return;
        InputData next;
        next.kind = m_kind;
        next.enabled = IsWindowEnabled() != FALSE;
        next.error = m_error;
        CString name;
        GetWindowText(name);
        next.name = name.GetString();
        FillAccess(next);
        InputData before;
        {
            std::lock_guard<std::mutex> lock(m_access->mutex);
            before = m_access->data;
            m_access->data = next;
        }
        RaiseChanges(before, next);
    }

    void InputWindow::RaiseChanges(const InputData& before, const InputData& after)
    {
        const bool listening = UiaClientsAreListening() != FALSE;
        auto raise = [&](PROPERTYID id, VARIANT oldValue, VARIANT newValue)
        {
            if (!listening) return;
            auto* provider = new InputProvider(m_access);
            UiaRaiseAutomationPropertyChangedEvent(provider, id, oldValue, newValue);
            provider->Release();
        };
        auto integer = [](int v) { VARIANT x{}; x.vt = VT_I4; x.lVal = v; return x; };
        auto state = [](int toggle) { return toggle == 1 ? ToggleState_On : toggle == 2 ? ToggleState_Indeterminate : ToggleState_Off; };
        if (before.toggle != after.toggle)
        {
            if (after.kind == InputKind::Radio)
            {
                VARIANT a{}, b{}; a.vt = b.vt = VT_BOOL;
                a.boolVal = before.toggle == 1 ? VARIANT_TRUE : VARIANT_FALSE; b.boolVal = after.toggle == 1 ? VARIANT_TRUE : VARIANT_FALSE;
                raise(UIA_SelectionItemIsSelectedPropertyId, a, b);
            }
            else raise(UIA_ToggleToggleStatePropertyId, integer(state(before.toggle)), integer(state(after.toggle)));
            ::NotifyWinEvent(EVENT_OBJECT_STATECHANGE, m_hWnd, OBJID_CLIENT, CHILDID_SELF);
        }
        if (before.value != after.value && after.kind == InputKind::Stepper)
        {
            VARIANT a{}, b{}; a.vt = b.vt = VT_R8; a.dblVal = before.value; b.dblVal = after.value;
            raise(UIA_RangeValueValuePropertyId, a, b);
            ::NotifyWinEvent(EVENT_OBJECT_VALUECHANGE, m_hWnd, OBJID_CLIENT, CHILDID_SELF);
        }
        if (before.selected != after.selected)
        {
            if (listening)
            {
                auto* provider = new InputProvider(m_access);
                UiaRaiseAutomationEvent(provider, UIA_SelectionItem_ElementSelectedEventId);
                provider->Release();
            }
            ::NotifyWinEvent(after.kind == InputKind::Dropdown ? EVENT_OBJECT_VALUECHANGE : EVENT_OBJECT_SELECTION, m_hWnd, OBJID_CLIENT, CHILDID_SELF);
        }
        if (before.text != after.text && after.kind == InputKind::Dropdown && listening)
        {
            VARIANT a{}, b{}; a.vt = b.vt = VT_BSTR;
            a.bstrVal = SysAllocString(before.text.c_str()); b.bstrVal = SysAllocString(after.text.c_str());
            raise(UIA_ValueValuePropertyId, a, b);
            VariantClear(&a); VariantClear(&b);
        }
        if (before.expanded != after.expanded)
        {
            raise(UIA_ExpandCollapseExpandCollapseStatePropertyId, integer(before.expanded ? ExpandCollapseState_Expanded : ExpandCollapseState_Collapsed),
                integer(after.expanded ? ExpandCollapseState_Expanded : ExpandCollapseState_Collapsed));
            ::NotifyWinEvent(EVENT_OBJECT_STATECHANGE, m_hWnd, OBJID_CLIENT, CHILDID_SELF);
        }
        if (before.enabled != after.enabled) ::NotifyWinEvent(EVENT_OBJECT_STATECHANGE, m_hWnd, OBJID_CLIENT, CHILDID_SELF);
        if (before.name != after.name) ::NotifyWinEvent(EVENT_OBJECT_NAMECHANGE, m_hWnd, OBJID_CLIENT, CHILDID_SELF);
    }

    LRESULT InputWindow::WindowProc(UINT message, WPARAM w, LPARAM l)
    {
        if (message == WM_SETTEXT || message == WM_SETFONT || message == WM_WINDOWPOSCHANGED || message == WM_ENABLE || message == WM_SHOWWINDOW
            || message == WM_MOUSEMOVE || message == WM_MOUSELEAVE || message == WM_LBUTTONDOWN || message == WM_LBUTTONUP
            || message == WM_KEYDOWN || message == WM_KEYUP || message == WM_CAPTURECHANGED)
            NativeLayoutObservation::Invalidate(m_hWnd);
        return CWnd::WindowProc(message, w, l);
    }

    void InputWindow::OnPaint()
    {
        CPaintDC paint(this);
        m_drawResult = PaintTo(paint.GetSafeHdc());
    }
    LRESULT InputWindow::OnPrintClient(WPARAM dc, LPARAM flags)
    {
        if (!dc || ((flags & PRF_CHECKVISIBLE) && !IsWindowVisible())) return 0;
        m_printResult = PaintTo(reinterpret_cast<HDC>(dc), true);
        return 0;
    }

    HRESULT InputWindow::PaintTo(HDC dc, bool printing)
    {
        NativeLayoutObservation::PaintScope observation(m_hWnd, dc, printing ? NativeLayoutObservation::Route::PrintClient : NativeLayoutObservation::Route::WindowPaint);
        Microsoft::WRL::ComPtr<ID2D1Factory> factory;
        Microsoft::WRL::ComPtr<ID2D1DCRenderTarget> target;
        HRESULT result = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, factory.GetAddressOf());
        const float dpi = static_cast<float>(Dpi());
        auto properties = D2D1::RenderTargetProperties(D2D1_RENDER_TARGET_TYPE_DEFAULT,
            D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE), dpi, dpi);
        if (SUCCEEDED(result)) result = factory->CreateDCRenderTarget(&properties, target.GetAddressOf());
        m_attachResult = result;
        CRect bounds;
        GetClientRect(bounds);
        if (SUCCEEDED(result)) result = target->BindDC(dc, bounds);
        if (SUCCEEDED(result))
        {
            const auto palette = Palette::Resolve(m_appearance);
            target->BeginDraw();
            target->Clear(m_customSurface
                ? D2D1::ColorF(GetRValue(m_surfaceColor) / 255.f, GetGValue(m_surfaceColor) / 255.f, GetBValue(m_surfaceColor) / 255.f)
                : palette.surface);
            Paint(target.Get(), palette, State());
            result = target->EndDraw();
        }
        observation.Complete(result);
        return result;
    }

    void InputWindow::OnLButtonDown(UINT, CPoint point)
    {
        if (!IsWindowEnabled()) return;
        if (DialogCode() != 0) SetFocus();
        SetCapture();
        m_pressed = true;
        PointerPressed(point);
        Invalidate(FALSE);
    }
    void InputWindow::OnLButtonUp(UINT, CPoint point)
    {
        const bool wasPressed = m_pressed;
        m_pressed = false;
        if (GetCapture() == this) ReleaseCapture();
        CRect bounds;
        GetClientRect(bounds);
        if (wasPressed) PointerReleased(point, bounds.PtInRect(point) != FALSE);
        if (m_hWnd) Invalidate(FALSE);
    }
    void InputWindow::OnMouseMove(UINT flags, CPoint point)
    {
        CRect bounds;
        GetClientRect(bounds);
        m_hover = bounds.PtInRect(point) != FALSE;
        TRACKMOUSEEVENT track{ sizeof(track), TME_LEAVE, m_hWnd, 0 };
        TrackMouseEvent(&track);
        PointerMoved(point);
        Invalidate(FALSE);
        CWnd::OnMouseMove(flags, point);
    }
    LRESULT InputWindow::OnMouseLeave(WPARAM, LPARAM)
    {
        m_hover = false;
        PointerLeft();
        Invalidate(FALSE);
        return 0;
    }
    void InputWindow::OnCaptureChanged(CWnd* next)
    {
        if (m_pressed) { m_pressed = false; PointerCanceled(); }
        Invalidate(FALSE);
        CWnd::OnCaptureChanged(next);
    }
    void InputWindow::OnKeyDown(UINT key, UINT repeat, UINT flags)
    {
        if (!IsWindowEnabled() || !KeyPressed(key, false)) CWnd::OnKeyDown(key, repeat, flags);
        if (m_hWnd) Invalidate(FALSE);
    }
    void InputWindow::OnSysKeyDown(UINT key, UINT repeat, UINT flags)
    {
        if (!IsWindowEnabled() || !KeyPressed(key, true)) CWnd::OnSysKeyDown(key, repeat, flags);
        if (m_hWnd) Invalidate(FALSE);
    }
    void InputWindow::OnKeyUp(UINT key, UINT repeat, UINT flags)
    {
        if (!IsWindowEnabled() || !KeyReleased(key)) CWnd::OnKeyUp(key, repeat, flags);
        if (m_hWnd) Invalidate(FALSE);
    }
    void InputWindow::OnChar(UINT ch, UINT repeat, UINT flags)
    {
        if (!IsWindowEnabled() || !CharTyped(ch)) CWnd::OnChar(ch, repeat, flags);
    }
    BOOL InputWindow::OnMouseWheel(UINT flags, short delta, CPoint point)
    {
        if (IsWindowEnabled() && MouseWheel(delta)) { Invalidate(FALSE); return TRUE; }
        return CWnd::OnMouseWheel(flags, delta, point);
    }
    void InputWindow::OnSetFocus(CWnd* old)
    {
        CWnd::OnSetFocus(old);
        NativeLayoutObservation::Invalidate(m_hWnd);
        Invalidate(FALSE);
        ::NotifyWinEvent(EVENT_OBJECT_FOCUS, m_hWnd, OBJID_CLIENT, CHILDID_SELF);
    }
    void InputWindow::OnKillFocus(CWnd* next)
    {
        m_pressed = false;
        CWnd::OnKillFocus(next);
        NativeLayoutObservation::Invalidate(m_hWnd);
        Invalidate(FALSE);
    }
    void InputWindow::OnEnable(BOOL enabled)
    {
        CWnd::OnEnable(enabled);
        m_pressed = false;
        Publish();
        Invalidate(FALSE);
    }
    UINT InputWindow::OnGetDlgCode()
    {
        UINT code = DialogCode();
        if (const auto* message = GetCurrentMessage())
            if (const auto* key = reinterpret_cast<const MSG*>(message->lParam))
                if ((key->message == WM_KEYDOWN || key->message == WM_KEYUP || key->message == WM_CHAR) && WantsKey(static_cast<UINT>(key->wParam)))
                    code |= DLGC_WANTMESSAGE;
        return code;
    }
    void InputWindow::OnDestroy()
    {
        NativeLayoutObservation::Unregister(m_hWnd);
        if (m_access) { std::lock_guard<std::mutex> lock(m_access->mutex); m_access->window = nullptr; }
        UiaReturnRawElementProvider(m_hWnd, 0, 0, nullptr);
        CWnd::OnDestroy();
    }
    LRESULT InputWindow::OnGetObject(WPARAM w, LPARAM l)
    {
        if (l != UiaRootObjectId || !m_access) return Default();
        auto* provider = new InputProvider(m_access);
        const LRESULT result = UiaReturnRawElementProvider(m_hWnd, w, l, provider);
        provider->Release();
        return result;
    }
    LRESULT InputWindow::OnInputAction(WPARAM action, LPARAM value)
    {
        if (IsWindowEnabled()) PerformAction(static_cast<InputActionId>(action), static_cast<int>(value));
        return 0;
    }
    LRESULT InputWindow::OnDpiAfterParent(WPARAM, LPARAM)
    {
        DpiChanged();
        NativeLayoutObservation::Invalidate(m_hWnd);
        Invalidate(FALSE);
        return 0;
    }
}
