#include "../stdafx.h"
#include "AlertDialog.h"
#include "Controls.h"
#include "../NativePreferences/Text.h"
#include <UIAutomation.h>
#include <algorithm>
#include <atomic>
#include <cmath>
#include <memory>
#include <mutex>
#include <string>
#pragma comment(lib, "uiautomationcore.lib")

namespace NativeMaterial
{
    namespace
    {
        constexpr UINT FirstActionId = 5200;
        const wchar_t* const InspectProperty = L"NativeMaterial.AlertDialog";
        const AlertTestHooks* g_hooks{};

        int Px(int dp, UINT dpi) { return MulDiv(dp, static_cast<int>(dpi), 96); }
        COLORREF Rgb(D2D1_COLOR_F c) { return RGB(BYTE(c.r * 255 + .5f), BYTE(c.g * 255 + .5f), BYTE(c.b * 255 + .5f)); }
        int Width(const RECT& r) { return static_cast<int>(r.right - r.left); }
        int Height(const RECT& r) { return static_cast<int>(r.bottom - r.top); }
        int Place(int value, int low, int high) { return (std::max)(low, (std::min)(value, high)); }

        CString Words(const wchar_t* english, const wchar_t* cantonese)
        {
            return NativePreferences::InlinePair(english, cantonese);
        }
        CString StandardLabel(int result)
        {
            switch (result)
            {
            case IDOK: return Words(L"OK", L"確定");
            case IDCANCEL: return Words(L"Cancel", L"取消");
            case IDYES: return Words(L"Yes", L"係");
            case IDNO: return Words(L"No", L"唔係");
            case IDRETRY: return Words(L"Retry", L"重試");
            case IDABORT: return Words(L"Abort", L"中止");
            case IDIGNORE: return Words(L"Ignore", L"忽略");
            case IDTRYAGAIN: return Words(L"Try again", L"再試");
            case IDCONTINUE: return Words(L"Continue", L"繼續");
            default: return Words(L"OK", L"確定");
            }
        }
        CString DefaultHeadline(AlertIcon icon)
        {
            switch (icon)
            {
            case AlertIcon::Error: return Words(L"Error", L"錯誤");
            case AlertIcon::Warning: return Words(L"Warning", L"警告");
            case AlertIcon::Information: return Words(L"Information", L"資訊");
            case AlertIcon::Question: return Words(L"Confirm", L"確認");
            default: return CString(L"TrafficMonitor");
            }
        }

        enum class Style { Headline, Body, Action, Glyph };
        struct StyleSpec { float size, line; DWRITE_FONT_WEIGHT weight; int gdiWeight; };
        StyleSpec Spec(Style style)
        {
            switch (style)
            {
            case Style::Headline: return { 24.f, 32.f, DWRITE_FONT_WEIGHT_NORMAL, FW_NORMAL };
            case Style::Action: return { 14.f, 20.f, DWRITE_FONT_WEIGHT_MEDIUM, FW_MEDIUM };
            case Style::Glyph: return { 16.f, 24.f, DWRITE_FONT_WEIGHT_SEMI_BOLD, FW_SEMIBOLD };
            default: return { 14.f, 20.f, DWRITE_FONT_WEIGHT_NORMAL, FW_NORMAL };
            }
        }

        // Text measurement and layout. DirectWrite is the normal route and uses the same face
        // as the Material buttons; the GDI route is the same design drawn without Direct2D.
        class Typography
        {
        public:
            struct Extent { int width{}, height{}, lines{ 1 }; };
            Typography(UINT dpi, bool gdi) : m_dpi(dpi), m_gdi(gdi)
            {
                if (!m_gdi && FAILED(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory),
                    reinterpret_cast<IUnknown**>(m_factory.GetAddressOf())))) m_gdi = true;
            }
            bool Gdi() const { return m_gdi; }
            UINT Dpi() const { return m_dpi; }
            float SizePx(Style s) const { return Spec(s).size * m_dpi / 96.f; }
            float LinePx(Style s) const { return Spec(s).line * m_dpi / 96.f; }

            Microsoft::WRL::ComPtr<IDWriteTextLayout> Layout(const CString& text, Style style, float width, float height, bool center) const
            {
                Microsoft::WRL::ComPtr<IDWriteTextFormat> format;
                Microsoft::WRL::ComPtr<IDWriteTextLayout> layout;
                if (!m_factory) return layout;
                const auto spec = Spec(style);
                if (FAILED(m_factory->CreateTextFormat(L"Segoe UI", nullptr, spec.weight, DWRITE_FONT_STYLE_NORMAL,
                    DWRITE_FONT_STRETCH_NORMAL, SizePx(style), L"", format.GetAddressOf()))) return layout;
                format->SetLineSpacing(DWRITE_LINE_SPACING_METHOD_UNIFORM, LinePx(style), LinePx(style) * .8f);
                format->SetTextAlignment(center ? DWRITE_TEXT_ALIGNMENT_CENTER : DWRITE_TEXT_ALIGNMENT_LEADING);
                format->SetWordWrapping(DWRITE_WORD_WRAPPING_WRAP);
                if (FAILED(m_factory->CreateTextLayout(text, static_cast<UINT32>(text.GetLength()), format.Get(),
                    (std::max)(1.f, width), (std::max)(1.f, height), layout.GetAddressOf()))) layout.Reset();
                return layout;
            }

