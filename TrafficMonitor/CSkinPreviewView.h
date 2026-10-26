#pragma once
#include "DrawCommon.h"
#include "SkinFile.h"
#include "NativeMaterial/ScrollRail.h"
#include "NativeMaterial/Theme.h"


// CSkinPreviewView 视图
//
// The preview area of the skin dialog. A plain child window (no system scroll bars): the skin preview is
// painted into an offscreen bitmap at the current scroll offset, and a picture larger than the area is
// scrolled with two Material scroll rails (vertical on the right, horizontal at the bottom), the mouse
// wheel (Shift for horizontal) and the keyboard on the focused rail.

class CSkinPreviewView : public CWnd
{
public:
	CSkinPreviewView();
	virtual ~CSkinPreviewView();

	BOOL Create(CWnd* parent, const CRect& rect, UINT id);

//成员函数
public:
	void SetSize(int width, int hight);
	void SetSkinData(CSkinFile* skin_data) { m_skin_data = skin_data; }
	void SetAppearance(NativeMaterial::Appearance appearance, COLORREF surface);

	// Scrolling state, in physical pixels (also read by the verification).
	CSize ContentSize() const { return m_size; }
	CPoint ScrollOffset() const { return m_offset; }
	CSize ViewSize() const { return m_view; }
	void ScrollTo(CPoint offset);
	NativeMaterial::ScrollRail& VerticalRail() { return m_vertical_rail; }
	NativeMaterial::ScrollRail& HorizontalRail() { return m_horizontal_rail; }

	//成员变量
protected:
	CSize m_size;					//预览图的大小
	CPoint m_offset;				//滚动位置
	CSize m_view;					//the visible part of the client area (without the rails)

    CSkinFile* m_skin_data{};

	NativeMaterial::ScrollRail m_vertical_rail;
	NativeMaterial::ScrollRail m_horizontal_rail;
	NativeMaterial::Appearance m_appearance{ NativeMaterial::Appearance::Light };
	COLORREF m_surface{ RGB(255, 255, 255) };

protected:
	void UpdateScroll();
	void DrawPreview(CDC* pDC);     // 绘制预览图（逻辑坐标，原点为预览图左上角）

	afx_msg void OnPaint();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	DECLARE_MESSAGE_MAP()
};
