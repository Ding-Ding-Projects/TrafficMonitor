#pragma once
#include "ScrollRail.h"
#include <afxwin.h>
namespace NativeMaterial
{
    class ResultList : public CListBox
    {
    public:
        BOOL Create(const RECT&,CWnd*,UINT,bool navigation=false);
        void SetAppearance(Appearance);
        void DrawItem(LPDRAWITEMSTRUCT) override;
        void MeasureItem(LPMEASUREITEMSTRUCT) override;
        void SyncScroll();
    protected:
        LRESULT WindowProc(UINT,WPARAM,LPARAM) override;
    private:
        ScrollRail m_rail;Appearance m_appearance{Appearance::Light};bool m_navigation{},m_sync{};int m_wheel{};
    };
    using Navigation=ResultList;
}