            HFONT CreateGdiFont(Style style) const
            {
                const auto spec = Spec(style);
                return CreateFontW(-static_cast<int>(SizePx(style) + .5f), 0, 0, 0, spec.gdiWeight, FALSE, FALSE, FALSE,
                    DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
            }

            // maxWidth 0 measures the natural single-paragraph width.
            Extent Measure(const CString& text, Style style, int maxWidth) const
            {
                Extent e;
                if (text.IsEmpty()) { e.lines = 0; return e; }
                if (!m_gdi)
                {
                    auto layout = Layout(text, style, maxWidth > 0 ? static_cast<float>(maxWidth) : 100000.f, 100000.f, false);
                    DWRITE_TEXT_METRICS metrics{};
                    if (layout && SUCCEEDED(layout->GetMetrics(&metrics)))
                    {
                        e.width = static_cast<int>(std::ceil(metrics.width));
                        e.height = static_cast<int>(std::ceil(metrics.height));
                        e.lines = static_cast<int>(metrics.lineCount);
                        return e;
                    }
                }
                HDC dc = GetDC(nullptr);
                HFONT font = CreateGdiFont(style);
                HGDIOBJ old = SelectObject(dc, font);
                TEXTMETRICW tm{};
                GetTextMetricsW(dc, &tm);
                RECT bounds{ 0, 0, maxWidth > 0 ? maxWidth : 100000, 0 };
                DrawTextW(dc, text, -1, &bounds, DT_CALCRECT | DT_WORDBREAK | DT_NOPREFIX | DT_EXPANDTABS | DT_EDITCONTROL);
                SelectObject(dc, old);
                DeleteObject(font);
                ReleaseDC(nullptr, dc);
                e.width = bounds.right;
                e.height = bounds.bottom;
                e.lines = (std::max)(1, static_cast<int>((bounds.bottom + tm.tmHeight / 2) / (std::max)(1L, tm.tmHeight)));
                return e;
            }
        private:
            UINT m_dpi;
            bool m_gdi;
            Microsoft::WRL::ComPtr<IDWriteFactory> m_factory;
        };

        ButtonVariant VariantFor(const AlertSpec& spec, size_t index)
        {
            if (spec.actions[index].destructive) return ButtonVariant::Destructive;
            return static_cast<int>(index) == spec.defaultIndex ? ButtonVariant::Primary : ButtonVariant::Text;
        }

        AlertLayout ComputeLayout(const AlertSpec& spec, UINT dpi, SIZE work, const Typography& type)
        {
            AlertLayout layout;
            const int pad = Px(24, dpi), gap8 = Px(8, dpi), gap16 = Px(16, dpi), iconSize = Px(24, dpi);
            const int margin = Px(3, dpi), visibleHeight = Px(40, dpi);
            const int maxWidth = (std::max)(Px(200, dpi), (std::min)(Px(560, dpi), static_cast<int>(work.cx) - 2 * gap16));
            const int minWidth = (std::min)(Px(280, dpi), maxWidth);
            const size_t count = spec.actions.size();

            std::vector<int> buttonWidths;
            int rowWidth = 0;
            for (size_t i = 0; i < count; ++i)
            {
                const auto variant = VariantFor(spec, i);
                const int inset = variant == ButtonVariant::Primary ? 24 : 12;
                const int text = type.Measure(spec.actions[i].label, Style::Action, 0).width;
                buttonWidths.push_back((std::max)(Px(64, dpi), text + 2 * Px(inset, dpi)) + 2 * margin);
                rowWidth += buttonWidths.back() - 2 * margin;
            }
            if (count > 1) rowWidth += gap8 * static_cast<int>(count - 1);

            const bool hasBody = !spec.body.IsEmpty();
            const bool hasIcon = spec.icon != AlertIcon::None;
            const int natural = (std::max)({ type.Measure(spec.headline, Style::Headline, 0).width,
                hasBody ? type.Measure(spec.body, Style::Body, 0).width : 0, rowWidth });
            const int width = (std::min)(maxWidth, (std::max)(minWidth, natural + 2 * pad));
            const int contentWidth = width - 2 * pad;

            const auto head = type.Measure(spec.headline, Style::Headline, contentWidth);
            const auto body = hasBody ? type.Measure(spec.body, Style::Body, contentWidth) : Typography::Extent{ 0, 0, 0 };
            layout.stacked = rowWidth > contentWidth;
            const int actionsHeight = layout.stacked
                ? static_cast<int>(count) * visibleHeight + gap8 * static_cast<int>(count - 1) : visibleHeight;

            int y = pad;
            if (hasIcon)
            {
                layout.icon = { (width - iconSize) / 2, y, (width - iconSize) / 2 + iconSize, y + iconSize };
                y += iconSize + gap16;
            }
            layout.headline = { pad, y, pad + contentWidth, y + head.height };
            y += head.height;
            const int maxHeight = work.cy * 85 / 100;
            const int below = pad + actionsHeight + pad;
            const int maxBody = (std::max)(Px(72, dpi), maxHeight - y - (hasBody ? gap16 : 0) - below);
            if (hasBody)
            {
                y += gap16;
                const int view = (std::min)(body.height, maxBody);
                layout.bodyView = { pad, y, pad + contentWidth, y + view };
                layout.scrolls = body.height > maxBody;
                y += view;
            }
            layout.bodyContentHeight = body.height;
            layout.bodyLines = body.lines;
            layout.bodyTextWidth = body.width;
            y += pad;
            const int actionsTop = y;
            if (layout.stacked)
            {
                for (size_t i = 0; i < count; ++i)
                {
                    const int top = actionsTop + static_cast<int>(i) * (visibleHeight + gap8);
                    layout.actions.push_back({ pad - margin, top - margin, width - pad + margin, top + visibleHeight + margin });
                }
            }
            else
            {
                layout.actions.resize(count);
                int right = width - pad + margin;
                for (size_t i = count; i-- > 0;)
                {
                    const int left = right - buttonWidths[i];
                    layout.actions[i] = { left, actionsTop - margin, right, actionsTop + visibleHeight + margin };
                    right = left + 2 * margin - gap8;
                }
            }
            y += actionsHeight + pad;
            layout.size = { width, y };
            return layout;
        }

