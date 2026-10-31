#include "../stdafx.h"
#include "FeedbackVisuals.h"
#include <algorithm>
#include <cmath>

namespace NativeMaterial
{
    using namespace FeedbackMetrics;
    namespace
    {
        using Microsoft::WRL::ComPtr;

        // One brush for a single paint.
        struct Pen
        {
            ID2D1RenderTarget* target;
            ComPtr<ID2D1SolidColorBrush> brush;
            explicit Pen(ID2D1RenderTarget* t) : target(t) { t->CreateSolidColorBrush(D2D1::ColorF(0, 0, 0), brush.GetAddressOf()); }
            bool Ok() const { return brush != nullptr; }
            ID2D1SolidColorBrush* operator()(D2D1_COLOR_F color) { brush->SetColor(color); return brush.Get(); }
        };

        D2D1_COLOR_F Alpha(D2D1_COLOR_F color, float opacity) { color.a = opacity; return color; }
        D2D1_RECT_F Inflate(D2D1_RECT_F r, float d) { return D2D1::RectF(r.left - d, r.top - d, r.right + d, r.bottom + d); }
        D2D1_POINT_2F Center(D2D1_RECT_F r) { return D2D1::Point2F((r.left + r.right) / 2, (r.top + r.bottom) / 2); }
        float Luminance(D2D1_COLOR_F c) { return .2126f * c.r + .7152f * c.g + .0722f * c.b; }

        // The M3 inverse roles are not part of Palette; they follow from the theme the palette resolved to.
        struct Inverse { D2D1_COLOR_F surface, onSurface, primary; };
        Inverse ResolveInverse(const Palette& p)
        {
            if (p.highContrast) return { p.onSurface, p.surface, p.surface };
            if (Luminance(p.onSurface) < .5f)   // light theme: dark snackbar
                return { D2D1::ColorF(0x1f2630), D2D1::ColorF(0xe6edf3), D2D1::ColorF(0x8ec0ff) };
            return { D2D1::ColorF(0xe6edf3), D2D1::ColorF(0x1f2630), D2D1::ColorF(0x1d5fd1) };
        }

        D2D1_COLOR_F Dim(const Palette& p) { return p.highContrast ? D2D1::ColorF(GetRValue(GetSysColor(COLOR_GRAYTEXT)) / 255.f, GetGValue(GetSysColor(COLOR_GRAYTEXT)) / 255.f, GetBValue(GetSysColor(COLOR_GRAYTEXT)) / 255.f) : BlendColor(p.onSurface, p.surface, .38f); }

        void CloseGlyph(Pen& pen, D2D1_POINT_2F c, float half, D2D1_COLOR_F color, float width)
        {
            pen.target->DrawLine(D2D1::Point2F(c.x - half, c.y - half), D2D1::Point2F(c.x + half, c.y + half), pen(color), width);
            pen.target->DrawLine(D2D1::Point2F(c.x - half, c.y + half), D2D1::Point2F(c.x + half, c.y - half), pen(color), width);
        }
        void CheckGlyph(Pen& pen, D2D1_POINT_2F c, float s, D2D1_COLOR_F color, float width)
        {
            pen.target->DrawLine(D2D1::Point2F(c.x - .30f * s, c.y + .02f * s), D2D1::Point2F(c.x - .08f * s, c.y + .26f * s), pen(color), width);
            pen.target->DrawLine(D2D1::Point2F(c.x - .08f * s, c.y + .26f * s), D2D1::Point2F(c.x + .32f * s, c.y - .22f * s), pen(color), width);
        }
    }

    // ------------------------------------------------------------------ snackbar
    float SnackbarActionWidth(const CString& label)
    {
        if (label.IsEmpty()) return 0.f;
        return (std::max)(48.f, MeasureMaterialText(label, SnackbarTextSize, DWRITE_FONT_WEIGHT_MEDIUM).width + 2 * SnackbarActionPadding);
    }

    SnackbarLayout LayoutSnackbar(float width, bool twoLines, float actionWidth, bool hasClose)
    {
        SnackbarLayout l{};
        l.twoLines = twoLines;
        l.hasAction = actionWidth > 0;
        l.hasClose = hasClose;
        l.height = twoLines ? SnackbarTwoLines : SnackbarOneLine;
        float right = width - 8.f;
        if (hasClose)
        {
            const float top = (l.height - SnackbarIconTarget) / 2;
            l.close = D2D1::RectF(width - SnackbarIconTarget, top, width, top + SnackbarIconTarget);
            right = l.close.left;
        }
        float textRight = hasClose ? l.close.left : width - SnackbarPadding;
        if (l.hasAction)
        {
            const float top = (l.height - SnackbarActionHeight) / 2;
            l.action = D2D1::RectF(right - actionWidth, top, right, top + SnackbarActionHeight);
            textRight = l.action.left - 8.f;
        }
        l.text = D2D1::RectF(SnackbarPadding, 0, (std::max)(SnackbarPadding + 1.f, textRight), l.height);
        return l;
    }

