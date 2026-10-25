#include "../stdafx.h"
#include "Controls.h"
#include <cstdio>
namespace { CWinApp application;int failures{};
void Check(bool value,const char* name){std::printf("%s %s\n",value?"PASS":"FAIL",name);if(!value)++failures;}
}
int wmain()
{
    if(!AfxWinInit(GetModuleHandle(nullptr),nullptr,GetCommandLine(),0))return 2;
    CWnd parent;Check(parent.CreateEx(WS_EX_CONTROLPARENT,AfxRegisterWndClass(0),L"Preview keyboard regression",WS_POPUP,0,0,600,400,nullptr,0)!=FALSE,"hidden native parent created");
    CButton before,after;NativeMaterial::ReadOnlyText preview;CEdit native;
    const CRect box(0,0,200,80);
    before.Create(L"Before",WS_CHILD|WS_VISIBLE|WS_TABSTOP,box,&parent,100);
    Check(preview.Create(box,&parent,56210)!=FALSE,"actual preview EDIT HWND created");
    after.Create(L"After",WS_CHILD|WS_VISIBLE|WS_TABSTOP,box,&parent,101);
    native.Create(WS_CHILD|WS_VISIBLE|WS_TABSTOP|ES_MULTILINE|ES_READONLY|ES_AUTOVSCROLL|WS_VSCROLL,box,&parent,102);
    preview.SetWindowText(L"First line\r\nSecond line");native.SetWindowText(L"First line\r\nSecond line");
    preview.SetSel(2,8);native.SetSel(2,8);
    MSG tab{};tab.hwnd=preview.GetSafeHwnd();tab.message=WM_KEYDOWN;tab.wParam=VK_TAB;
    const auto rawTab=native.SendMessage(WM_GETDLGCODE,VK_TAB,reinterpret_cast<LPARAM>(&tab));
    const auto actualTab=preview.SendMessage(WM_GETDLGCODE,VK_TAB,reinterpret_cast<LPARAM>(&tab));
    std::printf("DIALOGCODE native=%lld preview=%lld\n",rawTab,actualTab);
    Check((actualTab&(DLGC_WANTALLKEYS|DLGC_WANTTAB))==0,"Tab with valid MSG releases dialog navigation");
    Check(actualTab==(rawTab&~(DLGC_WANTALLKEYS|DLGC_WANTTAB)),"Tab preserves every unrelated native dialog flag");
    Check((preview.SendMessage(WM_GETDLGCODE,VK_TAB,0)&(DLGC_WANTALLKEYS|DLGC_WANTTAB))==0,"Tab query without MSG releases dialog navigation");
    Check((preview.SendMessage(WM_GETDLGCODE,0,reinterpret_cast<LPARAM>(&tab))&(DLGC_WANTALLKEYS|DLGC_WANTTAB))==0,"MSG-only Tab query releases dialog navigation");
    MSG ordinary=tab;ordinary.wParam='A';
    Check(preview.SendMessage(WM_GETDLGCODE,'A',reinterpret_cast<LPARAM>(&ordinary))==native.SendMessage(WM_GETDLGCODE,'A',reinterpret_cast<LPARAM>(&ordinary)),"ordinary key preserves native dialog flags");
    Check(preview.SendMessage(WM_GETDLGCODE,0,0)==native.SendMessage(WM_GETDLGCODE,0,0),"general dialog query preserves native flags");
    Check(::GetNextDlgTabItem(parent.GetSafeHwnd(),preview.GetSafeHwnd(),FALSE)==after.GetSafeHwnd(),"forward tab destination remains adjacent action");
    Check(::GetNextDlgTabItem(parent.GetSafeHwnd(),preview.GetSafeHwnd(),TRUE)==before.GetSafeHwnd(),"reverse tab destination remains preceding action");
    int first=0,last=0;preview.GetSel(first,last);Check(first==2&&last==8,"dialog queries retain native text selection");
    CString text;preview.GetWindowText(text);Check(text==L"First line\r\nSecond line","dialog queries retain preview content");
    Check((preview.GetStyle()&ES_READONLY)!=0,"preview remains read only");
    parent.DestroyWindow();std::printf("RESULT failures=%d\n",failures);return failures?1:0;
}
