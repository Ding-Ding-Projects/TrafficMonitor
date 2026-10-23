#pragma once
#include "ScrollRail.h"
namespace NativeMaterial
{
    class ScrollViewport : public CWnd
    {
    public:
        BOOL Create(CWnd* parent,UINT id=53999);
        void SetMetrics(int content,int page,int position);
        void SetAppearance(Appearance);
        void SetScrollName(const CString& name){m_rail.SetWindowText(name);}
        std::function<void(UINT,int)> scroll;
        std::function<HBRUSH()> background;
    protected:
        LRESULT WindowProc(UINT,WPARAM,LPARAM) override;
    private:
        ScrollRail m_rail;int m_position{},m_wheel{};HFONT m_font{};
    };
}
