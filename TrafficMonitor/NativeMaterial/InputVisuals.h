#pragma once
#include "Theme.h"
#include <afxstr.h>
#include <vector>

// Direct2D drawing and geometry for the Material 3 input components. Every
// coordinate here is in 96-dpi device-independent pixels; render targets carry
// the window DPI, so callers never scale. The same functions paint the real
// windows and the offscreen verification renders.
namespace NativeMaterial
{
    namespace InputMetrics
    {
        constexpr float MinimumTarget = 48.f;   // smallest interactive height (design contract: 40 to 48)
        constexpr float SwitchTrackWidth = 52.f, SwitchTrackHeight = 32.f;
        constexpr float ToggleGap = 12.f;       // gap between a control glyph and its label
        constexpr float StateLayer = 40.f;      // hover, focus and press layer diameter
        constexpr float CheckboxSize = 18.f, RadioSize = 20.f;
        constexpr float SegmentHeight = 40.f, SegmentMinimumWidth = 64.f;
        constexpr float FieldHeight = 56.f, FieldTop = 8.f, MessageHeight = 20.f;
        constexpr float StepperButton = 40.f;
        constexpr float RowHeight = 48.f;
        constexpr int PopupMaximumRows = 6;
    }

    struct InputState
    {
        bool enabled{ true }, hover{}, pressed{}, focused{}, error{};
    };

    enum class CheckState { Unchecked, Checked, Indeterminate };

    void DrawSwitch(ID2D1RenderTarget*, const Palette&, const CString& label, bool on, bool showIcon, const InputState&);
    void DrawCheckbox(ID2D1RenderTarget*, const Palette&, const CString& label, CheckState, const InputState&);
    void DrawRadio(ID2D1RenderTarget*, const Palette&, const CString& label, bool checked, const InputState&);

    // Rows of radio options drawn inside one window (one tab stop).
    void DrawRadioGroup(ID2D1RenderTarget*, const Palette&, const std::vector<CString>& labels, int selected, int focusRow, int hoverRow, const InputState&);
    D2D1_RECT_F RadioRowRect(float width, int row);

    std::vector<D2D1_RECT_F> SegmentRects(float width, float height, int count);
    void DrawSegments(ID2D1RenderTarget*, const Palette&, const std::vector<CString>& labels, int selected, int hoverSegment, const InputState&);

    struct StepperLayout { D2D1_RECT_F field, minus, plus, edit, message; };
    StepperLayout LayoutStepper(float width, float height);
    enum class StepperPart { None, Minus, Plus };
    void DrawStepper(ID2D1RenderTarget*, const Palette&, const CString& label, const CString& message, bool minusEnabled, bool plusEnabled,
        StepperPart hover, StepperPart pressed, const InputState&);

    struct DropdownLayout { D2D1_RECT_F field, text, arrow, message; };
    DropdownLayout LayoutDropdown(float width, float height);
    void DrawDropdownField(ID2D1RenderTarget*, const Palette&, const CString& label, const CString& text, const CString& message,
        bool placeholder, bool expanded, const InputState&);

    struct PopupLayout { D2D1_RECT_F frame, search, regex, edit, error, list; int rows{}; float height{}; };
    // rowCount is the number of items in the full list; error reserves a line below the search field.
    PopupLayout LayoutPopup(float width, int rowCount, bool hasError);
    struct PopupModel
    {
        std::vector<CString> rows;
        int selectedRow{ -1 }, hotRow{ -1 }, firstRow{};
        bool regex{}, regexHot{}, queryEmpty{ true };
        CString error, emptyText, regexLabel;
    };
    void DrawDropdownPopup(ID2D1RenderTarget*, const Palette&, const PopupModel&);

    struct LabeledFieldLayout { D2D1_RECT_F label, edit, clear, message; };
    LabeledFieldLayout LayoutLabeledField(float width, float height);
    // underlineIndex marks the label character drawn underlined as a keyboard mnemonic (-1 for none).
void DrawLabeledFieldChrome(ID2D1RenderTarget*, const Palette&, const CString& label, const CString& message, const InputState&, int underlineIndex = -1);
    void DrawClearButton(ID2D1RenderTarget*, const Palette&, bool fieldFocused, const InputState&);
}
