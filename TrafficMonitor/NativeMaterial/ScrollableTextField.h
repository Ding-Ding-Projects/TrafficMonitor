#pragma once
#include "TextField.h"
#include "ScrollRail.h"
namespace NativeMaterial
{
    // The rail follows the native EDIT's wrapped display lines and caret scrolling.
    class ScrollableTextField final : public TextField
    {
    public:
        void SetAppearance(Appearance);
        void SyncScroll();
    protected:
        LRESULT WindowProc(UINT, WPARAM, LPARAM) override;
    private:
        ScrollRail m_rail;
        bool m_sync{};int m_wheel{};
    };
}
