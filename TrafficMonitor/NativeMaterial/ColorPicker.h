#pragma once
#include "PickerDialog.h"
#include "InputControls.h"
#include <vector>

// Material 3 color picker: a modal dialog (no common color dialog, no stock control) with a saturation / value
// area, a hue slider, a hex field, red / green / blue steppers, preset and recent swatches, a before / after pair
// and a contrast preview against the light and dark surfaces.
//
//   COLORREF color = RGB(52, 94, 156);
//   if (NativeMaterial::PickColor(hwnd, color, L"Text color")) { /* color holds the choice */ }
//
// The pure color math below has no window dependency, so it can be verified without any UI.
namespace NativeMaterial
{
    // Hue in degrees [0, 360), saturation and value in [0, 1].
    struct Hsv { double h{}, s{}, v{}; };
    Hsv RgbToHsv(COLORREF color);
    COLORREF HsvToRgb(const Hsv& hsv);
    // Accepts "#RRGGBB", "RRGGBB", "#RGB" and "RGB" with surrounding blanks; rejects everything else.
    bool ParseHexColor(const CString& text, COLORREF& color);
    CString FormatHexColor(COLORREF color);            // "#RRGGBB", upper case
    double RelativeLuminance(COLORREF color);          // WCAG 2.x
    double ContrastRatio(COLORREF a, COLORREF b);      // 1 to 21
    // "AA" (4.5 or more), "AA large" (3 or more) or "Low" for a color drawn on a background.
    const wchar_t* ContrastGrade(double ratio);

    const std::vector<COLORREF>& PresetPalette();
    constexpr size_t RecentColorLimit = 8;
    // Most recent first, unique, at most RecentColorLimit entries. Process lifetime only; the owner decides
    // whether and where to persist them with SetRecentColors / RecentColors.
    std::vector<COLORREF> RecentColors();
    void SetRecentColors(const std::vector<COLORREF>& colors);
    void RememberColor(COLORREF color);

    // Direct2D drawing of the picker parts (96-dpi units; the same function paints the window and the verification).
    void DrawColorArea(ID2D1RenderTarget*, const Palette&, const Hsv&, const InputState&);
    void DrawHueSlider(ID2D1RenderTarget*, const Palette&, double hue, const InputState&);
    void DrawSwatches(ID2D1RenderTarget*, const Palette&, const std::vector<COLORREF>&, int columns, int selected, int focus, int hover, const InputState&);
    void DrawContrastPreview(ID2D1RenderTarget*, const Palette&, COLORREF color, D2D1_RECT_F bounds);
    constexpr float SwatchSize = 26.f, SwatchGap = 6.f;

    // Notifies the parent with EN_CHANGE after a user change. Arrow keys move by 1 percent (Shift: 10 percent),
    // Home / End jump to the left / right edge, PageUp / PageDown to the top / bottom edge.
    class ColorArea final : public InputWindow
    {
    public:
        BOOL Create(const CString& name, const RECT& bounds, CWnd* parent, UINT id);
        void SetHsv(const Hsv& hsv);                 // programmatic; does not notify
        Hsv GetHsv() const { return m_hsv; }
        // Moves as a user change would (clamped); returns whether it changed and notifies.
        bool MoveTo(double saturation, double value);
        void SetAccessibleName(const CString& name);
        static void PointToSv(float x, float y, float width, float height, double& saturation, double& value);
    protected:
        void Paint(ID2D1RenderTarget*, const Palette&, const InputState&) override;
        CSize MinimumLogicalSize() const override { return CSize(160, 120); }
        void FillAccess(InputData&) const override;
        void PointerPressed(CPoint) override;
        void PointerReleased(CPoint, bool) override;
        void PointerMoved(CPoint) override;
        bool KeyPressed(UINT, bool) override;
        UINT DialogCode() const override { return DLGC_WANTARROWS; }
    private:
        void FromPoint(CPoint);
        Hsv m_hsv{};
        bool m_dragging{};
    };

