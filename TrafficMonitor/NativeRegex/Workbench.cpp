#include "../stdafx.h"
#include "Workbench.h"
#include "../NativeMaterial/Controls.h"
#include <array>

namespace NativeRegex
{
namespace
{
    enum : UINT { Pattern=55001, Subject, Replacement, Mode, Case, Run, Replace, Split, Apply, Cancel, Results, InsertAny, InsertDigit, InsertWord, InsertGroup, InsertStart, InsertEnd };
    class Workbench final : public CDialog
    {
    public:
        Options draft; std::vector<std::wstring> subjects; HWND anchor{}; bool chinese{};
        NativeMaterial::Appearance appearance{};
    protected:
        BOOL OnInitDialog() override;
        BOOL OnCommand(WPARAM,LPARAM) override;
        void OnCancel() override { session.Cancel();EndDialog(IDCANCEL); }
        void OnOK() override { Execute(Operation::Search,false); }
        afx_msg void OnTimer(UINT_PTR);
        afx_msg void OnSize(UINT,int,int){Layout();}
        afx_msg void OnVScroll(UINT code,UINT position,CScrollBar*)
        {if(code==SB_LINEUP)scroll-=32;else if(code==SB_LINEDOWN)scroll+=32;else if(code==SB_PAGEUP)scroll-=160;else if(code==SB_PAGEDOWN)scroll+=160;else if(code==SB_THUMBTRACK)scroll=position;Layout();}
        afx_msg BOOL OnMouseWheel(UINT,short delta,CPoint){scroll-=delta/3;Layout();return TRUE;}
        afx_msg void OnPaint(){CPaintDC dc(this);CRect r;GetClientRect(r);dc.FillRect(r,&background);}
        afx_msg BOOL OnEraseBkgnd(CDC*){return TRUE;}
        afx_msg HBRUSH OnCtlColor(CDC* dc,CWnd*,UINT);
        afx_msg void OnGetMinMaxInfo(MINMAXINFO* info);
        afx_msg LRESULT OnDpi(WPARAM,LPARAM);
        DECLARE_MESSAGE_MAP()
    private:
        CString T(const wchar_t* en,const wchar_t* zh) const{return chinese?zh:en;}
        int Px(int n) const{return MulDiv(n,dpi,96);}
        void Layout();void Fonts();void Execute(Operation,bool);void Changed();Options Read();
        UINT dpi{96};int scroll{};bool loading{true},applying{},ready{};
        CFont font;CBrush background;
        CEdit pattern,subject,replacement,output;CListBox results;
        CStatic heading,engine,patternLabel,subjectLabel,replacementLabel,note,help;
        NativeMaterial::Button mode,caseButton,run,replace,split,apply,cancel,any,digit,word,group,start,end;
        Session session;Result last;
    };
    BEGIN_MESSAGE_MAP(Workbench,CDialog)
        ON_WM_TIMER()
        ON_WM_SIZE()
        ON_WM_VSCROLL()
        ON_WM_MOUSEWHEEL()
        ON_WM_PAINT()
        ON_WM_ERASEBKGND()
        ON_WM_CTLCOLOR()
        ON_WM_GETMINMAXINFO()
        ON_MESSAGE(WM_DPICHANGED,&Workbench::OnDpi)
    END_MESSAGE_MAP()
    BOOL Workbench::OnInitDialog()
    {
        CDialog::OnInitDialog();dpi=GetDpiForWindow(m_hWnd);if(!dpi)dpi=96;
        SetWindowText(T(L"Search pattern workbench",L"搜尋表示式工作台"));
        auto colors=NativeMaterial::Palette::Resolve(appearance);
        background.CreateSolidBrush(RGB(BYTE(colors.surface.r*255),BYTE(colors.surface.g*255),BYTE(colors.surface.b*255)));
        DWORD child=WS_CHILD|WS_VISIBLE;CRect r(0,0,100,32);
        heading.Create(T(L"Build a filter for this search",L"建立此搜尋專用篩選"),child,r,this);
        engine.Create(L"TrafficMonitor bounded NFA v1 | UTF-16 | ASCII case folding | leftmost-longest",child,r,this);
        patternLabel.Create(T(L"&Pattern / literal query",L"表示式／文字 (&P)"),child,r,this);
        pattern.CreateEx(WS_EX_CLIENTEDGE,L"EDIT",draft.query.c_str(),child|WS_TABSTOP|ES_AUTOHSCROLL,r,this,Pattern);pattern.SetLimitText(256);
        mode.Create(draft.regex?T(L"Mode: regex",L"模式：正規式"):T(L"Mode: literal",L"模式：文字"),r,this,Mode);
        caseButton.Create(draft.ignoreCase?T(L"ASCII case: ignore",L"忽略 ASCII 大小寫"):T(L"Case: exact",L"區分大小寫"),r,this,Case);
        any.Create(T(L"Any .",L"任意 ."),r,this,InsertAny);digit.Create(L"Digit [0-9]",r,this,InsertDigit);word.Create(L"Word \\w+",r,this,InsertWord);
        group.Create(L"Group (?:...)",r,this,InsertGroup);start.Create(L"Start ^",r,this,InsertStart);end.Create(L"End $",r,this,InsertEnd);
        help.Create(T(L"Supported: literals, . [a-z] [^a] \\d \\w \\s ^ $ | (?:...) ? * +. Escapes: \\n \\r \\t and punctuation. Dot excludes CR/LF. Groups do not capture. No lazy/count quantifiers, lookaround, backreferences, Unicode properties, other flavors or debugger. Full RegexBuddy parity is unfinished.",
            L"支援：文字、. [a-z] [^a] \\d \\w \\s ^ $ | (?:...) ? * +。跳脫：\\n \\r \\t 及標點。點號不包括換行。群組不擷取；暫不支援惰性／次數量詞、環視、反向參照、Unicode 屬性、其他引擎或偵錯。完整 RegexBuddy 功能仍未完成。"),child,r,this);
        subjectLabel.Create(T(L"&Example subject (local preview only)",L"範例文字，只供本機預覽 (&E)"),child,r,this);
        subject.CreateEx(WS_EX_CLIENTEDGE,L"EDIT",subjects.empty()?L"Ethernet 1\r\nWi-Fi 2":subjects.front().c_str(),child|WS_TABSTOP|ES_MULTILINE|ES_AUTOVSCROLL|WS_VSCROLL,r,this,Subject);subject.SetLimitText(8192);
        replacementLabel.Create(T(L"&Replacement (literal text, no group substitution)",L"取代文字，不展開群組參照 (&R)"),child,r,this);
        replacement.CreateEx(WS_EX_CLIENTEDGE,L"EDIT",L"",child|WS_TABSTOP|ES_AUTOHSCROLL,r,this,Replacement);replacement.SetLimitText(4096);
        run.Create(T(L"Search",L"搜尋"),r,this,Run);replace.Create(T(L"Replace preview",L"取代預覽"),r,this,Replace);split.Create(T(L"Split preview",L"分割預覽"),r,this,Split);
        results.Create(child|WS_TABSTOP|WS_VSCROLL|LBS_NOTIFY|LBS_NOINTEGRALHEIGHT,r,this,Results);
        output.CreateEx(WS_EX_CLIENTEDGE,L"EDIT",L"",child|WS_TABSTOP|ES_MULTILINE|ES_READONLY|ES_AUTOVSCROLL|WS_VSCROLL,r,this,0);
        note.Create(L"",child,r,this);
        apply.Create(T(L"Apply to search",L"套用至搜尋"),r,this,Apply);cancel.Create(T(L"Cancel",L"取消"),r,this,Cancel);
        for(auto* b:{&mode,&caseButton,&run,&replace,&split,&apply,&cancel,&any,&digit,&word,&group,&start,&end})b->SetAppearance(appearance);
        Fonts();RECT size{0,0,Px(780),Px(700)};AdjustWindowRectEx(&size,GetStyle(),FALSE,GetExStyle());
        RECT origin{};::GetWindowRect(anchor,&origin);MONITORINFO monitor{sizeof(monitor)};GetMonitorInfoW(MonitorFromWindow(anchor,MONITOR_DEFAULTTONEAREST),&monitor);
        int width=(std::min)(size.right-size.left,monitor.rcWork.right-monitor.rcWork.left),height=(std::min)(size.bottom-size.top,monitor.rcWork.bottom-monitor.rcWork.top);
        SetWindowPos(nullptr,(std::max)(monitor.rcWork.left,(std::min)(origin.left,monitor.rcWork.right-width)),(std::max)(monitor.rcWork.top,(std::min)(origin.bottom,monitor.rcWork.bottom-height)),width,height,SWP_NOZORDER);
        Layout();loading=false;SetTimer(1,40,nullptr);Execute(Operation::Search,false);pattern.SetFocus();return FALSE;
    }
    void Workbench::Fonts()
    {
        HFONT old=static_cast<HFONT>(font.Detach());font.CreateFont(-Px(14),0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,L"Segoe UI");
        for(CWnd* c=GetWindow(GW_CHILD);c;c=c->GetNextWindow())c->SetFont(&font);if(old)DeleteObject(old);
    }
    void Workbench::Layout()
    {
        if(!pattern.GetSafeHwnd())return;CRect r;GetClientRect(r);int w=MulDiv(r.Width(),96,dpi),viewport=MulDiv(r.Height(),96,dpi),h=(std::max)(700,viewport);
        scroll=(std::max)(0,(std::min)(scroll,h-viewport));SCROLLINFO si{sizeof(si),SIF_RANGE|SIF_PAGE|SIF_POS,0,h-1,static_cast<UINT>(viewport),scroll};SetScrollInfo(SB_VERT,&si,TRUE);
        auto put=[&](CWnd& c,int x,int y,int width,int height){c.MoveWindow(Px(x),Px(y-scroll),Px((std::max)(1,width)),Px((std::max)(1,height)));};
        put(heading,20,12,w-40,28);put(engine,20,42,w-40,38);put(patternLabel,20,82,w-40,22);put(pattern,20,106,w-40,34);
        put(mode,20,146,170,40);put(caseButton,202,146,230,40);
        int tokenWidth=(w-65)/6;int x=20;for(auto* b:{&any,&digit,&word,&group,&start,&end}){put(*b,x,192,tokenWidth,40);x+=tokenWidth+5;}
        put(help,20,240,w-40,82);put(subjectLabel,20,328,w-40,22);put(subject,20,352,w-40,70);
        put(replacementLabel,20,428,w-40,22);put(replacement,20,452,w-40,32);
        put(run,20,490,120,40);put(replace,152,490,180,40);put(split,344,490,180,40);
        put(results,20,538,(w-52)/2,h-638);put(output,32+(w-52)/2,538,(w-52)/2,h-638);
        put(note,20,h-92,w-40,40);put(apply,20,h-48,210,40);put(cancel,w-140,h-48,120,40);Invalidate(FALSE);
    }
    Options Workbench::Read(){CString text;pattern.GetWindowText(text);auto value=draft;value.query=text.GetString();return value;}
    void Workbench::Changed(){if(loading)return;session.Cancel();ready=false;applying=false;apply.EnableWindow(FALSE);note.SetWindowText(T(L"Draft changed. Run a preview before applying.",L"草稿已更改。請先執行預覽再套用。"));}
    void Workbench::Execute(Operation operation,bool commit)
    {
        CString sample,repl;subject.GetWindowText(sample);replacement.GetWindowText(repl);
        applying=commit;ready=false;apply.EnableWindow(FALSE);note.SetWindowText(T(L"Evaluating locally; Cancel retains the accepted search.",L"正在本機處理；取消會保留已套用搜尋。"));
        session.Request(Read(),commit?subjects:std::vector<std::wstring>{sample.GetString()},commit?Operation::Filter:operation,repl.GetString());
    }
    void Workbench::OnTimer(UINT_PTR id)
    {
        if(id!=1){CDialog::OnTimer(id);return;}Result result;if(!session.Poll(result))return;
        if(!result.complete){ready=false;applying=false;note.SetWindowText(result.error.c_str());return;}
        if(applying){draft=Read();EndDialog(IDOK);return;}
        last=std::move(result);ready=true;apply.EnableWindow(TRUE);results.ResetContent();
        for(size_t i=0;i<last.matches.size();++i){CString line;line.Format(T(L"Match %zu: offset %zu, length %zu",L"符合 %zu：位置 %zu，長度 %zu"),i+1,last.matches[i].start,last.matches[i].length);results.AddString(line);}
        output.SetWindowText(last.output.c_str());note.SetWindowText(T(L"Preview complete. Select a match to highlight its UTF-16 span. Apply validates all owner rows.",L"預覽完成。選取結果可標示 UTF-16 範圍。套用時會驗證全部來源項目。"));
    }
    BOOL Workbench::OnCommand(WPARAM w,LPARAM l)
    {
        UINT id=LOWORD(w),code=HIWORD(w);
        if((id==Pattern||id==Subject||id==Replacement)&&code==EN_CHANGE){Changed();return TRUE;}
        if(id==Results&&code==LBN_SELCHANGE){int row=results.GetCurSel();if(row>=0&&static_cast<size_t>(row)<last.matches.size()){auto s=last.matches[row];subject.SetSel(static_cast<int>(s.start),static_cast<int>(s.start+s.length));subject.SetFocus();}return TRUE;}
        if(code!=BN_CLICKED)return CDialog::OnCommand(w,l);
        switch(id)
        {
        case Cancel:OnCancel();return TRUE;
        case Apply:if(ready)Execute(Operation::Filter,true);return TRUE;
        case Run:Execute(Operation::Search,false);return TRUE;
        case Replace:Execute(Operation::Replace,false);return TRUE;
        case Split:Execute(Operation::Split,false);return TRUE;
        case Mode:draft.regex=!draft.regex;mode.SetLabel(draft.regex?T(L"Mode: regex",L"模式：正規式"):T(L"Mode: literal",L"模式：文字"));Changed();return TRUE;
        case Case:draft.ignoreCase=!draft.ignoreCase;caseButton.SetLabel(draft.ignoreCase?T(L"ASCII case: ignore",L"忽略 ASCII 大小寫"):T(L"Case: exact",L"區分大小寫"));Changed();return TRUE;
        default:break;
        }
        if(id>=InsertAny&&id<=InsertEnd)
        {
            const wchar_t* tokens[]={L".",L"[0-9]",L"\\w+",L"(?:)",L"^",L"$"};
            draft.regex=true;mode.SetLabel(T(L"Mode: regex",L"模式：正規式"));pattern.ReplaceSel(tokens[id-InsertAny],TRUE);Changed();pattern.SetFocus();return TRUE;
        }
        return CDialog::OnCommand(w,l);
    }
    HBRUSH Workbench::OnCtlColor(CDC* dc,CWnd*,UINT)
    {auto c=NativeMaterial::Palette::Resolve(appearance);dc->SetTextColor(RGB(BYTE(c.onSurface.r*255),BYTE(c.onSurface.g*255),BYTE(c.onSurface.b*255)));dc->SetBkColor(RGB(BYTE(c.surface.r*255),BYTE(c.surface.g*255),BYTE(c.surface.b*255)));return static_cast<HBRUSH>(background.GetSafeHandle());}
    void Workbench::OnGetMinMaxInfo(MINMAXINFO* info){RECT r{0,0,Px(640),Px(400)};AdjustWindowRectEx(&r,GetStyle(),FALSE,GetExStyle());info->ptMinTrackSize={r.right-r.left,r.bottom-r.top};}
    LRESULT Workbench::OnDpi(WPARAM w,LPARAM l){dpi=HIWORD(w);auto r=reinterpret_cast<RECT*>(l);SetWindowPos(nullptr,r->left,r->top,r->right-r->left,r->bottom-r->top,SWP_NOZORDER|SWP_NOACTIVATE);Fonts();Layout();return 0;}
}
bool Edit(HWND owner,HWND anchor,Options& accepted,const std::vector<std::wstring>& subjects,bool chinese,NativeMaterial::Appearance appearance)
{
    struct Template {DLGTEMPLATE dialog;WORD menu{},className{},title{};} layout{};
    layout.dialog.style=WS_POPUP|WS_CAPTION|WS_SYSMENU|WS_THICKFRAME|WS_CLIPCHILDREN|WS_VSCROLL|DS_MODALFRAME;layout.dialog.dwExtendedStyle=WS_EX_CONTROLPARENT;layout.dialog.cx=500;layout.dialog.cy=450;
    Workbench editor;editor.draft=accepted;editor.subjects=subjects;editor.anchor=anchor;editor.chinese=chinese;editor.appearance=appearance;
    if(!editor.InitModalIndirect(&layout.dialog,CWnd::FromHandle(owner)))return false;
    bool applied=editor.DoModal()==IDOK;if(applied)accepted=editor.draft;if(IsWindow(anchor))::SetFocus(anchor);return applied;
}
}
