#include "stdafx.h"
#include "TabDlg.h"
#include "TrafficMonitor.h"
#include "NativePreferences/Text.h"

namespace
{
    constexpr UINT RailControlId = 53990;
    constexpr UINT EnsureFocusVisibleMessage = WM_APP + 0x610;
}

IMPLEMENT_DYNAMIC(CTabDlg, CBaseDialog)

CTabDlg::CTabDlg(UINT nIDTemplate, CWnd * pParent) : CBaseDialog(nIDTemplate, pParent)
{
    m_pParent = pParent;
}


CTabDlg::~CTabDlg()
{
}


BOOL CTabDlg::PreTranslateMessage(MSG* pMsg)
{
    // TODO: 在此添加专用代码和/或调用基类
    //由于这是tab标签中的子对话框，因此用户按回车或ESC后不应该响应当前对话框的IDOK或IDCANCEL，
    //而应该响应tab标签的父窗口的IDOK或IDCANCEL命令
    if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_ESCAPE)
    {
        if (m_pParent != nullptr)
            m_pParent->SendMessage(WM_COMMAND, IDCANCEL);
        return TRUE;
    }
    if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_RETURN)
    {
        if (m_pParent != nullptr)
            m_pParent->SendMessage(WM_COMMAND, IDOK);
        return TRUE;
    }

    const BOOL handled = CBaseDialog::PreTranslateMessage(pMsg);
    // Keyboard navigation can land on a control that the scrolled content hides: bring it into view afterwards.
    if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_TAB && m_scroll_enable)
        PostMessage(EnsureFocusVisibleMessage);
    return handled;
}


BOOL CTabDlg::OnInitDialog()
{
    CBaseDialog::OnInitDialog();

    // TODO:  在此添加额外的初始化
    // A Material page already has the surface color of the appearance preference; a template page keeps the
    // fixed light surface it always had.
    if (!IsMaterialPage())
    {
        if (theApp.m_win_version.IsWindows11OrLater())
            SetBackgroundColor(RGB(249, 249, 249));
        else
            SetBackgroundColor(RGB(255, 255, 255));
    }

    return TRUE;  // return TRUE unless you set the focus to a control
                  // 异常: OCX 属性页应返回 FALSE
}

CWnd* CTabDlg::GetParentWindow()
{
    CWnd* pParent = GetParent();
    if (pParent != nullptr)
    {
        return pParent->GetParent();
    }
    return nullptr;
}

