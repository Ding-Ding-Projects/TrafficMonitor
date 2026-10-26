#include "../stdafx.h"
#include "FontPicker.h"
#include "PickerDraw.h"
#include "InputControls.Internal.h"
#include <algorithm>
#include <cstdlib>

namespace NativeMaterial
{
    using PickerDraw::Painter;
    using PickerDraw::TextStyle;

    namespace
    {
        CString Words(const wchar_t* english, const wchar_t* cantonese) { return NativePreferences::InlinePair(english, cantonese); }

        // Dialog geometry in DIPs.
        constexpr D2D1_RECT_F SearchRect{ 24, 68, 296, 154 };
        constexpr D2D1_RECT_F RegexRect{ 24, 154, 296, 202 };
        constexpr D2D1_RECT_F ListRect{ 24, 206, 296, 446 };
        constexpr D2D1_RECT_F PreviewRect{ 24, 456, 536, 520 };
        constexpr D2D1_RECT_F SizeRect{ 320, 68, 536, 152 };
        constexpr D2D1_RECT_F BoldRect{ 320, 164, 536, 212 };
        constexpr D2D1_RECT_F ItalicRect{ 320, 212, 536, 260 };
        constexpr D2D1_RECT_F UnderlineRect{ 320, 260, 536, 308 };
        constexpr D2D1_RECT_F StrikeRect{ 320, 308, 536, 356 };

        BOOL CALLBACK Collect(const LOGFONTW* font, const TEXTMETRICW*, DWORD, LPARAM param)
        {
            auto* families = reinterpret_cast<std::vector<CString>*>(param);
            if (font->lfFaceName[0] != L'\0' && font->lfFaceName[0] != L'@') families->push_back(CString(font->lfFaceName));
            return TRUE;
        }
    }

    int ClampFontPoints(int points) { return (std::max)(FontPointsMin, (std::min)(FontPointsMax, points)); }

    std::vector<CString> EnumerateFontFamilies()
    {
        std::vector<CString> families;
        HDC dc = GetDC(nullptr);
        LOGFONTW filter{};
        filter.lfCharSet = DEFAULT_CHARSET;
        EnumFontFamiliesExW(dc, &filter, Collect, reinterpret_cast<LPARAM>(&families), 0);
        ReleaseDC(nullptr, dc);
        std::sort(families.begin(), families.end(), [](const CString& a, const CString& b) { return a.CompareNoCase(b) < 0; });
        families.erase(std::unique(families.begin(), families.end(), [](const CString& a, const CString& b) { return a.CompareNoCase(b) == 0; }), families.end());
        return families;
    }

    std::vector<int> FilterFontFamilies(const std::vector<CString>& families, const CString& query, bool regex, CString* error)
    {
        return FilterDropdownItems(families, query, regex, error);
    }

    FontChoice ChoiceFromLogFont(const LOGFONT& font, int points)
    {
        FontChoice choice;
        choice.face = font.lfFaceName;
        choice.points = ClampFontPoints(points);
        choice.bold = font.lfWeight >= FW_SEMIBOLD;
        choice.italic = font.lfItalic != 0;
        choice.underline = font.lfUnderline != 0;
        choice.strike = font.lfStrikeOut != 0;
        return choice;
    }

    void ApplyFontChoice(const FontChoice& choice, LOGFONT& font)
    {
        wcsncpy_s(font.lfFaceName, LF_FACESIZE, choice.face.GetString(), _TRUNCATE);
        font.lfWeight = choice.bold ? FW_BOLD : FW_NORMAL;
        font.lfItalic = choice.italic ? TRUE : FALSE;
        font.lfUnderline = choice.underline ? TRUE : FALSE;
        font.lfStrikeOut = choice.strike ? TRUE : FALSE;
        font.lfHeight = -MulDiv(ClampFontPoints(choice.points), static_cast<int>(GetDpiForSystem()), 72);
    }