    // Hue 0..359 as a rainbow track. Same keys and notifications as the Slider (EN_CHANGE).
    class HueSlider final : public InputWindow
    {
    public:
        BOOL Create(const CString& name, const RECT& bounds, CWnd* parent, UINT id);
        void SetHue(int hue);                        // programmatic; wraps into 0..359; does not notify
        int GetHue() const { return m_hue; }
        bool MoveTo(int hue);                        // user change; clamps into 0..359; notifies
    protected:
        void Paint(ID2D1RenderTarget*, const Palette&, const InputState&) override;
        CSize MinimumLogicalSize() const override { return CSize(160, 40); }
        void FillAccess(InputData&) const override;
        void PointerPressed(CPoint) override;
        void PointerReleased(CPoint, bool) override;
        void PointerMoved(CPoint) override;
        bool KeyPressed(UINT, bool) override;
        UINT DialogCode() const override { return DLGC_WANTARROWS; }
        void PerformAction(InputActionId, int) override;
    private:
        int HueAt(CPoint) const;
        int m_hue{};
        bool m_dragging{};
    };

    // A grid of color swatches that behaves as one radio group. Notifies CBN_SELCHANGE when the user chooses one.
    class SwatchGrid final : public InputWindow
    {
    public:
        BOOL Create(const CString& name, int columns, const RECT& bounds, CWnd* parent, UINT id);
        void SetColors(const std::vector<COLORREF>& colors);
        const std::vector<COLORREF>& Colors() const { return m_colors; }
        void SetSelected(int index);                 // programmatic; does not notify
        int GetSelected() const { return m_selected; }
        int GetFocusIndex() const { return m_focus; }
        int Columns() const { return m_columns; }
        int Rows() const;
        // Smallest size in DIPs that shows every swatch.
        CSize NaturalSize() const;
        int IndexAt(CPoint) const;
    protected:
        void Paint(ID2D1RenderTarget*, const Palette&, const InputState&) override;
        CSize MinimumLogicalSize() const override { return NaturalSize(); }
        void FillAccess(InputData&) const override;
        void PointerPressed(CPoint) override;
        void PointerReleased(CPoint, bool) override;
        void PointerMoved(CPoint) override;
        void PointerLeft() override;
        bool KeyPressed(UINT, bool) override;
        UINT DialogCode() const override { return DLGC_WANTARROWS; }
        void PerformAction(InputActionId, int) override;
    private:
        void Choose(int index);
        std::vector<COLORREF> m_colors;
        int m_columns{ 1 }, m_selected{ -1 }, m_focus{ 0 }, m_hot{ -1 };
    };

    class ColorPickerDialog final : public PickerDialog
    {
    public:
        ColorPickerDialog();
        ~ColorPickerDialog() override;
        // Runs the dialog. Returns true and the chosen color when the user accepts.
        bool Run(HWND owner, COLORREF& color, const CString& title);
        // Programmatic change of the working color (updates every part, does not notify).
        void SetColor(COLORREF color);
        COLORREF Color() const { return m_color; }
        COLORREF OriginalColor() const { return m_original; }
        Hsv CurrentHsv() const { return m_hsv; }
        bool HexValid() const { return m_hexValid; }
        ColorArea& Area() { return m_area; }
        HueSlider& Hue() { return m_hue; }
        LabeledTextField& HexField() { return m_hex; }
        NumberStepper& Red() { return m_red; }
        NumberStepper& Green() { return m_green; }
        NumberStepper& Blue() { return m_blue; }
        SwatchGrid& Presets() { return m_presets; }
        SwatchGrid& Recents() { return m_recents; }
        static ColorPickerDialog* FromHandle(HWND dialog);
        enum ControlId : UINT { IdArea = 100, IdHue, IdHex, IdRed, IdGreen, IdBlue, IdPresets, IdRecents };
    protected:
        void CreateContent() override;
        void LayoutContent() override;
        void PaintContent(ID2D1RenderTarget*, const Palette&) override;
        std::vector<HWND> ContentFocusOrder() const override;
        bool CanAccept() const override { return m_hexValid; }
        bool CommitPending() override;
        HWND InitialFocus() const override { return m_area.GetSafeHwnd(); }
        bool ContentCommand(UINT id, UINT code) override;
    private:
        enum class Source { Program, Area, Hue, Hex, Rgb, Swatch };
        void ApplyHsv(const Hsv& hsv, Source source);
        void ApplyRgb(COLORREF color, Source source);
        void Sync(Source source);
        ColorArea m_area;
        HueSlider m_hue;
        LabeledTextField m_hex;
        NumberStepper m_red, m_green, m_blue;
        SwatchGrid m_presets, m_recents;
        Hsv m_hsv{};
        COLORREF m_color{}, m_original{};
        bool m_hexValid{ true }, m_updating{};
    };

    bool PickColor(HWND owner, COLORREF& inOut, const CString& title);
}