        D2D1_COLOR_F SurfaceColor(const Palette& p)
        {
            return p.highContrast ? p.surface : BlendColor(p.onSurface, p.surfaceContainer, .04f);
        }

        // One drawing surface per backend, so the design is written once.
        class Gfx
        {
        public:
            virtual ~Gfx() = default;
            virtual void Line(float x1, float y1, float x2, float y2, float width, D2D1_COLOR_F color) = 0;
            virtual void Circle(float x, float y, float radius, float width, D2D1_COLOR_F color, bool fill) = 0;
            virtual void RoundRect(const RECT& r, float radius, D2D1_COLOR_F color) = 0;
            virtual void Clip(const RECT& r) = 0;
            virtual void Unclip() = 0;
            virtual void Text(const CString& text, const RECT& r, Style style, D2D1_COLOR_F color, bool center) = 0;
        };

        class D2dGfx final : public Gfx
        {
        public:
            D2dGfx(Canvas& canvas, const Typography& type) : m_canvas(canvas), m_type(type) {}
            void Line(float x1, float y1, float x2, float y2, float width, D2D1_COLOR_F color) override
            {
                if (auto* b = m_canvas.Brush(color)) m_canvas.Target()->DrawLine(D2D1::Point2F(x1, y1), D2D1::Point2F(x2, y2), b, width);
            }
            void Circle(float x, float y, float radius, float width, D2D1_COLOR_F color, bool fill) override
            {
                auto* b = m_canvas.Brush(color);
                if (!b) return;
                const auto shape = D2D1::Ellipse(D2D1::Point2F(x, y), radius, radius);
                if (fill) m_canvas.Target()->FillEllipse(shape, b); else m_canvas.Target()->DrawEllipse(shape, b, width);
            }
            void RoundRect(const RECT& r, float radius, D2D1_COLOR_F color) override
            {
                if (auto* b = m_canvas.Brush(color))
                    m_canvas.Target()->FillRoundedRectangle(D2D1::RoundedRect(D2D1::RectF(static_cast<float>(r.left), static_cast<float>(r.top),
                        static_cast<float>(r.right), static_cast<float>(r.bottom)), radius, radius), b);
            }
            void Clip(const RECT& r) override
            {
                m_canvas.Target()->PushAxisAlignedClip(D2D1::RectF(static_cast<float>(r.left), static_cast<float>(r.top),
                    static_cast<float>(r.right), static_cast<float>(r.bottom)), D2D1_ANTIALIAS_MODE_ALIASED);
                ++m_clips;
            }
            void Unclip() override { if (m_clips) { m_canvas.Target()->PopAxisAlignedClip(); --m_clips; } }
            void Text(const CString& text, const RECT& r, Style style, D2D1_COLOR_F color, bool center) override
            {
                auto layout = m_type.Layout(text, style, static_cast<float>(Width(r)), static_cast<float>(Height(r)), center);
                auto* b = m_canvas.Brush(color);
                if (layout && b) m_canvas.Target()->DrawTextLayout(D2D1::Point2F(static_cast<float>(r.left), static_cast<float>(r.top)), layout.Get(), b);
            }
        private:
            Canvas& m_canvas;
            const Typography& m_type;
            int m_clips{};
        };

        class GdiGfx final : public Gfx
        {
        public:
            GdiGfx(HDC dc, const Typography& type) : m_dc(dc), m_type(type) { SetBkMode(m_dc, TRANSPARENT); }
            void Line(float x1, float y1, float x2, float y2, float width, D2D1_COLOR_F color) override
            {
                HPEN pen = CreatePen(PS_SOLID, (std::max)(1, static_cast<int>(width + .5f)), Rgb(color));
                HGDIOBJ old = SelectObject(m_dc, pen);
                MoveToEx(m_dc, static_cast<int>(x1 + .5f), static_cast<int>(y1 + .5f), nullptr);
                LineTo(m_dc, static_cast<int>(x2 + .5f), static_cast<int>(y2 + .5f));
                SelectObject(m_dc, old);
                DeleteObject(pen);
            }
            void Circle(float x, float y, float radius, float width, D2D1_COLOR_F color, bool fill) override
            {
                HPEN pen = CreatePen(fill ? PS_NULL : PS_SOLID, (std::max)(1, static_cast<int>(width + .5f)), Rgb(color));
                HBRUSH brush = fill ? CreateSolidBrush(Rgb(color)) : static_cast<HBRUSH>(GetStockObject(NULL_BRUSH));
                HGDIOBJ oldPen = SelectObject(m_dc, pen), oldBrush = SelectObject(m_dc, brush);
                Ellipse(m_dc, static_cast<int>(x - radius), static_cast<int>(y - radius), static_cast<int>(x + radius) + 1, static_cast<int>(y + radius) + 1);
                SelectObject(m_dc, oldPen);
                SelectObject(m_dc, oldBrush);
                DeleteObject(pen);
                if (fill) DeleteObject(brush);
            }
            void RoundRect(const RECT& r, float radius, D2D1_COLOR_F color) override
            {
                HPEN pen = CreatePen(PS_NULL, 0, 0);
                HBRUSH brush = CreateSolidBrush(Rgb(color));
                HGDIOBJ oldPen = SelectObject(m_dc, pen), oldBrush = SelectObject(m_dc, brush);
                ::RoundRect(m_dc, r.left, r.top, r.right + 1, r.bottom + 1, static_cast<int>(radius * 2), static_cast<int>(radius * 2));
                SelectObject(m_dc, oldPen);
                SelectObject(m_dc, oldBrush);
                DeleteObject(pen);
                DeleteObject(brush);
            }
            void Clip(const RECT& r) override
            {
                SaveDC(m_dc);
                IntersectClipRect(m_dc, r.left, r.top, r.right, r.bottom);
                ++m_saved;
            }
            void Unclip() override { if (m_saved) { RestoreDC(m_dc, -1); --m_saved; } }
            void Text(const CString& text, const RECT& r, Style style, D2D1_COLOR_F color, bool center) override
            {
                HFONT font = m_type.CreateGdiFont(style);
                HGDIOBJ old = SelectObject(m_dc, font);
                SetTextColor(m_dc, Rgb(color));
                RECT bounds = r;
                DrawTextW(m_dc, text, -1, &bounds, DT_WORDBREAK | DT_NOPREFIX | DT_EXPANDTABS | DT_EDITCONTROL | (center ? DT_CENTER : DT_LEFT));
                SelectObject(m_dc, old);
                DeleteObject(font);
            }
        private:
            HDC m_dc;
            const Typography& m_type;
            int m_saved{};
        };

