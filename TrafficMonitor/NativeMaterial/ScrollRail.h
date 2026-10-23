#pragma once
#include "Theme.h"
#include <afxwin.h>
#include <functional>
#include <memory>
namespace NativeMaterial
{
    struct ScrollAccess;
    // Own HWND, native focus and a range provider. Coordinates are viewport units.
    class ScrollRail final : public CWnd
    {
    public:
        BOOL Create(CWnd* parent, UINT id);
        void SetMetrics(int content, int page, int position);
        void SetAppearance(Appearance value) { m_appearance=value; if(m_hWnd)Invalidate(); }
        std::function<void(int)> changed;
    protected:
        LRESULT WindowProc(UINT,WPARAM,LPARAM) override;
    private:
        void Paint(HDC); CRect Thumb() const; void Move(int);
        std::shared_ptr<ScrollAccess> m_access;
        Appearance m_appearance{Appearance::Light};
        int m_content{},m_page{},m_position{},m_dragOrigin{},m_dragPosition{},m_wheel{};
        bool m_drag{},m_hover{};
    };
}
