//这是用于Tab标签中的子对话框类
#pragma once
#include "BaseDialog.h"
#include "NativeMaterial/ScrollRail.h"

// A page of a tabbed host dialog (COptionsDlg). The host places the page with MoveWindow and then calls
// SetScrollbarInfo(); the page scrolls its content with a Material scroll rail (NativeMaterial::ScrollRail), never
// with a system scroll bar.
//
// Two kinds of page share this class:
//   Material page   the constructor calls EnableMaterialPage(); OnInitDialog replaces the stock controls through the
//                   CBaseDialog ReplaceWith* helpers and ends with FinishMaterialPage(). The page implements
//                   LayoutPage(), which places every control for the width it is given; the base scrolls the content
//                   by moving the controls and keeps the rail in step.
//   Template page   a page that still lays itself out from its dialog template and the dynamic layout manager. It is
//                   scrolled by moving its child windows. This kind only exists until the page is converted.
class CTabDlg : public CBaseDialog
{
    DECLARE_DYNAMIC(CTabDlg)
public:
    CTabDlg(UINT nIDTemplate, CWnd *pParent = NULL);
    ~CTabDlg();
    virtual BOOL PreTranslateMessage(MSG* pMsg);
    virtual BOOL OnInitDialog();
    CWnd* GetParentWindow();
    // nPage: height in pixels of the area the page is shown in; nMax: height in pixels of the template content
    // (only used by a template page, a Material page measures itself through LayoutPage).
    void SetScrollbarInfo(int nPage, int nMax);
    virtual void OnTabEntered() {}      //当标签切换到当前窗口时被调用
    virtual void OnOK();
    void SetScrollEnable(bool enable) { m_scroll_enable = enable; }
    virtual void SetControlMouseWheelEnable(bool enable) {}     //在派生类中重写此函数以设置控件是否允许响应鼠标滚轮
    virtual void OnSettingsApplied() {}   //点击了“应用”按钮，应用完设置之后被调用

    bool IsScrollBarVisible();    //滚动条是否可见

    // Inspection used by the host and the verification.
    int ScrollPosition() const { return m_scroll_pos; }
    int ContentHeight() const { return m_content_height; }
    int ViewportHeight() const { return m_viewport_height; }
    NativeMaterial::ScrollRail& ScrollRailControl() { return m_rail; }

protected:
    void EnableDlgCtrl(UINT id, bool enable);
    void ShowDlgCtrl(UINT id, bool show);

    // ---- Material page ----
    // Places the page controls. area.left is 0, area.Width() is the width the controls may use (the rail is already
    // excluded) and area.top is the top of the content, already shifted up by the scroll position; area.Height() is the
    // visible height. With apply == false nothing is moved and the return value is the height the content needs for
    // that width; with apply == true the controls are positioned and the return value is the same height.
    virtual int LayoutPage(const CRect& area, bool apply) { UNREFERENCED_PARAMETER(area); UNREFERENCED_PARAMETER(apply); return 0; }
    // Call at the end of OnInitDialog after the page replaced the controls it converts itself: converts every remaining
    // static text, push button, check box and group box, then lays the page out.
    void FinishMaterialPage();
    // Lays the page out again (after a text or visibility change that alters the height).
    void RelayoutPage() { UpdatePageLayout(); }
    // Keeps the scroll rail in the appearance preference. A page that overrides this calls the base first.
    virtual void OnMaterialAppearanceChanged() override;
    // Moves the content so that `position` pixels of it lie above the visible area (clamped to the scrollable range).
    void ScrollContentTo(int position);

protected:
    CWnd* m_pParent;
    bool m_scroll_enable{ false };
    int m_scroll_pos{};           // content offset in pixels (0 = top)
    int m_content_height{};
    int m_viewport_height{};

private:
    void EnsureRail();
    void UpdatePageLayout();
    void SyncRail();
    void EnsureFocusVisible();
    int m_layout_width{};         // width LayoutPage was given by the latest layout
    bool m_page_ready{ false };
    NativeMaterial::ScrollRail m_rail;

protected:
    // CWnd的虚方法，声明在afxwin.h，实现在wincore.cpp，在WM_SIZE消息处理时被CWnd调用
    // 内部在一些判断之后使用CWnd持有的CMFCDynamicLayout指针执行m_pDynamicLayout->Adjust()
    virtual void ResizeDynamicLayout() override final;  // 这里重写是为了改变调用时机，从默认的窗口OnSize之前改为父窗口OnSize之后

public:
    DECLARE_MESSAGE_MAP()
    afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg LRESULT OnEnsureFocusVisible(WPARAM, LPARAM);
};