        void DrawIcon(Gfx& g, const Palette& p, AlertIcon icon, const RECT& box, UINT dpi)
        {
            if (icon == AlertIcon::None) return;
            const float s = static_cast<float>(Width(box)), cx = box.left + s / 2, cy = box.top + s / 2;
            const float stroke = (std::max)(1.5f, 2.f * dpi / 96.f), dot = (std::max)(1.5f, 1.4f * dpi / 96.f);
            const auto color = icon == AlertIcon::Error ? p.error : p.primary;
            if (icon == AlertIcon::Warning)
            {
                const float topY = cy - s * .44f, baseY = cy + s * .4f, half = s * .5f;
                g.Line(cx, topY, cx - half, baseY, stroke, color);
                g.Line(cx, topY, cx + half, baseY, stroke, color);
                g.Line(cx - half, baseY, cx + half, baseY, stroke, color);
                g.Line(cx, cy - s * .12f, cx, cy + s * .14f, stroke, color);
                g.Circle(cx, cy + s * .27f, dot, 0, color, true);
                return;
            }
            g.Circle(cx, cy, s / 2 - stroke, stroke, color, false);
            if (icon == AlertIcon::Information)
            {
                g.Circle(cx, cy - s * .22f, dot, 0, color, true);
                g.Line(cx, cy - s * .05f, cx, cy + s * .26f, stroke, color);
            }
            else if (icon == AlertIcon::Error)
            {
                g.Line(cx, cy - s * .27f, cx, cy + s * .06f, stroke, color);
                g.Circle(cx, cy + s * .24f, dot, 0, color, true);
            }
            else
                g.Text(CString(L"?"), box, Style::Glyph, color, true);
        }

        // Accessible name is the headline and the description is the supporting text.
        struct AlertAccess
        {
            std::mutex mutex;
            HWND window{};
            std::wstring name, description;
        };
        class AlertProvider final : public IRawElementProviderSimple
        {
        public:
            explicit AlertProvider(std::shared_ptr<AlertAccess> state) : m_state(std::move(state)) {}
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
            ULONG STDMETHODCALLTYPE Release() override
            {
                const auto remaining = --m_references;
                if (!remaining) delete this;
                return remaining;
            }
            HRESULT STDMETHODCALLTYPE get_ProviderOptions(ProviderOptions* value) override
            {
                if (!value) return E_POINTER;
                *value = ProviderOptions_ServerSideProvider;
                return S_OK;
            }
            HRESULT STDMETHODCALLTYPE GetPatternProvider(PATTERNID, IUnknown** value) override
            {
                if (!value) return E_POINTER;
                *value = nullptr;
                return S_OK;
            }
            HRESULT STDMETHODCALLTYPE GetPropertyValue(PROPERTYID id, VARIANT* value) override
            {
                if (!value) return E_POINTER;
                VariantInit(value);
                std::lock_guard<std::mutex> lock(m_state->mutex);
                if (!m_state->window) return UIA_E_ELEMENTNOTAVAILABLE;
                auto text = [&](const std::wstring& s) { value->vt = VT_BSTR; value->bstrVal = SysAllocString(s.c_str()); return value->bstrVal ? S_OK : E_OUTOFMEMORY; };
                if (id == UIA_ControlTypePropertyId) { value->vt = VT_I4; value->lVal = UIA_WindowControlTypeId; }
                else if (id == UIA_NamePropertyId) return text(m_state->name);
                else if (id == UIA_HelpTextPropertyId || id == UIA_FullDescriptionPropertyId) return text(m_state->description);
                else if (id == UIA_LocalizedControlTypePropertyId) return text(std::wstring(NativePreferences::InlinePair(L"alert dialog", L"提示對話框").GetString()));
                else if (id == UIA_AutomationIdPropertyId) return text(L"NativeMaterial.AlertDialog");
                else if (id == UIA_IsDialogPropertyId || id == UIA_IsEnabledPropertyId || id == UIA_IsControlElementPropertyId || id == UIA_IsContentElementPropertyId)
                { value->vt = VT_BOOL; value->boolVal = VARIANT_TRUE; }
                return S_OK;
            }
            HRESULT STDMETHODCALLTYPE get_HostRawElementProvider(IRawElementProviderSimple** value) override
            {
                if (!value) return E_POINTER;
                *value = nullptr;
                std::lock_guard<std::mutex> lock(m_state->mutex);
                return m_state->window ? UiaHostProviderFromHwnd(m_state->window, value) : UIA_E_ELEMENTNOTAVAILABLE;
            }
        private:
            std::atomic<ULONG> m_references{ 1 };
            std::shared_ptr<AlertAccess> m_state;
        };