    bool SnackbarNeedsTwoLines(const CString& text, float width, float actionWidth, bool hasClose)
    {
        const auto l = LayoutSnackbar(width, false, actionWidth, hasClose);
        return MeasureMaterialText(text, SnackbarTextSize, DWRITE_FONT_WEIGHT_NORMAL).width > l.text.right - l.text.left;
    }

    float SnackbarWidthFor(float hostWidth)
    {
        const float available = hostWidth - 2 * SnackbarMargin;
        return available >= SnackbarMinWidth ? (std::min)(available, SnackbarMaxWidth) : (std::max)(available, 200.f);
    }

    void DrawSnackbar(ID2D1RenderTarget* target, const Palette& palette, const SnackbarModel& model, const InputState& state)
    {
        Pen pen(target); if (!pen.Ok()) return;
        const auto size = target->GetSize();
        if (size.width < 40 || size.height < 24) return;
        const auto ink = ResolveInverse(palette);
        const auto layout = LayoutSnackbar(size.width, model.twoLines, SnackbarActionWidth(model.action), model.closable);
        target->Clear(ink.surface);
        DrawMaterialText(target, palette, model.text, layout.text, SnackbarTextSize, DWRITE_FONT_WEIGHT_NORMAL, ink.onSurface, DWRITE_TEXT_ALIGNMENT_LEADING, model.twoLines ? 2 : 1);
        if (layout.hasAction)
        {
            const bool hot = model.hoverPart == SnackbarPart::Action, pressed = model.pressedPart == SnackbarPart::Action;
            const bool focus = state.focused && model.focusPart == SnackbarPart::Action;
            const auto pill = D2D1::RoundedRect(layout.action, SnackbarActionHeight / 2, SnackbarActionHeight / 2);
            if (hot || pressed || focus) target->FillRoundedRectangle(pill, pen(Alpha(ink.primary, pressed || focus ? .12f : .08f)));
            DrawMaterialText(target, palette, model.action, layout.action, SnackbarTextSize, DWRITE_FONT_WEIGHT_MEDIUM, ink.primary, DWRITE_TEXT_ALIGNMENT_CENTER);
            if (focus) target->DrawRoundedRectangle(D2D1::RoundedRect(Inflate(layout.action, -1.f), SnackbarActionHeight / 2, SnackbarActionHeight / 2), pen(ink.onSurface), 2.f);
        }
        if (layout.hasClose)
        {
            const bool hot = model.hoverPart == SnackbarPart::Close, pressed = model.pressedPart == SnackbarPart::Close;
            const bool focus = state.focused && model.focusPart == SnackbarPart::Close;
            const auto c = Center(layout.close);
            if (hot || pressed || focus) target->FillEllipse(D2D1::Ellipse(c, 20, 20), pen(Alpha(ink.onSurface, pressed || focus ? .12f : .08f)));
            CloseGlyph(pen, c, 6.f, ink.onSurface, 2.f);
            if (focus) target->DrawEllipse(D2D1::Ellipse(c, 19, 19), pen(ink.onSurface), 2.f);
        }
        // Focus on the message itself (neither the action nor the close control is selected).
        if (state.focused && model.focusPart == SnackbarPart::None)
            target->DrawRoundedRectangle(D2D1::RoundedRect(D2D1::RectF(2, 2, size.width - 2, size.height - 2), SnackbarCorner, SnackbarCorner), pen(ink.onSurface), 2.f);
    }

    // ------------------------------------------------------------------ progress
    ProgressLayout LayoutProgress(float width, float height, ProgressShape shape, bool wantsText)
    {
        ProgressLayout l{};
        if (shape == ProgressShape::Linear)
        {
            l.hasText = wantsText && height >= LinearWithText - 4.f;
            const float top = l.hasText ? ProgressTextRow + 8.f : (height - LinearTrack) / 2;
            l.indicator = D2D1::RectF(0, top, width, top + LinearTrack);
            l.text = l.hasText ? D2D1::RectF(0, 0, width, ProgressTextRow) : D2D1::RectF();
            return l;
        }
        const float d = (std::max)(CircularMinimum, (std::min)(CircularDefault, (std::min)(width, height)));
        const float top = (height - d) / 2;
        l.indicator = D2D1::RectF(0, top, d, top + d);
        l.hasText = wantsText && width >= d + ProgressTextGap + 24.f;
        l.text = l.hasText ? D2D1::RectF(d + ProgressTextGap, 0, width, height) : D2D1::RectF();
        return l;
    }

