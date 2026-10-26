#pragma once
#include "PickerDialog.h"
#include "InputControls.h"
#include "MaterialList.h"
#include <vector>

// Material 3 font picker: a modal dialog (no common font dialog, no stock control) with a searchable list of
// the installed font families, a size stepper, bold / italic / underline / strikeout options and a live preview.
//
//   LOGFONT font{};  wcscpy_s(font.lfFaceName, L"Segoe UI");  int points = 10;
//   if (NativeMaterial::PickFont(hwnd, font, points)) { /* font.lfFaceName, lfWeight, lfItalic ..., points */ }
//
// It carries what the CFontDialog call sites read: face name, point size, bold, italic, underline, strikeout.
// Vertical ("@") families are never listed (the CF_NOVERTFONTS behavior of the old dialog). The search is a
// case-insensitive substring filter, or an ECMAScript regular expression with the Regex option on.
namespace NativeMaterial
{
    constexpr int FontPointsMin = 1, FontPointsMax = 200;
    int ClampFontPoints(int points);

    // Installed font families: sorted case-insensitively, unique, without "@" vertical families.
    std::vector<CString> EnumerateFontFamilies();
    // Indexes into `families` that match the query (all of them when the query is empty). An invalid regular
    // expression yields no rows and fills `error`.
    std::vector<int> FilterFontFamilies(const std::vector<CString>& families, const CString& query, bool regex, CString* error);

    // The font the preview and the result describe.
    struct FontChoice
    {
        CString face;
        int points{ 10 };
        bool bold{}, italic{}, underline{}, strike{};
    };
    // Applies a choice onto a LOGFONT the way the dialog returns it (face, weight, flags, height from the point size).
    void ApplyFontChoice(const FontChoice& choice, LOGFONT& font);
    FontChoice ChoiceFromLogFont(const LOGFONT& font, int points);
    // Direct2D drawing of the preview panel (96-dpi units).
    void DrawFontPreview(ID2D1RenderTarget*, const Palette&, const FontChoice&, D2D1_RECT_F bounds);

    class FontPickerDialog final : public PickerDialog
    {
    public:
        FontPickerDialog();
        ~FontPickerDialog() override;
        // Runs the dialog. Returns true and updates the font and size when the user accepts.
        bool Run(HWND owner, LOGFONT& font, int& points);
        FontChoice Choice() const;
        LabeledTextField& Search() { return m_search; }
        Checkbox& RegexOption() { return m_regex; }
        MaterialList& List() { return m_list; }
        NumberStepper& Size() { return m_size; }
        Checkbox& Bold() { return m_bold; }
        Checkbox& Italic() { return m_italic; }
        Checkbox& Underline() { return m_underline; }
        Checkbox& Strike() { return m_strike; }
        // Families currently shown in the list (after filtering).
        const std::vector<CString>& VisibleFamilies() const { return m_visible; }
        const std::vector<CString>& AllFamilies() const { return m_families; }
        // Selects a family as a user choice would (updates the list selection and the preview).
        bool ChooseFamily(const CString& face);
        static FontPickerDialog* FromHandle(HWND dialog);
        enum ControlId : UINT { IdSearch = 200, IdRegex, IdList, IdSize, IdBold, IdItalic, IdUnderline, IdStrike };
    protected:
        void CreateContent() override;
        void LayoutContent() override;
        void PaintContent(ID2D1RenderTarget*, const Palette&) override;
        std::vector<HWND> ContentFocusOrder() const override;
        HWND InitialFocus() const override { return m_list.GetSafeHwnd(); }
        bool CommitPending() override;
        bool ContentCommand(UINT id, UINT code) override;
    private:
        void Refilter();
        void SyncSelection();
        LabeledTextField m_search;
        Checkbox m_regex, m_bold, m_italic, m_underline, m_strike;
        MaterialList m_list;
        NumberStepper m_size;
        std::vector<CString> m_families, m_visible;
        FontChoice m_choice;
        int m_boldWeight{ FW_BOLD };
        bool m_updating{};
    };

    bool PickFont(HWND owner, LOGFONT& inOut, int& pointSize);
}