        HWND ResolveOwner(HWND owner)
        {
            if (!owner || !IsWindow(owner)) return nullptr;
            HWND root = GetAncestor(owner, GA_ROOT);
            if (!root) return nullptr;
            HWND popup = GetLastActivePopup(root);
            return popup && IsWindow(popup) && IsWindowVisible(popup) ? popup : root;
        }

        class AlertWindow final : public CWnd
        {
        public:
            int Run(const AlertSpec& spec);
            void Fill(AlertSnapshot& out) const;
        protected:
            LRESULT WindowProc(UINT message, WPARAM wparam, LPARAM lparam) override;
        private:
            void Layout(bool keepCenter);
            void BuildButtons();
            void Paint();
            void DrawAll(Gfx& g, const Palette& p);
            void Finish(int result) { m_result = result; m_done = true; }
            void MoveFocus(int step);
            int FocusedIndex() const;
            void FocusAction(int index);
            void Scroll(int delta);
            int MaxScroll() const { return (std::max)(0, m_layout.bodyContentHeight - Height(m_layout.bodyView)); }
            bool Pretranslate(const MSG& msg);
            void UseGdi();

            AlertSpec m_spec;
            AlertLayout m_layout;
            std::unique_ptr<Typography> m_type;
            std::vector<std::unique_ptr<Button>> m_buttons;
            std::vector<ButtonVariant> m_variants;
            std::shared_ptr<AlertAccess> m_access;
            Canvas m_canvas;
            Appearance m_appearance{ Appearance::Light };
            RECT m_work{};
            UINT m_dpi{ 96 };
            int m_focus{}, m_result{}, m_scroll{};
            bool m_done{}, m_forceGdi{};
        };

        void AlertWindow::UseGdi()
        {
            m_forceGdi = true;
            m_canvas.Detach();
            m_type = std::make_unique<Typography>(m_dpi, true);
            Layout(true);
            Invalidate(FALSE);
        }

        void AlertWindow::Layout(bool keepCenter)
        {
            const SIZE work{ Width(m_work), Height(m_work) };
            m_layout = ComputeLayout(m_spec, m_dpi, work, *m_type);
            m_scroll = (std::min)(m_scroll, MaxScroll());
            if (!m_hWnd) return;
            RECT rect{};
            GetWindowRect(&rect);
            const int cx = (rect.left + rect.right) / 2, cy = (rect.top + rect.bottom) / 2;
            int x = keepCenter ? cx - m_layout.size.cx / 2 : rect.left, y = keepCenter ? cy - m_layout.size.cy / 2 : rect.top;
            x = Place(x, static_cast<int>(m_work.left), static_cast<int>(m_work.right - m_layout.size.cx));
            y = Place(y, static_cast<int>(m_work.top), static_cast<int>(m_work.bottom - m_layout.size.cy));
            SetWindowPos(nullptr, x, y, m_layout.size.cx, m_layout.size.cy, SWP_NOZORDER | SWP_NOACTIVATE);
            const int radius = Px(28, m_dpi);
            SetWindowRgn(CreateRoundRectRgn(0, 0, m_layout.size.cx + 1, m_layout.size.cy + 1, radius * 2, radius * 2), TRUE);
            if (!m_type->Gdi()) m_canvas.Resize(static_cast<UINT>(m_layout.size.cx), static_cast<UINT>(m_layout.size.cy), 96.f);
            for (size_t i = 0; i < m_buttons.size() && i < m_layout.actions.size(); ++i)
            {
                const RECT& r = m_layout.actions[i];
                m_buttons[i]->MoveWindow(r.left, r.top, Width(r), Height(r), TRUE);
            }
        }

        void AlertWindow::BuildButtons()
        {
            const auto palette = Palette::Resolve(m_appearance);
            const COLORREF surface = Rgb(SurfaceColor(palette));
            for (size_t i = 0; i < m_spec.actions.size(); ++i)
            {
                auto button = std::make_unique<Button>();
                const RECT& r = m_layout.actions[i];
                button->Create(m_spec.actions[i].label, r, this, FirstActionId + static_cast<UINT>(i));
                button->SetAppearance(m_appearance);
                button->SetVariant(m_variants[i]);
                button->SetSurfaceColor(surface);
                m_buttons.push_back(std::move(button));
            }
        }

