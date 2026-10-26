// CSkinPreviewView.cpp: 实现文件
//

#include "stdafx.h"
#include "TrafficMonitor.h"
#include "CSkinPreviewView.h"
#include "NativePreferences/Text.h"


// CSkinPreviewView

namespace
{
    constexpr UINT VerticalRailId = 3001;
    constexpr UINT HorizontalRailId = 3002;
}

CSkinPreviewView::CSkinPreviewView()
{

}

CSkinPreviewView::~CSkinPreviewView()
{
}


BEGIN_MESSAGE_MAP(CSkinPreviewView, CWnd)
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
	ON_WM_SIZE()
	ON_WM_MOUSEWHEEL()
END_MESSAGE_MAP()


BOOL CSkinPreviewView::Create(CWnd* parent, const CRect& rect, UINT id)
{
	const CString class_name = AfxRegisterWndClass(CS_HREDRAW | CS_VREDRAW, ::LoadCursor(nullptr, IDC_ARROW));
	// No WS_VSCROLL / WS_HSCROLL: the Material scroll rails replace the system scroll bars.
	if (!CWnd::Create(class_name, L"", WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN, rect, parent, id))
		return FALSE;
	if (!m_vertical_rail.Create(this, VerticalRailId) || !m_horizontal_rail.Create(this, HorizontalRailId))
		return FALSE;
	m_horizontal_rail.SetHorizontal(true);
	m_vertical_rail.SetWindowText(NativePreferences::InlinePair(L"Preview vertical scroll bar", L"預覽垂直捲軸"));
	m_horizontal_rail.SetWindowText(NativePreferences::InlinePair(L"Preview horizontal scroll bar", L"預覽水平捲軸"));
	m_vertical_rail.changed = [this](int position) { m_offset.y = position; Invalidate(FALSE); };
	m_horizontal_rail.changed = [this](int position) { m_offset.x = position; Invalidate(FALSE); };
	m_vertical_rail.SetAppearance(m_appearance);
	m_horizontal_rail.SetAppearance(m_appearance);
	UpdateScroll();
	return TRUE;
}

void CSkinPreviewView::SetAppearance(NativeMaterial::Appearance appearance, COLORREF surface)
{
	m_appearance = appearance;
	m_surface = surface;
	if (m_vertical_rail.GetSafeHwnd() != nullptr)
		m_vertical_rail.SetAppearance(appearance);
	if (m_horizontal_rail.GetSafeHwnd() != nullptr)
		m_horizontal_rail.SetAppearance(appearance);
	if (m_hWnd != nullptr)
		Invalidate(FALSE);
}

void CSkinPreviewView::UpdateScroll()
{
	if (m_hWnd == nullptr || m_vertical_rail.GetSafeHwnd() == nullptr || m_horizontal_rail.GetSafeHwnd() == nullptr)
		return;
	CRect client;
	GetClientRect(client);
	const int rail = MulDiv(12, GetDpiForWindow(m_hWnd), 96);
	const int client_width = client.Width(), client_height = client.Height();

	// A rail takes room from the other direction, so the need for each is settled together.
	bool need_vertical = false, need_horizontal = false;
	for (int pass = 0; pass < 2; ++pass)
	{
		need_vertical = m_size.cy > client_height - (need_horizontal ? rail : 0);
		need_horizontal = m_size.cx > client_width - (need_vertical ? rail : 0);
	}
	m_view.cx = (std::max)(0, client_width - (need_vertical ? rail : 0));
	m_view.cy = (std::max)(0, client_height - (need_horizontal ? rail : 0));

	const int max_x = (std::max)(0, static_cast<int>(m_size.cx - m_view.cx));
	const int max_y = (std::max)(0, static_cast<int>(m_size.cy - m_view.cy));
	m_offset.x = (std::max)(0, (std::min)(static_cast<int>(m_offset.x), max_x));
	m_offset.y = (std::max)(0, (std::min)(static_cast<int>(m_offset.y), max_y));

	// The rails hide themselves when the content fits the page.
	m_vertical_rail.MoveWindow(client_width - rail, 0, rail, m_view.cy);
	m_vertical_rail.SetStep(MulDiv(24, GetDpiForWindow(m_hWnd), 96));
	m_vertical_rail.SetMetrics(m_size.cy, m_view.cy, m_offset.y);
	m_horizontal_rail.MoveWindow(0, client_height - rail, m_view.cx, rail);
	m_horizontal_rail.SetStep(MulDiv(24, GetDpiForWindow(m_hWnd), 96));
	m_horizontal_rail.SetMetrics(m_size.cx, m_view.cx, m_offset.x);
	Invalidate(FALSE);
}

