#include "../stdafx.h"
#include "Navigation.h"
#include "ResponsivenessDiagnostics.h"
#include <cstdio>

namespace {
CWinApp application;
int failures{};
void Check(bool value,const char* name) { std::printf("%s %s\n",value?"PASS":"FAIL",name);if(!value)++failures; }
LONG64 Calls() { auto* data=NativeDiagnostics::Data();return data?data->counters[static_cast<unsigned>(NativeDiagnostics::Operation::Measure)].calls:-1; }
}
int wmain()
{
    if(!AfxWinInit(GetModuleHandle(nullptr),nullptr,GetCommandLine(),0))return 2;
    Check(NativeDiagnostics::Data()!=nullptr,"numeric diagnostics explicitly enabled");
    CWnd parent;
    Check(parent.CreateEx(0,AfxRegisterWndClass(0),L"Result list regression",WS_POPUP,0,0,800,600,nullptr,0)!=FALSE,"hidden native parent created");
    NativeMaterial::ResultList list;
    Check(list.Create(CRect(0,0,400,180),&parent,100)!=FALSE,"actual ResultList HWND created");
    CFont regular, equivalent, large;
    regular.CreateFontW(-16,0,0,0,FW_NORMAL,FALSE,FALSE,0,DEFAULT_CHARSET,0,0,0,0,L"Segoe UI");
    equivalent.CreateFontW(-16,0,0,0,FW_NORMAL,FALSE,FALSE,0,DEFAULT_CHARSET,0,0,0,0,L"Segoe UI");
    large.CreateFontW(-32,0,0,0,FW_NORMAL,FALSE,FALSE,0,DEFAULT_CHARSET,0,0,0,0,L"Segoe UI");
    list.SetFont(&regular);
    const LONG64 start=Calls();
    bool indices=true;for(int i=0;i<120;++i)indices=indices&&(list.AddString(L"A row with several words that can wrap across a narrow list")==i);
    Check(indices,"append returns actual row indices");
    const auto additions=Calls()-start;
    std::printf("MEASURE append120=%lld\n",additions);
    Check(additions>=120&&additions<=240,"append measurement is linear, at most two per row");
    list.SetCurSel(60);list.SetTopIndex(50);
    const int height=list.GetItemHeight(60),top=list.GetTopIndex();
    auto before=Calls();
    list.SendMessage(WM_SIZE,SIZE_RESTORED,MAKELPARAM(400,180));
    list.MoveWindow(0,0,400,200,FALSE);
    Check(Calls()==before,"same wrapping width and height-only layout do not measure rows");
    list.SetFont(&equivalent,FALSE);
    Check(Calls()==before,"equivalent font object does not measure rows");
    Check(list.GetItemHeight(60)==height,"unchanged font retains stored row height");
    Check(list.GetCurSel()==60&&list.GetTopIndex()==top,"unchanged layout retains selection and scroll");
    before=Calls();list.MoveWindow(0,0,180,200,FALSE);
    Check(Calls()-before==120,"changed wrapping width measures every row once");
    Check(list.GetItemHeight(60)>height,"narrow width increases wrapped row height");
    Check(list.GetCurSel()==60&&list.GetTopIndex()==top,"width change retains selection and scroll");
    const int narrow=list.GetItemHeight(60);
    before=Calls();list.SetFont(&large,FALSE);
    Check(Calls()-before==120,"changed font measures every row once");
    Check(list.GetItemHeight(60)>narrow,"larger font increases wrapped row height");
    before=Calls();list.SendMessage(WM_DPICHANGED_AFTERPARENT);
    Check(Calls()==before,"unchanged actual DPI notification avoids redundant measurement");
    const int preservedHeight=list.GetItemHeight(0);
    list.SetItemHeight(0,preservedHeight+1);
    before=Calls();const int inserted=list.InsertString(5,L"First line\nSecond line\nThird line");
    Check(inserted==5&&Calls()-before>=1&&Calls()-before<=2,"insert measures only returned index");
    Check(list.GetItemHeight(0)==preservedHeight+1,"insertion preserves existing row heights");
    std::printf("INSERT selection=%d top=%d\n",list.GetCurSel(),list.GetTopIndex());
    Check(list.GetCurSel()==61&&list.GetTopIndex()==top,"native insertion preserves selection adjustment and scroll index");
    const int shortRow=list.AddString(L"Short");
    Check(list.GetItemHeight(inserted)>list.GetItemHeight(shortRow),"multiline inserted text receives measured height");
    before=Calls();const int count=list.GetCount();
    Check(list.DeleteString(count+10)==LB_ERR,"invalid native deletion returns failure");
    Check(list.GetCount()==count&&Calls()==before,"failed mutation leaves row inventory and measurements intact");
    list.ResetContent();Check(list.GetCount()==0,"reset removes all rows");
    before=Calls();Check(list.AddString(L"New row")==0,"append after reset uses index zero");
    Check(Calls()-before>=1&&Calls()-before<=2,"reset does not retain stale row inventory");
    list.DestroyWindow();parent.DestroyWindow();
    std::printf("RESULT failures=%d\n",failures);
    return failures?1:0;
}