        void AlertWindow::DrawAll(Gfx& g, const Palette& p)
        {
            DrawIcon(g, p, m_spec.icon, m_layout.icon, m_dpi);
            g.Text(m_spec.headline, m_layout.headline, Style::Headline, p.onSurface, m_spec.icon != AlertIcon::None);
            if (m_spec.body.IsEmpty()) return;
            RECT content = m_layout.bodyView;
            content.top -= m_scroll;
            content.bottom = content.top + (std::max)(m_layout.bodyContentHeight, Height(m_layout.bodyView));
            g.Clip(m_layout.bodyView);
            g.Text(m_spec.body, content, Style::Body, p.onSurfaceVariant, false);
            g.Unclip();
            if (m_layout.scrolls)
            {
                const int track = Height(m_layout.bodyView), total = m_layout.bodyContentHeight;
                const int thumb = (std::max)(Px(24, m_dpi), track * track / (std::max)(1, total));
                const int top = m_layout.bodyView.top + (MaxScroll() ? (track - thumb) * m_scroll / MaxScroll() : 0);
                const int x = m_layout.bodyView.right + Px(10, m_dpi);
                g.RoundRect({ x, top, x + Px(4, m_dpi), top + thumb }, Px(2, m_dpi) * 1.f, p.outline);
            }
        }

        void AlertWindow::Paint()
        {
            PAINTSTRUCT ps{};
            ::BeginPaint(m_hWnd, &ps);
            const auto palette = Palette::Resolve(m_appearance);
            const auto background = SurfaceColor(palette);
            if (!m_type->Gdi())
            {
                if (SUCCEEDED(m_canvas.Begin(background)))
                {
                    D2dGfx g(m_canvas, *m_type);
                    DrawAll(g, palette);
                    if (FAILED(m_canvas.End())) Invalidate(FALSE);
                    ::EndPaint(m_hWnd, &ps);
                    return;
                }
                UseGdi();
            }
            RECT client{};
            GetClientRect(&client);
            HBRUSH brush = CreateSolidBrush(Rgb(background));
            FillRect(ps.hdc, &client, brush);
            DeleteObject(brush);
            GdiGfx g(ps.hdc, *m_type);
            DrawAll(g, palette);
            ::EndPaint(m_hWnd, &ps);
        }

        void AlertWindow::FocusAction(int index)
        {
            if (index < 0 || index >= static_cast<int>(m_buttons.size())) return;
            m_focus = index;
            m_buttons[index]->SetFocus();
        }
        int AlertWindow::FocusedIndex() const
        {
            const HWND focus = ::GetFocus();
            for (size_t i = 0; i < m_buttons.size(); ++i)
                if (focus && focus == m_buttons[i]->GetSafeHwnd()) return static_cast<int>(i);
            return m_focus;
        }
        void AlertWindow::MoveFocus(int step)
        {
            const int count = static_cast<int>(m_buttons.size());
            if (count) FocusAction(((FocusedIndex() + step) % count + count) % count);
        }
        void AlertWindow::Scroll(int delta)
        {
            const int next = (std::max)(0, (std::min)(MaxScroll(), m_scroll + delta));
            if (next == m_scroll) return;
            m_scroll = next;
            InvalidateRect(nullptr, FALSE);
        }

        bool AlertWindow::Pretranslate(const MSG& msg)
        {
            if ((msg.message != WM_KEYDOWN && msg.message != WM_SYSKEYDOWN) || (msg.hwnd != m_hWnd && !::IsChild(m_hWnd, msg.hwnd))) return false;
            const bool repeat = (msg.lParam & 0x40000000) != 0;
            switch (msg.wParam)
            {
            case VK_ESCAPE:
                // A held key that opened the dialog must not also close it.
                if (!repeat) Finish(m_spec.dismissResult);
                return true;
            case VK_RETURN:
                if (!repeat) Finish(m_spec.actions[FocusedIndex()].result);
                return true;
            case VK_TAB:
                MoveFocus((GetKeyState(VK_SHIFT) & 0x8000) ? -1 : 1);
                return true;
            case VK_LEFT: MoveFocus(-1); return true;
            case VK_RIGHT: MoveFocus(1); return true;
            case VK_UP: if (m_layout.scrolls) { Scroll(-Px(20, m_dpi)); return true; } return false;
            case VK_DOWN: if (m_layout.scrolls) { Scroll(Px(20, m_dpi)); return true; } return false;
            case VK_PRIOR: if (m_layout.scrolls) { Scroll(-Height(m_layout.bodyView)); return true; } return false;
            case VK_NEXT: if (m_layout.scrolls) { Scroll(Height(m_layout.bodyView)); return true; } return false;
            case VK_HOME: if (m_layout.scrolls) { Scroll(-m_scroll); return true; } return false;
            case VK_END: if (m_layout.scrolls) { Scroll(MaxScroll()); return true; } return false;
            }
            return false;
        }

