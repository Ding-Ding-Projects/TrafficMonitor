#include "../stdafx.h"
#include "TableVisuals.h"
#include <algorithm>
#include <cmath>

namespace NativeMaterial
{
    using namespace TableMetrics;
    using Microsoft::WRL::ComPtr;

    TableLayout LayoutColumns(const std::vector<TableColumn>& columns, float viewportWidth, bool checkable, bool fillLast)
    {
        TableLayout layout;
        layout.checkWidth = checkable ? CheckColumn : 0.f;
        float x = layout.checkWidth;
        for (const auto& column : columns)
        {
            const float width = (std::max)(MinimumColumn, static_cast<float>(column.width));
            layout.x.push_back(x);
            layout.width.push_back(width);
            x += width;
        }
        if (fillLast && !layout.width.empty() && x < viewportWidth)
        {
            layout.width.back() += viewportWidth - x;
            x = viewportWidth;
        }
        layout.total = x;
        return layout;
    }

    namespace
    {
        // One brush and one DirectWrite factory for a single paint.
        struct Ink
        {
            ID2D1RenderTarget* target;
            const Palette& p;
            ComPtr<ID2D1SolidColorBrush> brush;
            ComPtr<IDWriteFactory> text;
            Ink(ID2D1RenderTarget* t, const Palette& palette) : target(t), p(palette)
            {
                t->CreateSolidColorBrush(palette.primary, brush.GetAddressOf());
                DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), reinterpret_cast<IUnknown**>(text.GetAddressOf()));
            }
            bool Ok() const { return brush && text; }
            ID2D1SolidColorBrush* operator()(D2D1_COLOR_F color) { brush->SetColor(color); return brush.Get(); }
            D2D1_COLOR_F Dim() const
            {
                if (p.highContrast) { const COLORREF g = GetSysColor(COLOR_GRAYTEXT); return D2D1::ColorF(GetRValue(g) / 255.f, GetGValue(g) / 255.f, GetBValue(g) / 255.f); }
                return BlendColor(p.onSurface, p.surface, .38f);
            }
            D2D1_COLOR_F Layer(D2D1_COLOR_F color, float opacity) const { color.a = opacity; return color; }
        };

        // Single-line text, vertically centered, trimmed with an ellipsis and clipped to its rectangle.
        void Text(Ink& k, const CString& s, D2D1_RECT_F r, float size, DWRITE_FONT_WEIGHT weight, D2D1_COLOR_F color, DWRITE_TEXT_ALIGNMENT align)
        {
            if (s.IsEmpty() || r.right - r.left < 2 || r.bottom - r.top < 2) return;
            ComPtr<IDWriteTextFormat> format; ComPtr<IDWriteTextLayout> layout;
            if (FAILED(k.text->CreateTextFormat(L"Segoe UI", nullptr, weight, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, size, L"", format.GetAddressOf()))) return;
            format->SetTextAlignment(align);
            format->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
            format->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
            ComPtr<IDWriteInlineObject> sign;
            if (SUCCEEDED(k.text->CreateEllipsisTrimmingSign(format.Get(), sign.GetAddressOf())))
            {
                DWRITE_TRIMMING trimming{ DWRITE_TRIMMING_GRANULARITY_CHARACTER, 0, 0 };
                format->SetTrimming(&trimming, sign.Get());
            }
            if (FAILED(k.text->CreateTextLayout(s, s.GetLength(), format.Get(), r.right - r.left, r.bottom - r.top, layout.GetAddressOf()))) return;
            k.target->DrawTextLayout(D2D1::Point2F(r.left, r.top), layout.Get(), k(color), D2D1_DRAW_TEXT_OPTIONS_CLIP);
        }

        DWRITE_TEXT_ALIGNMENT Alignment(ColumnAlign align)
        {
            return align == ColumnAlign::Right ? DWRITE_TEXT_ALIGNMENT_TRAILING : align == ColumnAlign::Center ? DWRITE_TEXT_ALIGNMENT_CENTER : DWRITE_TEXT_ALIGNMENT_LEADING;
        }

        // Converts an icon to a premultiplied bitmap by drawing it over black and over white, which
        // recovers the alpha of both masked and 32-bit icons.
        ComPtr<ID2D1Bitmap> IconBitmap(ID2D1RenderTarget* target, HICON icon, int px)
        {
            ComPtr<ID2D1Bitmap> bitmap;
            if (!icon || px < 4) return bitmap;
            HDC dc = CreateCompatibleDC(nullptr);
            BITMAPINFO info{};
            info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER); info.bmiHeader.biWidth = px; info.bmiHeader.biHeight = -px;
            info.bmiHeader.biPlanes = 1; info.bmiHeader.biBitCount = 32; info.bmiHeader.biCompression = BI_RGB;
            void* blackBits{}; void* whiteBits{};
            HBITMAP black = CreateDIBSection(dc, &info, DIB_RGB_COLORS, &blackBits, nullptr, 0);
            HBITMAP white = CreateDIBSection(dc, &info, DIB_RGB_COLORS, &whiteBits, nullptr, 0);
            if (black && white && blackBits && whiteBits)
            {
                const size_t count = static_cast<size_t>(px) * px;
                auto previous = SelectObject(dc, black);
                memset(blackBits, 0, count * 4);
                DrawIconEx(dc, 0, 0, icon, px, px, 0, nullptr, DI_NORMAL);
                GdiFlush();
                SelectObject(dc, white);
                memset(whiteBits, 0xFF, count * 4);
                DrawIconEx(dc, 0, 0, icon, px, px, 0, nullptr, DI_NORMAL);
                GdiFlush();
                SelectObject(dc, previous);
                std::vector<DWORD> pixels(count);
                const auto* onBlack = static_cast<const DWORD*>(blackBits);
                const auto* onWhite = static_cast<const DWORD*>(whiteBits);
                for (size_t i = 0; i < count; ++i)
                {
                    const int alpha = (std::max)(0, (std::min)(255, 255 - (static_cast<int>((onWhite[i] >> 8) & 255) - static_cast<int>((onBlack[i] >> 8) & 255))));
                    // The black render is already premultiplied by the icon alpha; keep its color channels within it.
                    const DWORD r = (std::min)(static_cast<DWORD>(alpha), (onBlack[i] >> 16) & 255);
                    const DWORD g = (std::min)(static_cast<DWORD>(alpha), (onBlack[i] >> 8) & 255);
                    const DWORD b = (std::min)(static_cast<DWORD>(alpha), onBlack[i] & 255);
                    pixels[i] = (static_cast<DWORD>(alpha) << 24) | (r << 16) | (g << 8) | b;
                }
                float dpiX = 96.f, dpiY = 96.f;
                target->GetDpi(&dpiX, &dpiY);
                target->CreateBitmap(D2D1::SizeU(px, px), pixels.data(), static_cast<UINT32>(px * 4),
                    D2D1::BitmapProperties(D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED), dpiX, dpiY), bitmap.GetAddressOf());
            }
            if (black) DeleteObject(black);
            if (white) DeleteObject(white);
            DeleteDC(dc);
            return bitmap;
        }

        void SortArrow(Ink& k, D2D1_POINT_2F c, SortDirection direction, D2D1_COLOR_F color)
        {
            const float up = direction == SortDirection::Ascending ? -1.f : 1.f;
            k.target->DrawLine(D2D1::Point2F(c.x, c.y + 5.f * up), D2D1::Point2F(c.x, c.y - 5.f * up), k(color), 2.f);
            k.target->DrawLine(D2D1::Point2F(c.x - 4.f, c.y - 1.f * up), D2D1::Point2F(c.x, c.y - 5.f * up), k(color), 2.f);
            k.target->DrawLine(D2D1::Point2F(c.x + 4.f, c.y - 1.f * up), D2D1::Point2F(c.x, c.y - 5.f * up), k(color), 2.f);
        }
    }

    void DrawDataTable(ID2D1RenderTarget* target, const Palette& c, const TablePaintModel& m, const InputState& s)
    {
        Ink k(target, c); if (!k.Ok()) return;
        const auto size = target->GetSize();
        if (size.width < 8 || size.height < 8) return;
        float dpiX = 96.f, dpiY = 96.f;
        target->GetDpi(&dpiX, &dpiY);
        const float width = m.viewportWidth > 0 ? (std::min)(m.viewportWidth, size.width) : size.width;
        const float headerHeight = m.showHeader ? HeaderHeight : 0.f;
        const auto layout = LayoutColumns(m.columns, width, m.checkable, m.fillLast);
        const auto text = s.enabled ? c.onSurface : k.Dim();

        // ------------------------------------------------------------------ body
        const auto body = D2D1::RectF(0, headerHeight, width, size.height);
        target->PushAxisAlignedClip(body, D2D1_ANTIALIAS_MODE_ALIASED);
        if (m.rowCount == 0)
            Text(k, m.emptyText, D2D1::RectF(body.left + CellPadding, body.top, body.right - CellPadding, (std::min)(body.bottom, body.top + 3 * RowHeight)),
                14.f, DWRITE_FONT_WEIGHT_NORMAL, s.enabled ? c.onSurfaceVariant : k.Dim(), DWRITE_TEXT_ALIGNMENT_CENTER);
        for (const auto& row : m.rows)
        {
            const float top = headerHeight + row.index * RowHeight - m.scrollY;
            const auto rect = D2D1::RectF(0, top, width, top + RowHeight);
            if (rect.bottom <= body.top || rect.top >= body.bottom) continue;
            if (row.selected) target->FillRectangle(rect, k(s.enabled ? c.secondaryContainer : k.Layer(c.onSurface, .12f)));
            if (s.enabled && row.hover)
                target->FillRectangle(rect, k(k.Layer(row.selected ? c.onSecondaryContainer : c.onSurface, .08f)));
            target->DrawLine(D2D1::Point2F(0, rect.bottom - .5f), D2D1::Point2F(width, rect.bottom - .5f), k(c.outlineVariant), 1.f);
            const auto rowText = row.selected && s.enabled ? c.onSecondaryContainer : text;
            if (m.checkable)
            {
                InputState item = s;
                item.hover = row.hover; item.focused = false; item.pressed = false;
                DrawCheckGlyph(target, c, D2D1::Point2F(layout.checkWidth / 2 - m.scrollX, (rect.top + rect.bottom) / 2), row.check, item);
            }
            for (size_t col = 0; col < m.columns.size() && col < row.cells.size(); ++col)
            {
                const float left = layout.x[col] - m.scrollX, right = left + layout.width[col];
                if (right <= 0 || left >= width) continue;
                if (m.editRow == row.index && m.editColumn == static_cast<int>(col)) continue;
                const auto& cell = row.cells[col];
                float x = left + CellPadding;
                const float limit = right - CellPadding;
                const float middle = (rect.top + rect.bottom) / 2;
                if (cell.icon && x + IconSize <= limit)
                {
                    const int px = static_cast<int>(std::lround(IconSize * dpiX / 96.f));
                    if (auto bitmap = IconBitmap(target, cell.icon, px))
                        target->DrawBitmap(bitmap.Get(), D2D1::RectF(x, middle - IconSize / 2, x + IconSize, middle + IconSize / 2));
                    x += IconSize + ContentGap;
                }
                if (cell.hasSwatch && x + SwatchSize <= limit)
                {
                    const auto box = D2D1::RectF(x, middle - SwatchSize / 2, x + SwatchSize, middle + SwatchSize / 2);
                    target->FillRoundedRectangle(D2D1::RoundedRect(box, 3, 3), k(D2D1::ColorF(GetRValue(cell.swatch) / 255.f, GetGValue(cell.swatch) / 255.f, GetBValue(cell.swatch) / 255.f)));
                    target->DrawRoundedRectangle(D2D1::RoundedRect(D2D1::RectF(box.left + .5f, box.top + .5f, box.right - .5f, box.bottom - .5f), 3, 3), k(c.outline), 1.f);
                    x += SwatchSize + ContentGap;
                }
                const bool hasOther = x > left + CellPadding;
                Text(k, cell.text, D2D1::RectF(x, rect.top, (std::max)(x, limit), rect.bottom), 14.f, DWRITE_FONT_WEIGHT_NORMAL, rowText,
                    hasOther ? DWRITE_TEXT_ALIGNMENT_LEADING : Alignment(m.columns[col].align));
            }
            if (row.focused && s.focused)
                target->DrawRectangle(D2D1::RectF(rect.left + 2, rect.top + 2, rect.right - 2, rect.bottom - 2), k(c.primary), 2.f);
        }
        if (m.dropGap >= 0)
        {
            const float y = headerHeight + m.dropGap * RowHeight - m.scrollY;
            target->FillRectangle(D2D1::RectF(0, y - 1.f, width, y + 1.f), k(c.primary));
        }
        target->PopAxisAlignedClip();

        // ---------------------------------------------------------------- header
        if (!m.showHeader) return;
        const auto header = D2D1::RectF(0, 0, width, HeaderHeight);
        target->FillRectangle(header, k(c.surfaceContainer));
        target->PushAxisAlignedClip(header, D2D1_ANTIALIAS_MODE_ALIASED);
        for (size_t col = 0; col < m.columns.size() && col < layout.x.size(); ++col)
        {
            const float left = layout.x[col] - m.scrollX, right = left + layout.width[col];
            if (right <= 0 || left >= width) continue;
            const auto& column = m.columns[col];
            const bool sorted = static_cast<int>(col) == m.sortColumn && m.sortDirection != SortDirection::None;
            if (s.enabled && column.sortable && static_cast<int>(col) == m.hoverHeader)
                target->FillRectangle(D2D1::RectF((std::max)(0.f, left), 0, (std::min)(width, right), HeaderHeight), k(k.Layer(c.onSurface, .08f)));
            const float arrowSpace = sorted ? 24.f : 0.f;
            const bool rightAligned = column.align == ColumnAlign::Right;
            const auto label = D2D1::RectF(left + CellPadding + (rightAligned ? arrowSpace : 0.f), 0, (std::max)(left + CellPadding, right - CellPadding - (rightAligned ? 0.f : arrowSpace)), HeaderHeight);
            Text(k, column.title, label, 14.f, DWRITE_FONT_WEIGHT_SEMI_BOLD, text, Alignment(column.align));
            if (sorted)
            {
                const float cx = rightAligned ? left + CellPadding + 6.f : right - CellPadding - 6.f;
                if (cx > left && cx < right) SortArrow(k, D2D1::Point2F(cx, HeaderHeight / 2), m.sortDirection, s.enabled ? c.primary : k.Dim());
            }
            if (m.resizeHot == static_cast<int>(col))
                target->FillRectangle(D2D1::RectF(right - 1.f, 8.f, right + 1.f, HeaderHeight - 8.f), k(c.primary));
        }
        target->PopAxisAlignedClip();
        target->DrawLine(D2D1::Point2F(0, HeaderHeight - .5f), D2D1::Point2F(width, HeaderHeight - .5f), k(c.outline), 1.f);
    }
}