    namespace
    {
        void Arc(Pen& pen, D2D1_POINT_2F c, float radius, float startDegrees, float sweepDegrees, D2D1_COLOR_F color, float width)
        {
            if (sweepDegrees <= 0.f) return;
            if (sweepDegrees >= 359.5f) { pen.target->DrawEllipse(D2D1::Ellipse(c, radius, radius), pen(color), width); return; }
            ComPtr<ID2D1Factory> factory;
            pen.target->GetFactory(factory.GetAddressOf());
            ComPtr<ID2D1PathGeometry> path;
            ComPtr<ID2D1GeometrySink> sink;
            ComPtr<ID2D1StrokeStyle> style;
            if (!factory || FAILED(factory->CreatePathGeometry(path.GetAddressOf())) || FAILED(path->Open(sink.GetAddressOf()))) return;
            factory->CreateStrokeStyle(D2D1::StrokeStyleProperties(D2D1_CAP_STYLE_ROUND, D2D1_CAP_STYLE_ROUND, D2D1_CAP_STYLE_ROUND), nullptr, 0, style.GetAddressOf());
            const float pi = 3.14159265f;
            auto at = [&](float degrees) { const float r = degrees * pi / 180.f; return D2D1::Point2F(c.x + radius * std::sin(r), c.y - radius * std::cos(r)); };
            sink->BeginFigure(at(startDegrees), D2D1_FIGURE_BEGIN_HOLLOW);
            sink->AddArc(D2D1::ArcSegment(at(startDegrees + sweepDegrees), D2D1::SizeF(radius, radius), 0.f, D2D1_SWEEP_DIRECTION_CLOCKWISE,
                sweepDegrees > 180.f ? D2D1_ARC_SIZE_LARGE : D2D1_ARC_SIZE_SMALL));
            sink->EndFigure(D2D1_FIGURE_END_OPEN);
            sink->Close();
            pen.target->DrawGeometry(path.Get(), pen(color), width, style.Get());
        }
    }