        LRESULT AlertWindow::WindowProc(UINT message, WPARAM wparam, LPARAM lparam)
        {
            switch (message)
            {
            case WM_PAINT: Paint(); return 0;
            case WM_ERASEBKGND: return 1;
            case WM_PRINTCLIENT:
            {
                if (!wparam) return 0;
                const Typography fallback(m_dpi, true);
                GdiGfx g(reinterpret_cast<HDC>(wparam), fallback);
                RECT client{};
                GetClientRect(&client);
                const auto palette = Palette::Resolve(m_appearance);
                HBRUSH brush = CreateSolidBrush(Rgb(SurfaceColor(palette)));
                FillRect(reinterpret_cast<HDC>(wparam), &client, brush);
                DeleteObject(brush);
                DrawAll(g, palette);
                return 0;
            }
            case WM_COMMAND:
                if (HIWORD(wparam) == BN_CLICKED && LOWORD(wparam) >= FirstActionId
                    && LOWORD(wparam) < FirstActionId + m_spec.actions.size())
                {
                    Finish(m_spec.actions[LOWORD(wparam) - FirstActionId].result);
                    return 0;
                }
                break;
            case WM_CLOSE: Finish(m_spec.dismissResult); return 0;
            case WM_MOUSEWHEEL:
                if (m_layout.scrolls) { Scroll(-GET_WHEEL_DELTA_WPARAM(wparam) * Px(40, m_dpi) / WHEEL_DELTA); return 0; }
                break;
            case WM_MOUSEACTIVATE: return MA_ACTIVATE;
            case WM_ACTIVATE:
                if (LOWORD(wparam) != WA_INACTIVE && !m_buttons.empty()) FocusAction(m_focus);
                break;
            case WM_SETFOCUS:
                if (!m_buttons.empty()) { FocusAction(m_focus); return 0; }
                break;
            case WM_DPICHANGED:
            {
                m_dpi = HIWORD(wparam);
                m_type = std::make_unique<Typography>(m_dpi, m_forceGdi);
                Layout(true);
                Invalidate(FALSE);
                return 0;
            }
            case WM_GETOBJECT:
                if (static_cast<LONG>(lparam) == static_cast<LONG>(UiaRootObjectId) && m_access)
                {
                    auto* provider = new AlertProvider(m_access);
                    const LRESULT result = UiaReturnRawElementProvider(m_hWnd, wparam, lparam, provider);
                    provider->Release();
                    return result;
                }
                break;
            case WM_DESTROY:
                RemovePropW(m_hWnd, InspectProperty);
                if (m_access) { std::lock_guard<std::mutex> lock(m_access->mutex); m_access->window = nullptr; }
                m_canvas.Detach();
                break;
            }
            return CWnd::WindowProc(message, wparam, lparam);
        }

        void AlertWindow::Fill(AlertSnapshot& out) const
        {
            out = AlertSnapshot{};
            for (size_t i = 0; i < m_spec.actions.size(); ++i)
            {
                out.results.push_back(m_spec.actions[i].result);
                out.labels.push_back(m_spec.actions[i].label);
                out.variants.push_back(static_cast<int>(m_variants[i]));
                out.buttons.push_back(i < m_buttons.size() ? m_buttons[i]->GetSafeHwnd() : nullptr);
                if (i < m_buttons.size() && ::GetFocus() == m_buttons[i]->GetSafeHwnd()) out.systemFocusIndex = static_cast<int>(i);
            }
            out.focusedIndex = m_focus;
            out.defaultIndex = m_spec.defaultIndex;
            out.dismissResult = m_spec.dismissResult;
            out.headline = m_spec.headline;
            out.body = m_spec.body;
            out.icon = m_spec.icon;
            out.layout = m_layout;
            out.gdi = m_type->Gdi();
            out.dpi = m_dpi;
            out.scroll = m_scroll;
        }

        int AlertWindow::Run(const AlertSpec& spec)
        {
            m_spec = spec;
            if (m_spec.actions.empty()) m_spec.actions.push_back({ IDOK, StandardLabel(IDOK), false });
            if (m_spec.actions.size() > 3) m_spec.actions.resize(3);
            if (m_spec.headline.IsEmpty()) m_spec.headline = DefaultHeadline(m_spec.icon);
            if (m_spec.defaultIndex < 0 || m_spec.defaultIndex >= static_cast<int>(m_spec.actions.size())) m_spec.defaultIndex = 0;
            if (m_spec.dismissResult == 0) m_spec.dismissResult = m_spec.actions.back().result;
            m_focus = m_spec.defaultIndex;
            m_result = m_spec.dismissResult;
            m_appearance = spec.forceAppearance ? spec.appearance : NativePreferences::EffectiveAppearance();
            for (size_t i = 0; i < m_spec.actions.size(); ++i) m_variants.push_back(VariantFor(m_spec, i));

            const HWND prevFocus = ::GetFocus();
            const HWND owner = ResolveOwner(m_spec.owner);
            const bool ownerShown = owner && ::IsWindowVisible(owner) && !::IsIconic(owner);
            m_dpi = owner ? GetDpiForWindow(owner) : GetDpiForSystem();
            if (!m_dpi) m_dpi = 96;
            MONITORINFO monitor{ sizeof(monitor) };
            const HMONITOR handle = ownerShown ? MonitorFromWindow(owner, MONITOR_DEFAULTTONEAREST) : MonitorFromPoint({ 0, 0 }, MONITOR_DEFAULTTOPRIMARY);
            if (!GetMonitorInfoW(handle, &monitor)) { monitor.rcWork = { 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN) }; }
            m_work = monitor.rcWork;
            m_forceGdi = g_hooks && g_hooks->forceGdi;
            m_type = std::make_unique<Typography>(m_dpi, m_forceGdi);
            m_layout = ComputeLayout(m_spec, m_dpi, { Width(m_work), Height(m_work) }, *m_type);

            RECT anchor = m_work;
            if (ownerShown) ::GetWindowRect(owner, &anchor);
            const int x = Place(static_cast<int>((anchor.left + anchor.right - m_layout.size.cx) / 2), static_cast<int>(m_work.left), static_cast<int>(m_work.right - m_layout.size.cx));
            const int y = Place(static_cast<int>((anchor.top + anchor.bottom - m_layout.size.cy) / 2), static_cast<int>(m_work.top), static_cast<int>(m_work.bottom - m_layout.size.cy));

