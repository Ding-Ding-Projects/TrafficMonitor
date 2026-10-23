#pragma once
#include "Controls.h"
#include "../NativeRegex/Engine.h"
#include <vector>
namespace NativeMaterial
{
    // A named native button opens a searchable, checked menu, not a combo proxy.
    class SearchSelector final : public CWnd
    {
    public:
        BOOL Create(const RECT&, CWnd*, UINT);
        void SetAppearance(Appearance);
        void SetSurfaceColor(COLORREF color){m_button.SetSurfaceColor(color);}
        void ResetContent();
        void AddString(const CString&);
        int GetCurSel() const { return m_selected; }
        void SetPrompt(const CString&);
    protected:
        LRESULT WindowProc(UINT, WPARAM, LPARAM) override;
    private:
        void Choose();
        Button m_button;
        std::vector<CString> m_items;
        int m_selected{-1};
        CString m_prompt;
        NativeRegex::Options m_filter;
    };
}