void CSkinPreviewView::ScrollTo(CPoint offset)
{
	m_offset = offset;
	UpdateScroll();
}

void CSkinPreviewView::DrawPreview(CDC* pDC)
{
    CDrawCommon drawer;
    drawer.Create(pDC, nullptr);
    CRect view_rect(0, 0, (std::max)(static_cast<int>(m_view.cx), static_cast<int>(m_size.cx)), (std::max)(static_cast<int>(m_view.cy), static_cast<int>(m_size.cy)));

    if (m_skin_data == nullptr)
    {
        drawer.FillRect(view_rect, m_surface);
        return;
    }

    //如果皮肤是png格式，绘制10x10的棋盘背景
    if (m_skin_data->IsPNG())
    {
        int grid_size = theApp.DPI(10);

        // 检查画布大小
        int rows = view_rect.Height() / grid_size + 1;  // 行数
        int cols = view_rect.Width() / grid_size + 1;  // 列数

        // 遍历每个网格
        for (int row = 0; row < rows; ++row)
        {
            for (int col = 0; col < cols; ++col)
            {
                // 计算当前网格的矩形区域
                CRect rect(col * grid_size, row * grid_size,
                    (col + 1) * grid_size, (row + 1) * grid_size);

                // 判断当前网格颜色（交替填充）
                COLORREF color = ((row + col) % 2 == 0) ? RGB(204, 204, 204) : RGB(254, 254, 254);

                // 填充矩形
                drawer.FillRect(rect, color);
            }
        }
    }
    //绘制纯色背景
    else
    {
        drawer.FillRect(view_rect, m_surface);
    }

    //绘制预览图
    CRect draw_rect(CPoint(0, 0), m_size);
    m_skin_data->DrawPreview(pDC, draw_rect);
}

void CSkinPreviewView::OnPaint()
{
	CPaintDC dc(this);
	CRect client;
	GetClientRect(client);
	if (m_view.cx <= 0 || m_view.cy <= 0)
	{
		dc.FillSolidRect(client, m_surface);
		return;
	}

	// The picture is drawn offscreen at the scroll offset and copied over in one step.
	CDC memory;
	memory.CreateCompatibleDC(&dc);
	CBitmap bitmap;
	bitmap.CreateCompatibleBitmap(&dc, m_view.cx, m_view.cy);
	CBitmap* previous_bitmap = memory.SelectObject(&bitmap);
	memory.SetViewportOrg(-m_offset.x, -m_offset.y);
	DrawPreview(&memory);
	memory.SetViewportOrg(0, 0);
	dc.BitBlt(0, 0, m_view.cx, m_view.cy, &memory, 0, 0, SRCCOPY);
	memory.SelectObject(previous_bitmap);

	// The corner between the two rails and any strip the rails leave uncovered.
	CRect right(m_view.cx, 0, client.right, client.bottom), bottom(0, m_view.cy, m_view.cx, client.bottom);
	if (!right.IsRectEmpty())
		dc.FillSolidRect(right, m_surface);
	if (!bottom.IsRectEmpty())
		dc.FillSolidRect(bottom, m_surface);
}

BOOL CSkinPreviewView::OnEraseBkgnd(CDC* pDC)
{
	return TRUE;
}

void CSkinPreviewView::OnSize(UINT nType, int cx, int cy)
{
	CWnd::OnSize(nType, cx, cy);
	if (nType != SIZE_MINIMIZED)
		UpdateScroll();
}

BOOL CSkinPreviewView::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	const int step = MulDiv(72, GetDpiForWindow(m_hWnd), 96) * (zDelta > 0 ? -1 : 1);
	CPoint offset = m_offset;
	if (nFlags & MK_SHIFT)
		offset.x += step;
	else
		offset.y += step;
	if (offset != m_offset)
		ScrollTo(offset);
	return TRUE;
}

void CSkinPreviewView::SetSize(int width, int hight)
{
	m_size = CSize(width, hight);
	UpdateScroll();
}

// CSkinPreviewView 消息处理程序