    void DrawFontPreview(ID2D1RenderTarget* target, const Palette& c, const FontChoice& choice, D2D1_RECT_F bounds)
    {
        Painter k(target, c);
        if (!k.Ok()) return;
        k.FillRounded(bounds, 12.f, c.container);
        k.StrokeRounded(bounds, 12.f, c.outlineVariant, 1.f);
        TextStyle caption;
        caption.size = 11.f;
        caption.weight = DWRITE_FONT_WEIGHT_MEDIUM;
        caption.center = false;
        CString line;
        line.Format(L"%s, %d pt", choice.face.GetString(), choice.points);
        k.Text(line, D2D1::RectF(bounds.left + 14, bounds.top + 6, bounds.right - 14, bounds.top + 22), caption, c.onSurfaceVariant);
        const float room = bounds.bottom - bounds.top - 28.f;
        TextStyle sample;
        sample.size = (std::max)(8.f, (std::min)(room, choice.points * 96.f / 72.f));
        sample.weight = choice.bold ? DWRITE_FONT_WEIGHT_BOLD : DWRITE_FONT_WEIGHT_NORMAL;
        sample.italic = choice.italic;
        sample.underline = choice.underline;
        sample.strike = choice.strike;
        sample.family = choice.face.GetString();
        k.Text(CString(L"AaBbYyZz 0123456789 TrafficMonitor 流量"), D2D1::RectF(bounds.left + 14, bounds.top + 22, bounds.right - 14, bounds.bottom - 4), sample, c.onSurface);
    }

    // ------------------------------------------------------------------ FontPickerDialog
    FontPickerDialog::FontPickerDialog() = default;
    FontPickerDialog::~FontPickerDialog() = default;

    FontPickerDialog* FontPickerDialog::FromHandle(HWND dialog) { return dynamic_cast<FontPickerDialog*>(PickerDialog::FromWindow(dialog)); }

    FontChoice FontPickerDialog::Choice() const { return m_choice; }

    bool FontPickerDialog::Run(HWND owner, LOGFONT& font, int& points)
    {
        int initial = points;
        if (initial <= 0)
        {
            const int dpi = static_cast<int>(GetDpiForSystem());
            initial = font.lfHeight != 0 ? MulDiv(std::abs(font.lfHeight), 72, dpi ? dpi : 96) : 10;
        }
        m_choice = ChoiceFromLogFont(font, initial);
        m_boldWeight = font.lfWeight >= FW_SEMIBOLD ? font.lfWeight : FW_BOLD;
        const int result = RunModal(owner, Words(L"Choose font", L"揀選字體"), 560, 600);
        if (result != IDOK || m_choice.face.IsEmpty()) return false;
        ApplyFontChoice(m_choice, font);
        if (m_choice.bold) font.lfWeight = m_boldWeight;      // an incoming semi-bold or heavier weight survives an untouched Bold option
        points = m_choice.points;
        return true;
    }

    void FontPickerDialog::CreateContent()
    {
        const CRect none(0, 0, 10, 10);
        m_families = EnumerateFontFamilies();
        m_search.Create(Words(L"Search fonts", L"搜尋字體"), none, this, IdSearch);
        m_regex.Create(Words(L"Regular expression", L"正則式"), none, this, IdRegex);
        m_list.Create(none, this, IdList);
        m_size.Create(Words(L"Size (pt)", L"大小 (pt)"), none, this, IdSize);
        m_bold.Create(Words(L"Bold", L"粗體"), none, this, IdBold);
        m_italic.Create(Words(L"Italic", L"斜體"), none, this, IdItalic);
        m_underline.Create(Words(L"Underline", L"底線"), none, this, IdUnderline);
        m_strike.Create(Words(L"Strikethrough", L"刪除線"), none, this, IdStrike);
        Style(m_search); Style(m_regex); Style(m_list); Style(m_size); Style(m_bold); Style(m_italic); Style(m_underline); Style(m_strike);
        m_list.SetAccessibleName(Words(L"Font families", L"字體家族"));
        m_search.SetSupportingText(Words(L"Type part of a name", L"輸入部分名稱"));
        m_size.SetRange(FontPointsMin, FontPointsMax);
        m_size.SetStep(1);
        m_size.SetValue(m_choice.points);
        m_bold.SetChecked(m_choice.bold);
        m_italic.SetChecked(m_choice.italic);
        m_underline.SetChecked(m_choice.underline);
        m_strike.SetChecked(m_choice.strike);
        Refilter();
    }