void CTabDlg::EnsureRail()
{
    if (m_rail.GetSafeHwnd() != nullptr)
        return;
    if (!m_rail.Create(this, RailControlId))
        return;
    m_rail.SetAppearance(MaterialAppearance());
    m_rail.SetStep(DPI(48));
    m_rail.SetWindowText(NativePreferences::InlinePair(L"Scroll page", L"捲動頁面"));
    // Last in z-order, so the rail is the last tab stop of the page and never overlaps the content (the content is
    // laid out beside it).
    m_rail.SetWindowPos(&CWnd::wndBottom, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    m_rail.changed = [this](int position)
    {
        ScrollContentTo(position);
    };
}

void CTabDlg::SyncRail()
{
    if (m_rail.GetSafeHwnd() == nullptr)
        return;
    CRect client;
    GetClientRect(client);
    const int rail_width = DPI(12);
    m_rail.MoveWindow(client.right - rail_width, 0, rail_width, client.Height());
    m_rail.SetMetrics(m_content_height, m_viewport_height, m_scroll_pos);
    // A template page has no room reserved for the rail, so the rail lies over its right edge.
    m_rail.SetWindowPos(IsMaterialPage() ? &CWnd::wndBottom : &CWnd::wndTop, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
}

void CTabDlg::UpdatePageLayout()
{
    if (!IsMaterialPage() || !m_page_ready || GetSafeHwnd() == nullptr)
        return;
    CRect client;
    GetClientRect(client);
    if (client.Width() <= 0 || client.Height() <= 0)
        return;
    EnsureRail();

    const int rail_width = DPI(12);
    const int viewport = client.Height();
    int width = client.Width();
    int content = LayoutPage(CRect(0, 0, width, viewport), false);
    if (content > viewport)
    {
        // The rail takes its width from the content; measure again for the narrower column.
        width = (std::max)(1, width - rail_width);
        content = LayoutPage(CRect(0, 0, width, viewport), false);
    }
    m_viewport_height = viewport;
    m_content_height = content;
    m_layout_width = width;
    m_scroll_pos = (std::max)(0, (std::min)(m_scroll_pos, (std::max)(0, content - viewport)));
    m_scroll_enable = true;
    LayoutPage(CRect(0, -m_scroll_pos, width, -m_scroll_pos + viewport), true);
    SyncRail();
    SetControlMouseWheelEnable(!IsScrollBarVisible());
}

void CTabDlg::FinishMaterialPage()
{
    if (!IsMaterialPage())
        return;
    ConvertRemainingStockControls();
    const CString remaining = RemainingStockControls();
    if (!remaining.IsEmpty())
        TRACE(L"Material page: stock controls remain: %s\n", remaining.GetString());
    m_page_ready = true;
    UpdatePageLayout();
}

void CTabDlg::OnMaterialAppearanceChanged()
{
    if (m_rail.GetSafeHwnd() != nullptr)
        m_rail.SetAppearance(MaterialAppearance());
}

void CTabDlg::SetScrollbarInfo(int nPage, int nMax)
{
    if (IsMaterialPage())
    {
        // The page measures itself; only the host's area matters, and that is the client area of the page.
        m_scroll_enable = true;
        UpdatePageLayout();
        return;
    }

    // Template page: the controls keep their template positions and are scrolled by moving the child windows.
    EnsureRail();
    m_scroll_enable = true;
    const int reset_pos = m_scroll_pos;     // how far the controls are currently displaced
    if (nMax < 0) nMax = 0;
    if (nPage < 0) nPage = 0;
    // 父窗口OnSize后的新滚动状态边界检查
    m_content_height = nMax;
    m_viewport_height = nPage;
    m_scroll_pos = (std::max)(0, (std::min)(m_scroll_pos, (std::max)(0, nMax - nPage)));
    // 复位滚动
    if (reset_pos != 0)
        ScrollWindow(0, reset_pos);
    // 代替框架做出OnSize的响应
    CBaseDialog::ResizeDynamicLayout(); // 这个方法会无视滚动更新控件位置导致控件错位，在滚动为0时调用避开这个问题
    // 执行滚动
    if (m_scroll_pos != 0)
        ScrollWindow(0, -m_scroll_pos);
    SyncRail();
}

void CTabDlg::ScrollContentTo(int position)
{
    const int most = (std::max)(0, m_content_height - m_viewport_height);
    position = (std::max)(0, (std::min)(position, most));
    if (position == m_scroll_pos)
        return;
    if (IsMaterialPage())
    {
        m_scroll_pos = position;
        LayoutPage(CRect(0, -m_scroll_pos, m_layout_width, -m_scroll_pos + m_viewport_height), true);
    }
    else
    {
        ScrollWindow(0, m_scroll_pos - position);
        m_scroll_pos = position;
    }
    if (m_rail.GetSafeHwnd() != nullptr)
        m_rail.SetMetrics(m_content_height, m_viewport_height, m_scroll_pos);
}

void CTabDlg::EnsureFocusVisible()
{
    HWND focus = ::GetFocus();
    if (focus == nullptr || focus == m_rail.GetSafeHwnd() || !::IsChild(m_hWnd, focus))
        return;
    // The direct child of the page that holds the focused window.
    HWND child = focus;
    while (child != nullptr && ::GetParent(child) != m_hWnd)
        child = ::GetParent(child);
    if (child == nullptr)
        return;
    CRect rect;
    ::GetWindowRect(child, rect);
    ScreenToClient(rect);
    const int margin = DPI(8);
    if (rect.top < 0)
        ScrollContentTo(m_scroll_pos + rect.top - margin);
    else if (rect.bottom > m_viewport_height)
        ScrollContentTo(m_scroll_pos + rect.bottom - m_viewport_height + margin);
}

LRESULT CTabDlg::OnEnsureFocusVisible(WPARAM, LPARAM)
{
    EnsureFocusVisible();
    return 0;
}

void CTabDlg::EnableDlgCtrl(UINT id, bool enable)
{
    CWnd* pCtrl = GetDlgItem(id);
    if (pCtrl != nullptr)
        pCtrl->EnableWindow(enable);
}

void CTabDlg::ShowDlgCtrl(UINT id, bool show)
{
    CWnd* pCtrl = GetDlgItem(id);
    if (pCtrl != nullptr)
        pCtrl->ShowWindow(show);
}

BEGIN_MESSAGE_MAP(CTabDlg, CBaseDialog)
    ON_WM_MOUSEWHEEL()
    ON_WM_SIZE()
    ON_MESSAGE(EnsureFocusVisibleMessage, &CTabDlg::OnEnsureFocusVisible)
END_MESSAGE_MAP()


void CTabDlg::ResizeDynamicLayout()
{
    // m_pDynamicLayout->Adjust()会无视窗口滚动更新控件位置
    // 之前在父窗口OnSize后重置不可靠（进入退出最大化和贴靠时没能正常工作），也没找到检出这两种特殊OnSize的方法
    // 框架内部调用ResizeDynamicLayout太早，来不及先复位滚动，故覆写此虚方法介入

    // 如果已启用滚动那么此处不调用基类使框架的处理无效，改为在父窗口的OnSize后自行调用
    if (!m_scroll_enable)
        CBaseDialog::ResizeDynamicLayout();
}


BOOL CTabDlg::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
    // TODO: 在此添加消息处理程序代码和/或调用默认值
    if (m_scroll_enable && m_content_height > m_viewport_height)
    {
        ScrollContentTo(m_scroll_pos - DPI(64) * zDelta / 120);
        return TRUE;
    }

    return CBaseDialog::OnMouseWheel(nFlags, zDelta, pt);
}


void CTabDlg::OnOK()
{
    // TODO: 在此添加专用代码和/或调用基类

    CBaseDialog::OnOK();
}


bool CTabDlg::IsScrollBarVisible()
{
    return m_content_height > m_viewport_height;
}

void CTabDlg::OnSize(UINT nType, int cx, int cy)
{
    CBaseDialog::OnSize(nType, cx, cy);

    // TODO: 在此处添加消息处理程序代码
    if (IsMaterialPage() && nType != SIZE_MINIMIZED)
        UpdatePageLayout();
    SetControlMouseWheelEnable(!IsScrollBarVisible());      //如果显示了滚动条，则禁止控件响应鼠标滚轮，此时鼠标滚轮用于滚动窗口
}
