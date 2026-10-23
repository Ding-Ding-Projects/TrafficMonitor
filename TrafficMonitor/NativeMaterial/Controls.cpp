#include "../stdafx.h"
#include "Controls.h"
#include <UIAutomation.h>
#include <atomic>
#include <mutex>
#pragma comment(lib, "uiautomationcore.lib")

namespace NativeMaterial
{
    struct ButtonAccess
    {
        std::mutex mutex;
        HWND window{};
        std::wstring label;
        bool enabled{ true };
    };

    // Each real child HWND participates in the system HWND fragment tree.
    // Custom virtual tables require their own fragment providers separately.
    class ButtonProvider final : public IRawElementProviderSimple, public IInvokeProvider
    {
    public:
        explicit ButtonProvider(std::shared_ptr<ButtonAccess> state) : m_state(std::move(state)) {}
        HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void** value) override
        {
            if (!value) return E_POINTER;
            *value = nullptr;
            if (iid == __uuidof(IUnknown) || iid == __uuidof(IRawElementProviderSimple))
                *value = static_cast<IRawElementProviderSimple*>(this);
            else if (iid == __uuidof(IInvokeProvider)) *value = static_cast<IInvokeProvider*>(this);
            else return E_NOINTERFACE;
            AddRef(); return S_OK;
        }
        ULONG STDMETHODCALLTYPE AddRef() override { return ++m_references; }
        ULONG STDMETHODCALLTYPE Release() override
        {
            const auto remaining = --m_references;
            if (!remaining) delete this;
            return remaining;
        }
        HRESULT STDMETHODCALLTYPE get_ProviderOptions(ProviderOptions* value) override
        {
            if (!value) return E_POINTER;
            *value = ProviderOptions_ServerSideProvider; return S_OK;
        }
        HRESULT STDMETHODCALLTYPE GetPatternProvider(PATTERNID id, IUnknown** value) override
        {
            if (!value) return E_POINTER;
            *value = nullptr;
            if (id == UIA_InvokePatternId) return QueryInterface(__uuidof(IInvokeProvider), reinterpret_cast<void**>(value));
            return S_OK;
        }
        HRESULT STDMETHODCALLTYPE GetPropertyValue(PROPERTYID id, VARIANT* value) override
        {
            if (!value) return E_POINTER;
            VariantInit(value);
            std::lock_guard<std::mutex> lock(m_state->mutex);
            if (!m_state->window) return UIA_E_ELEMENTNOTAVAILABLE;
            if (id == UIA_ControlTypePropertyId) { value->vt = VT_I4; value->lVal = UIA_ButtonControlTypeId; }
            else if (id == UIA_NamePropertyId)
            {
                value->vt = VT_BSTR; value->bstrVal = SysAllocString(m_state->label.c_str());
                if (!value->bstrVal) return E_OUTOFMEMORY;
            }
            else if (id == UIA_IsEnabledPropertyId || id == UIA_IsKeyboardFocusablePropertyId
                || id == UIA_IsControlElementPropertyId || id == UIA_IsContentElementPropertyId)
            {
                value->vt = VT_BOOL;
                value->boolVal = id == UIA_IsEnabledPropertyId && !m_state->enabled ? VARIANT_FALSE : VARIANT_TRUE;
            }
            return S_OK;
        }
        HRESULT STDMETHODCALLTYPE get_HostRawElementProvider(IRawElementProviderSimple** value) override
        {
            if (!value) return E_POINTER;
            *value = nullptr;
            std::lock_guard<std::mutex> lock(m_state->mutex);
            if (!m_state->window) return UIA_E_ELEMENTNOTAVAILABLE;
            return UiaHostProviderFromHwnd(m_state->window, value);
        }
        HRESULT STDMETHODCALLTYPE Invoke() override
        {
            std::lock_guard<std::mutex> lock(m_state->mutex);
            if (!m_state->window) return UIA_E_ELEMENTNOTAVAILABLE;
            if (!m_state->enabled) return UIA_E_ELEMENTNOTENABLED;
            return PostMessageW(m_state->window, BM_CLICK, 0, 0) ? S_OK : HRESULT_FROM_WIN32(GetLastError());
        }
    private:
        std::atomic<ULONG> m_references{ 1 };
        std::shared_ptr<ButtonAccess> m_state;
    };

    BEGIN_MESSAGE_MAP(Button, CWnd)
        ON_WM_PAINT()
        ON_WM_ERASEBKGND()
        ON_WM_LBUTTONDOWN()
        ON_WM_LBUTTONUP()
        ON_WM_KEYDOWN()
        ON_WM_KEYUP()
        ON_WM_SETFOCUS()
        ON_WM_KILLFOCUS()
        ON_WM_ENABLE()
        ON_WM_DESTROY()
        ON_WM_GETDLGCODE()
        ON_MESSAGE(WM_GETOBJECT, &Button::OnGetObject)
        ON_MESSAGE(BM_CLICK, &Button::OnClick)
    END_MESSAGE_MAP()

    BOOL Button::Create(const CString& label, const RECT& bounds, CWnd* parent, UINT id)
    {
        m_access = std::make_shared<ButtonAccess>();
        m_access->label = label.GetString();
        const auto className = AfxRegisterWndClass(CS_HREDRAW | CS_VREDRAW, LoadCursor(nullptr, IDC_ARROW));
        if (!CWnd::CreateEx(0, className, label, WS_CHILD | WS_VISIBLE | WS_TABSTOP, bounds, parent, id)) return FALSE;
        m_access->window = m_hWnd;
        return TRUE;
    }
    void Button::SetAppearance(Appearance appearance) { m_appearance = appearance; Invalidate(); }
    void Button::SetLabel(const CString& label)
    {
        SetWindowText(label);
        { std::lock_guard<std::mutex> lock(m_access->mutex); m_access->label = label.GetString(); }
        Invalidate();
    }
    BOOL Button::OnEraseBkgnd(CDC*) { return TRUE; }
    void Button::OnPaint()
    {
        CPaintDC paint(this);
        const auto colors = Palette::Resolve(m_appearance);
        const float dpi = static_cast<float>(GetDpiForWindow(m_hWnd));
        if (!m_canvas.Target() && FAILED(m_canvas.Attach(m_hWnd, dpi))) return;
        CRect bounds; GetClientRect(bounds);
        m_canvas.Resize(bounds.Width(), bounds.Height(), dpi);
        if (FAILED(m_canvas.Begin(colors.surface))) return;
        const auto size = m_canvas.Target()->GetSize();
        const auto rect = D2D1::RectF(3, 3, size.width - 3, size.height - 3);
        const auto shape = D2D1::RoundedRect(rect, 20, 20);
        m_canvas.Target()->FillRoundedRectangle(shape, m_canvas.Brush(IsWindowEnabled() ? colors.primary : colors.outline));
        if (m_pressed)
        {
            auto state = colors.onPrimary; state.a = 0.12f;
            m_canvas.Target()->FillRoundedRectangle(shape, m_canvas.Brush(state));
        }
        if (GetFocus() == this) m_canvas.Target()->DrawRoundedRectangle(
            D2D1::RoundedRect(D2D1::RectF(1, 1, size.width - 1, size.height - 1), 22, 22), m_canvas.Brush(colors.primary), 2);
        Microsoft::WRL::ComPtr<IDWriteTextFormat> format;
        if (SUCCEEDED(m_canvas.TextFormat(14, DWRITE_FONT_WEIGHT_MEDIUM, format.GetAddressOf())))
        {
            format->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
            format->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
            CString label; GetWindowText(label);
            m_canvas.Target()->DrawText(label, label.GetLength(), format.Get(),
                D2D1::RectF(16, 5, size.width - 16, size.height - 5), m_canvas.Brush(colors.onPrimary));
        }
        if (FAILED(m_canvas.End())) Invalidate(FALSE);
    }
    void Button::OnLButtonDown(UINT, CPoint) { if (IsWindowEnabled()) { SetFocus(); SetCapture(); m_pressed = true; Invalidate(); } }
    void Button::OnLButtonUp(UINT, CPoint point)
    {
        const bool pressed = m_pressed; m_pressed = false;
        if (GetCapture() == this) ReleaseCapture();
        Invalidate(); CRect bounds; GetClientRect(bounds);
        if (pressed && bounds.PtInRect(point)) OnClick(0, 0);
    }
    UINT Button::OnGetDlgCode() { return DLGC_BUTTON; }
    void Button::OnKeyDown(UINT key, UINT repeat, UINT flags)
    {
        if ((key == VK_SPACE || key == VK_RETURN) && !(flags & 0x4000)) { m_pressed = true; Invalidate(); }
        else CWnd::OnKeyDown(key, repeat, flags);
    }
    void Button::OnKeyUp(UINT key, UINT repeat, UINT flags)
    {
        if ((key == VK_SPACE || key == VK_RETURN) && m_pressed) { m_pressed = false; Invalidate(); OnClick(0, 0); }
        else CWnd::OnKeyUp(key, repeat, flags);
    }
    void Button::OnSetFocus(CWnd* old) { CWnd::OnSetFocus(old); Invalidate(); ::NotifyWinEvent(EVENT_OBJECT_FOCUS, m_hWnd, OBJID_CLIENT, CHILDID_SELF); }
    void Button::OnKillFocus(CWnd* next) { m_pressed = false; CWnd::OnKillFocus(next); Invalidate(); }
    void Button::OnEnable(BOOL enabled)
    {
        CWnd::OnEnable(enabled);
        { std::lock_guard<std::mutex> lock(m_access->mutex); m_access->enabled = enabled != FALSE; }
        m_pressed = false; Invalidate();
    }
    void Button::OnDestroy()
    {
        { std::lock_guard<std::mutex> lock(m_access->mutex); m_access->window = nullptr; }
        m_canvas.Detach(); CWnd::OnDestroy();
    }
    LRESULT Button::OnGetObject(WPARAM wparam, LPARAM lparam)
    {
        if (lparam != UiaRootObjectId) return Default();
        auto provider = new ButtonProvider(m_access);
        const LRESULT result = UiaReturnRawElementProvider(m_hWnd, wparam, lparam, provider);
        provider->Release(); return result;
    }
    LRESULT Button::OnClick(WPARAM, LPARAM)
    {
        if (IsWindowEnabled() && GetParent())
            GetParent()->SendMessage(WM_COMMAND, MAKEWPARAM(GetDlgCtrlID(), BN_CLICKED), reinterpret_cast<LPARAM>(m_hWnd));
        return 0;
    }
}