    void FontPickerDialog::LayoutContent()
    {
        auto place = [&](CWnd& window, D2D1_RECT_F r) { MovePx(window, r.left, r.top, r.right - r.left, r.bottom - r.top); };
        place(m_search, SearchRect);
        place(m_regex, RegexRect);
        place(m_list, ListRect);
        place(m_size, SizeRect);
        place(m_bold, BoldRect);
        place(m_italic, ItalicRect);
        place(m_underline, UnderlineRect);
        place(m_strike, StrikeRect);
    }

    void FontPickerDialog::PaintContent(ID2D1RenderTarget* target, const Palette& palette)
    {
        DrawFontPreview(target, palette, m_choice, PreviewRect);
    }

    std::vector<HWND> FontPickerDialog::ContentFocusOrder() const
    {
        return { const_cast<LabeledTextField&>(m_search).Field().GetSafeHwnd(), m_regex.GetSafeHwnd(), m_list.GetSafeHwnd(), m_size.EditWindow(),
            m_bold.GetSafeHwnd(), m_italic.GetSafeHwnd(), m_underline.GetSafeHwnd(), m_strike.GetSafeHwnd() };
    }

    bool FontPickerDialog::CommitPending()
    {
        const HWND focus = ::GetFocus();
        return !(focus && focus == m_size.EditWindow()) || m_size.CommitText();
    }

    void FontPickerDialog::Refilter()
    {
        CString error;
        const auto rows = FilterFontFamilies(m_families, m_search.GetText(), m_regex.IsChecked(), &error);
        m_visible.clear();
        for (const int row : rows) m_visible.push_back(m_families[row]);
        m_updating = true;
        m_list.SetItems(m_visible);
        m_search.SetErrorText(error);
        SyncSelection();
        m_updating = false;
        if (m_hWnd) Invalidate(FALSE);
    }

    void FontPickerDialog::SyncSelection()
    {
        int found = -1;
        for (size_t i = 0; i < m_visible.size(); ++i)
            if (m_visible[i].CompareNoCase(m_choice.face) == 0) { found = static_cast<int>(i); break; }
        m_list.SetCurSel(found);
    }

    bool FontPickerDialog::ChooseFamily(const CString& face)
    {
        for (size_t i = 0; i < m_visible.size(); ++i)
        {
            if (m_visible[i].CompareNoCase(face) != 0) continue;
            m_choice.face = m_visible[i];
            m_list.SetCurSel(static_cast<int>(i));
            if (m_hWnd) Invalidate(FALSE);
            return true;
        }
        return false;
    }

    bool FontPickerDialog::ContentCommand(UINT id, UINT code)
    {
        if (m_updating) return false;
        switch (id)
        {
        case IdSearch: if (code != EN_CHANGE) return false; Refilter(); return true;
        case IdRegex: if (code != BN_CLICKED) return false; Refilter(); return true;
        case IdList:
            if (code == TableSelectionChanged)
            {
                const int row = m_list.GetCurSel();
                if (row >= 0 && row < static_cast<int>(m_visible.size())) { m_choice.face = m_visible[row]; Invalidate(FALSE); }
                return true;
            }
            if (code == TableActivated) { if (!m_choice.face.IsEmpty()) Finish(IDOK); return true; }
            return false;
        case IdSize: if (code != EN_CHANGE) return false; m_choice.points = ClampFontPoints(m_size.GetValue()); Invalidate(FALSE); return true;
        case IdBold: if (code != BN_CLICKED) return false; m_choice.bold = m_bold.IsChecked(); Invalidate(FALSE); return true;
        case IdItalic: if (code != BN_CLICKED) return false; m_choice.italic = m_italic.IsChecked(); Invalidate(FALSE); return true;
        case IdUnderline: if (code != BN_CLICKED) return false; m_choice.underline = m_underline.IsChecked(); Invalidate(FALSE); return true;
        case IdStrike: if (code != BN_CLICKED) return false; m_choice.strike = m_strike.IsChecked(); Invalidate(FALSE); return true;
        default: return false;
        }
    }

    bool PickFont(HWND owner, LOGFONT& inOut, int& pointSize)
    {
        FontPickerDialog dialog;
        return dialog.Run(owner, inOut, pointSize);
    }
}
