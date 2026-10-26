#pragma once
#include "InputVisuals.h"

// Direct2D drawing and geometry for the Material 3 feedback components: snackbar, progress indicators and
// chips. Coordinates are 96-dpi device-independent pixels; render targets carry the window DPI, so callers
// never scale. The same functions paint the real windows and the offscreen verification renders.
namespace NativeMaterial
{
    namespace FeedbackMetrics
    {
        // Snackbar (M3: 344 to 672 wide, 48 high for one line, 68 for two).
        constexpr float SnackbarMinWidth = 344.f, SnackbarMaxWidth = 672.f, SnackbarMargin = 16.f;
        constexpr float SnackbarOneLine = 48.f, SnackbarTwoLines = 68.f, SnackbarPadding = 16.f, SnackbarCorner = 4.f;
        constexpr float SnackbarTextSize = 14.f, SnackbarIconTarget = 48.f, SnackbarActionHeight = 40.f, SnackbarActionPadding = 12.f;
        // Chip (M3: 32 high container; the window is 40 high so the state layer and focus ring fit).
        constexpr float ChipHeight = 32.f, ChipTarget = 40.f, ChipCorner = 8.f, ChipPadding = 16.f, ChipIconPadding = 8.f;
        constexpr float ChipIcon = 18.f, ChipGap = 8.f, ChipTextSize = 14.f, ChipInset = 2.f;
        // Progress (M3: 4 high linear track, 48 wide circular indicator with a 4 stroke).
        constexpr float LinearTrack = 4.f, LinearBare = 8.f, LinearWithText = 32.f, CircularDefault = 48.f, CircularMinimum = 24.f;
        constexpr float CircularStroke = 4.f, ProgressTextSize = 14.f, ProgressTextGap = 12.f, ProgressTextRow = 20.f;
    }

    // ------------------------------------------------------------------ snackbar
    enum class SnackbarPart { None, Action, Close };
    struct SnackbarLayout { D2D1_RECT_F text, action, close; float height{}; bool twoLines{}; bool hasAction{}, hasClose{}; };
    // Width of the action button for its label (text plus padding, at least 48).
    float SnackbarActionWidth(const CString& label);
    SnackbarLayout LayoutSnackbar(float width, bool twoLines, float actionWidth, bool hasClose);
    // True when the text does not fit on one line next to the action and close controls.
    bool SnackbarNeedsTwoLines(const CString& text, float width, float actionWidth, bool hasClose);
    // The width a snackbar takes inside a host client area of the given DIP width.
    float SnackbarWidthFor(float hostWidth);
    struct SnackbarModel { CString text, action; bool closable{}; bool twoLines{}; SnackbarPart focusPart{ SnackbarPart::None }, hoverPart{ SnackbarPart::None }, pressedPart{ SnackbarPart::None }; };
    void DrawSnackbar(ID2D1RenderTarget*, const Palette&, const SnackbarModel&, const InputState&);

    // ------------------------------------------------------------------ progress
    enum class ProgressShape { Linear, Circular };
    struct ProgressLayout { D2D1_RECT_F indicator, text; bool hasText{}; };
    ProgressLayout LayoutProgress(float width, float height, ProgressShape shape, bool wantsText);
    struct ProgressModel
    {
        ProgressShape shape{ ProgressShape::Linear };
        bool indeterminate{};
        float fraction{};     // 0..1, determinate only
        float phase{};        // 0..1 position in the animation cycle; ignored when reduced
        bool reducedMotion{}; // static indeterminate presentation
        CString status;
    };
    void DrawProgress(ID2D1RenderTarget*, const Palette&, const ProgressModel&, const InputState&);

    // --------------------------------------------------------------------- chips
    enum class ChipKind { Assist, Filter, Input, Suggestion };
    enum class ChipPart { None, Body, Close };
    struct ChipLayout { D2D1_RECT_F body, icon, text, close; bool hasIcon{}, hasClose{}; };
    // Whether the chip shows a leading check (selected filter chip) or a trailing close icon (input chip).
    inline bool ChipHasCheck(ChipKind kind, bool selected) { return kind == ChipKind::Filter && selected; }
    inline bool ChipHasClose(ChipKind kind) { return kind == ChipKind::Input; }
    float ChipWidth(const CString& label, ChipKind kind, bool selected);
    ChipLayout LayoutChip(float width, float height, ChipKind kind, bool selected);
    struct ChipModel { ChipKind kind{ ChipKind::Assist }; CString label; bool selected{}; ChipPart hoverPart{ ChipPart::None }, pressedPart{ ChipPart::None }; };
    void DrawChip(ID2D1RenderTarget*, const Palette&, const ChipModel&, const InputState&);
}