    void DrawProgress(ID2D1RenderTarget* target, const Palette& palette, const ProgressModel& model, const InputState& state)
    {
        Pen pen(target); if (!pen.Ok()) return;
        const auto size = target->GetSize();
        if (size.width < 8 || size.height < 4) return;
        const bool wantsText = !model.status.IsEmpty();
        const auto layout = LayoutProgress(size.width, size.height, model.shape, wantsText);
        const D2D1_COLOR_F track = state.enabled ? palette.secondaryContainer : BlendColor(palette.onSurface, palette.surface, .12f);
        const D2D1_COLOR_F active = state.enabled ? palette.primary : Dim(palette);
        const float fraction = (std::max)(0.f, (std::min)(1.f, model.fraction));
        if (layout.hasText)
            DrawMaterialText(target, palette, model.status, layout.text, ProgressTextSize, DWRITE_FONT_WEIGHT_NORMAL, state.enabled ? palette.onSurfaceVariant : Dim(palette),
                model.shape == ProgressShape::Linear ? DWRITE_TEXT_ALIGNMENT_LEADING : DWRITE_TEXT_ALIGNMENT_LEADING);
        if (model.shape == ProgressShape::Linear)
        {
            const auto rail = layout.indicator;
            const float w = rail.right - rail.left, radius = LinearTrack / 2;
            target->FillRoundedRectangle(D2D1::RoundedRect(rail, radius, radius), pen(track));
            float from = 0.f, to = 0.f;
            if (!model.indeterminate) to = w * fraction;
            else if (model.reducedMotion) { from = w * .3f; to = w * .7f; }
            else { const float segment = w * .4f; from = -segment + model.phase * (w + segment); to = from + segment; }
            from = (std::max)(0.f, from); to = (std::min)(w, to);
            if (to - from >= 1.f)
            {
                target->PushAxisAlignedClip(rail, D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
                target->FillRoundedRectangle(D2D1::RoundedRect(D2D1::RectF(rail.left + from, rail.top, rail.left + to, rail.bottom), radius, radius), pen(active));
                target->PopAxisAlignedClip();
            }
            return;
        }
        const auto box = layout.indicator;
        const auto c = Center(box);
        const float radius = (box.right - box.left - CircularStroke) / 2;
        target->DrawEllipse(D2D1::Ellipse(c, radius, radius), pen(track), CircularStroke);
        if (!model.indeterminate) { Arc(pen, c, radius, 0.f, 360.f * fraction, active, CircularStroke); return; }
        if (model.reducedMotion) { Arc(pen, c, radius, 0.f, 270.f, active, CircularStroke); return; }
        const float pi = 3.14159265f;
        Arc(pen, c, radius, std::fmod(model.phase * 1080.f, 360.f), 60.f + 180.f * (.5f - .5f * std::cos(2 * pi * model.phase)), active, CircularStroke);
    }

    // --------------------------------------------------------------------- chips
    float ChipWidth(const CString& label, ChipKind kind, bool selected)
    {
        const float text = MeasureMaterialText(label, ChipTextSize, DWRITE_FONT_WEIGHT_MEDIUM).width;
        const float lead = ChipHasCheck(kind, selected) ? ChipIconPadding + ChipIcon + ChipIconPadding : ChipPadding;
        const float trail = ChipHasClose(kind) ? ChipIconPadding + ChipIcon + ChipIconPadding : ChipPadding;
        return 2 * ChipInset + lead + (std::ceil)(text) + trail;
    }

    ChipLayout LayoutChip(float width, float height, ChipKind kind, bool selected)
    {
        ChipLayout l{};
        const float top = (height - ChipHeight) / 2;
        l.body = D2D1::RectF(ChipInset, top, width - ChipInset, top + ChipHeight);
        l.hasIcon = ChipHasCheck(kind, selected);
        l.hasClose = ChipHasClose(kind);
        float left = l.body.left + ChipPadding, right = l.body.right - ChipPadding;
        if (l.hasIcon)
        {
            l.icon = D2D1::RectF(l.body.left + ChipIconPadding, top + (ChipHeight - ChipIcon) / 2, l.body.left + ChipIconPadding + ChipIcon, top + (ChipHeight + ChipIcon) / 2);
            left = l.icon.right + ChipIconPadding;
        }
        if (l.hasClose)
        {
            l.close = D2D1::RectF(l.body.right - ChipIconPadding - ChipIcon, top + (ChipHeight - ChipIcon) / 2, l.body.right - ChipIconPadding, top + (ChipHeight + ChipIcon) / 2);
            right = l.close.left - ChipIconPadding;
        }
        l.text = D2D1::RectF(left, top, (std::max)(left + 1.f, right), top + ChipHeight);
        return l;
    }

    void DrawChip(ID2D1RenderTarget* target, const Palette& palette, const ChipModel& model, const InputState& state)
    {
        Pen pen(target); if (!pen.Ok()) return;
        const auto size = target->GetSize();
        if (size.width < 24 || size.height < 24) return;
        const bool filled = model.selected && (model.kind == ChipKind::Filter || model.kind == ChipKind::Input);
        const auto layout = LayoutChip(size.width, size.height, model.kind, filled);
        const D2D1_COLOR_F label = !state.enabled ? Dim(palette) : filled ? palette.onSecondaryContainer : palette.onSurfaceVariant;
        const auto shape = D2D1::RoundedRect(layout.body, ChipCorner, ChipCorner);
        if (filled) target->FillRoundedRectangle(shape, pen(state.enabled ? palette.secondaryContainer : BlendColor(palette.onSurface, palette.surface, .12f)));
        else
        {
            const D2D1_COLOR_F line = !state.enabled ? BlendColor(palette.onSurface, palette.surface, .12f) : palette.outline;
            target->DrawRoundedRectangle(D2D1::RoundedRect(Inflate(layout.body, -.5f), ChipCorner, ChipCorner), pen(line), 1.f);
        }
        const bool bodyHot = state.enabled && model.hoverPart == ChipPart::Body;
        const bool bodyPressed = state.enabled && model.pressedPart == ChipPart::Body;
        if (bodyHot || bodyPressed || (state.enabled && state.focused))
            target->FillRoundedRectangle(shape, pen(Alpha(label, bodyPressed || state.focused ? .12f : .08f)));
        if (layout.hasIcon) CheckGlyph(pen, Center(layout.icon), ChipIcon, label, 2.f);
        DrawMaterialText(target, palette, model.label, layout.text, ChipTextSize, DWRITE_FONT_WEIGHT_MEDIUM, label);
        if (layout.hasClose)
        {
            const auto c = Center(layout.close);
            const bool hot = state.enabled && model.hoverPart == ChipPart::Close, pressed = state.enabled && model.pressedPart == ChipPart::Close;
            if (hot || pressed) target->FillEllipse(D2D1::Ellipse(c, 12, 12), pen(Alpha(label, pressed ? .12f : .08f)));
            CloseGlyph(pen, c, 4.5f, label, 1.8f);
        }
        if (state.enabled && state.focused) target->DrawRoundedRectangle(D2D1::RoundedRect(Inflate(layout.body, 1.f), ChipCorner + 1, ChipCorner + 1), pen(palette.primary), 2.f);
    }
}