            const auto className = AfxRegisterWndClass(CS_DROPSHADOW | CS_HREDRAW | CS_VREDRAW, LoadCursor(nullptr, IDC_ARROW), nullptr);
            DWORD exStyle = WS_EX_CONTROLPARENT | (owner ? WS_EX_TOOLWINDOW : WS_EX_APPWINDOW);
            if (!CWnd::CreateEx(exStyle, className, m_spec.headline, WS_POPUP | WS_CLIPCHILDREN, x, y, m_layout.size.cx, m_layout.size.cy, owner, nullptr))
                return m_spec.dismissResult;
            SetPropW(m_hWnd, InspectProperty, this);
            m_access = std::make_shared<AlertAccess>();
            m_access->window = m_hWnd;
            m_access->name = m_spec.headline.GetString();
            m_access->description = m_spec.body.GetString();
            if (!m_type->Gdi() && FAILED(m_canvas.Attach(m_hWnd, 96.f))) UseGdi();
            Layout(false);
            BuildButtons();
            Layout(false);

            bool disabledOwner = false;
            if (owner && ::IsWindowEnabled(owner)) { ::EnableWindow(owner, FALSE); disabledOwner = true; }
            ShowWindow(SW_SHOW);
            SetForegroundWindow();
            SetActiveWindow();
            FocusAction(m_focus);
            UpdateWindow();
            ::NotifyWinEvent(EVENT_SYSTEM_ALERT, m_hWnd, OBJID_CLIENT, CHILDID_SELF);
            ::NotifyWinEvent(EVENT_SYSTEM_DIALOGSTART, m_hWnd, OBJID_CLIENT, CHILDID_SELF);
            if (g_hooks && g_hooks->shown) g_hooks->shown(m_hWnd, g_hooks->context);

            MSG msg{};
            while (!m_done)
            {
                const BOOL got = GetMessage(&msg, nullptr, 0, 0);
                if (got == 0) { PostQuitMessage(static_cast<int>(msg.wParam)); Finish(m_spec.dismissResult); break; }
                if (got < 0) break;
                if (Pretranslate(msg)) continue;
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }

            if (disabledOwner) ::EnableWindow(owner, TRUE);
            DestroyWindow();
            m_buttons.clear();
            if (prevFocus && IsWindow(prevFocus) && GetWindowThreadProcessId(prevFocus, nullptr) == GetCurrentThreadId()) ::SetFocus(prevFocus);
            return m_result;
        }
    }

    int ShowAlert(const AlertSpec& spec)
    {
        AlertWindow window;
        return window.Run(spec);
    }

    AlertSpec SpecFromMessageBox(HWND owner, const CString& text, const CString& caption, UINT type)
    {
        AlertSpec spec;
        spec.owner = owner;
        spec.body = text;
        spec.headline = caption;
        switch (type & MB_ICONMASK)
        {
        case MB_ICONERROR: spec.icon = AlertIcon::Error; break;
        case MB_ICONWARNING: spec.icon = AlertIcon::Warning; break;
        case MB_ICONQUESTION: spec.icon = AlertIcon::Question; break;
        case MB_ICONINFORMATION: spec.icon = AlertIcon::Information; break;
        default: break;
        }
        std::vector<int> ids;
        int dismiss = IDOK;
        switch (type & MB_TYPEMASK)
        {
        case MB_OKCANCEL: ids = { IDOK, IDCANCEL }; dismiss = IDCANCEL; break;
        case MB_ABORTRETRYIGNORE: ids = { IDABORT, IDRETRY, IDIGNORE }; dismiss = IDIGNORE; break;
        case MB_YESNOCANCEL: ids = { IDYES, IDNO, IDCANCEL }; dismiss = IDCANCEL; break;
        case MB_YESNO: ids = { IDYES, IDNO }; dismiss = IDNO; break;
        case MB_RETRYCANCEL: ids = { IDRETRY, IDCANCEL }; dismiss = IDCANCEL; break;
        case MB_CANCELTRYCONTINUE: ids = { IDCANCEL, IDTRYAGAIN, IDCONTINUE }; dismiss = IDCANCEL; break;
        default: ids = { IDOK }; dismiss = IDOK; break;
        }
        for (const int id : ids) spec.actions.push_back({ id, StandardLabel(id), false });
        spec.dismissResult = dismiss;
        switch (type & MB_DEFMASK)
        {
        case MB_DEFBUTTON2: spec.defaultIndex = 1; break;
        case MB_DEFBUTTON3: spec.defaultIndex = 2; break;
        default: spec.defaultIndex = 0; break;
        }
        if (spec.defaultIndex >= static_cast<int>(spec.actions.size())) spec.defaultIndex = 0;
        return spec;
    }

    int Alert(HWND owner, const CString& text, const CString& caption, UINT type)
    {
        return ShowAlert(SpecFromMessageBox(owner, text, caption, type));
    }

    int Alert(const CString& text, UINT type)
    {
        CWinApp* app = AfxGetApp();
        CWnd* main = app ? app->m_pMainWnd : nullptr;
        return Alert(main ? main->GetSafeHwnd() : nullptr, text, CString(), type);
    }

    AlertLayout MeasureAlert(const AlertSpec& input, UINT dpi, SIZE workArea, bool forceGdi)
    {
        AlertSpec spec = input;
        if (spec.actions.empty()) spec.actions.push_back({ IDOK, StandardLabel(IDOK), false });
        if (spec.headline.IsEmpty()) spec.headline = DefaultHeadline(spec.icon);
        return ComputeLayout(spec, dpi ? dpi : 96, workArea, Typography(dpi ? dpi : 96, forceGdi));
    }

    bool InspectAlert(HWND dialog, AlertSnapshot& out)
    {
        auto* window = static_cast<AlertWindow*>(GetPropW(dialog, InspectProperty));
        if (!window) return false;
        window->Fill(out);
        return true;
    }

    void SetAlertTestHooks(const AlertTestHooks* hooks) { g_hooks = hooks; }
}
