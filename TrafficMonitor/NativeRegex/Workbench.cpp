#include "../stdafx.h"
#include "Workbench.h"
#include "../NativePreferences/Text.h"
#include "../NativePreferences/Subscription.h"
#include <map>
#include <afxdlgs.h>
#include "Collections.h"
#include "../TrafficMonitor.h"
#include "../NativeMaterial/Controls.h"
#include "../NativeMaterial/TextField.h"
#include "../NativeMaterial/ScrollableTextField.h"
#include "../NativeMaterial/SearchSelector.h"
#include "../NativeMaterial/ScrollViewport.h"
#include "../NativeMaterial/Navigation.h"
#include "../NativeMaterial/Panel.h"
#include <array>
#include <imm.h>

namespace NativeRegex
{
struct EditorState { HWND window{}; };
namespace
{
    enum : UINT { Pattern=55001, Subject, Replacement, Mode, Case, Run, Replace, Split, Apply, Cancel, Results, InsertAny, InsertDigit, InsertWord, InsertGroup, InsertStart, InsertEnd, SavedList, SavedName, SaveNew, LoadSaved, RenameSaved, DeleteSaved, RecoverSaved, RefreshSaved, ExportSaved, ImportSaved, TestPage, ReplacePage, ExplainPage, LibraryPage, OptionsToggle, Output };
    class Workbench final : public CDialog
    {
    public:
        Options draft; std::vector<std::wstring> subjects; HWND anchor{},owner{}; bool chinese{};
        std::shared_ptr<EditorState> state;bool autoDelete{};
        std::function<std::vector<std::wstring>()> readSubjects;
        std::function<void(const Options&)> accept;
        bool Translate(MSG* message) { return PreTranslateMessage(message)!=FALSE; }
        void Reanchor();
        NativeMaterial::Appearance appearance{};
    protected:
        BOOL PreTranslateMessage(MSG* msg) override
        {
            if(msg->message==WM_KEYDOWN&&msg->wParam==VK_ESCAPE)
            {HIMC context=ImmGetContext(msg->hwnd);const bool composing=context&&ImmGetCompositionStringW(context,GCS_COMPSTR,nullptr,0)>0;if(context)ImmReleaseContext(msg->hwnd,context);if(composing)return FALSE;}
            if(msg->message==WM_KEYDOWN&&msg->wParam==VK_TAB)
            {
                HWND next=::GetNextDlgTabItem(m_hWnd,::GetFocus(),(GetKeyState(VK_SHIFT)&0x8000)!=0);
                if(next){::SetFocus(next);RevealFocus();}return TRUE;
            }
            return CDialog::PreTranslateMessage(msg);
        }
        BOOL OnInitDialog() override;
        BOOL OnCommand(WPARAM,LPARAM) override;
        void OnCancel() override { Finish(false); }
        void PostNcDestroy() override { if(state && state->window==m_hWnd)state->window=nullptr;if(autoDelete)delete this; }
        afx_msg void OnDestroy() { preferences.Detach();session.Cancel();collectionValidation.Cancel();KillTimer(1);if(state)state->window=nullptr;CDialog::OnDestroy(); }
        afx_msg void OnClose() { Finish(false); }
        void OnOK() override { Execute(operation,false); }
        afx_msg void OnTimer(UINT_PTR);
        afx_msg void OnSize(UINT,int,int){Layout();RevealFocus();}
        afx_msg void OnVScroll(UINT code,UINT position,CScrollBar*)
        {if(code==SB_LINEUP)scroll-=32;else if(code==SB_LINEDOWN)scroll+=32;else if(code==SB_PAGEUP)scroll-=bodyHeight;else if(code==SB_PAGEDOWN)scroll+=bodyHeight;else if(code==SB_THUMBTRACK||code==SB_THUMBPOSITION)scroll=position;Layout();}
        afx_msg BOOL OnMouseWheel(UINT,short delta,CPoint){scroll-=delta/3;Layout();return TRUE;}
        afx_msg void OnPaint(){CPaintDC dc(this);CRect r;GetClientRect(r);dc.FillRect(r,&background);}
        afx_msg BOOL OnEraseBkgnd(CDC*){return TRUE;}
        afx_msg HBRUSH OnCtlColor(CDC* dc,CWnd*,UINT);
        afx_msg void OnGetMinMaxInfo(MINMAXINFO* info);
        afx_msg LRESULT OnDpi(WPARAM,LPARAM);
        DECLARE_MESSAGE_MAP()
    private:
        CString T(const wchar_t* en,const wchar_t* zh) const{return NativePreferences::Pair(en,zh);}
        NativePreferences::Subscription preferences;
        std::map<HWND,std::pair<std::wstring,std::wstring>> messages;
        void Note(CStatic& control,const wchar_t* en,const wchar_t* zh){messages[control.GetSafeHwnd()]={en,zh};control.SetWindowText(T(en,zh));if(!loading)Layout();}
        void Labels();LRESULT OnPreferences(WPARAM,LPARAM);
        LRESULT WindowProc(UINT message,WPARAM w,LPARAM l) override {if(message==WM_SETTINGCHANGE||message==WM_THEMECHANGED)NativePreferences::RefreshSystemAppearance();return CDialog::WindowProc(message,w,l);}

        int Px(int n) const{return MulDiv(n,dpi,96);}
        void Layout();void Fonts();void Execute(Operation,bool);void Changed();Options Read();
        void Finish(bool commit);
        void CollectionStart(CollectionAction,const SavedExpression& = {},const std::wstring& = L"");
        void CollectionTransfer(bool import);void CollectionPoll();void CollectionValidate(bool save);void CollectionControls(bool enabled);
        SavedExpression pendingRecord;bool validatingCollection{},savingCollection{},confirmDelete{},confirmRecovery{};
        Operation operation{Operation::Search};
        std::vector<SavedExpression> savedRecords;std::future<CollectionResult> collectionWorker;Session collectionValidation;
        NativeMaterial::SearchSelector savedList;NativeMaterial::TextField savedName;CStatic savedLabel,nameLabel,collectionNote;
        NativeMaterial::Button saveNew,loadSaved,renameSaved,deleteSaved,recoverSaved,refreshSaved,exportSaved,importSaved;
        RECT previousAnchor{};
        UINT dpi{96};int scroll{};bool loading{true},applying{},ready{};
        CFont font,headingFont;CBrush background;
        NativeMaterial::TextField pattern,replacement;
        NativeMaterial::ScrollableTextField subject,output;
        NativeMaterial::ResultList results;
        NativeMaterial::ScrollViewport viewport;
        NativeMaterial::Panel panel;
        std::array<NativeMaterial::Button,4> pages;
        NativeMaterial::Button options;
        CStatic resultsLabel,outputLabel;
        int page{},contentHeight{},bodyHeight{};
        std::array<int,4> offsets{};
        bool optionsVisible{};
        void Theme();void RevealFocus();
        CStatic heading,headingSecondary,engine,patternLabel,subjectLabel,replacementLabel,note,help;
        NativeMaterial::Button mode,caseButton,run,replace,split,apply,cancel,any,digit,word,group,start,end;
        Session session;Result last;
    };
    BEGIN_MESSAGE_MAP(Workbench,CDialog)
        ON_WM_TIMER()
        ON_WM_DESTROY()
        ON_WM_CLOSE()
        ON_WM_SIZE()
        ON_WM_VSCROLL()
        ON_WM_MOUSEWHEEL()
        ON_WM_PAINT()
        ON_WM_ERASEBKGND()
        ON_WM_CTLCOLOR()
        ON_WM_GETMINMAXINFO()
        ON_MESSAGE(WM_DPICHANGED,&Workbench::OnDpi)
        ON_MESSAGE(NativePreferences::ChangedMessage,&Workbench::OnPreferences)
    END_MESSAGE_MAP()
    BOOL Workbench::OnInitDialog()
    {
        CDialog::OnInitDialog();state->window=m_hWnd;dpi=GetDpiForWindow(m_hWnd);if(!dpi)dpi=96;
        preferences.Attach(m_hWnd);appearance=NativePreferences::EffectiveAppearance();
        SetWindowText(T(L"Search pattern workbench",L"搜尋表示式工作台"));
        auto colors=NativeMaterial::Palette::Resolve(appearance);
        background.CreateSolidBrush(RGB(BYTE(colors.surface.r*255),BYTE(colors.surface.g*255),BYTE(colors.surface.b*255)));
        DWORD child=WS_CHILD|WS_VISIBLE;CRect r(0,0,100,32);
        collectionNote.Create(L"",child,r,this);
        savedLabel.Create(T(L"Saved expressions (local only)",L"已儲存表示式，只限本機"),child,r,this);
        savedList.Create(r,this,SavedList);
        loadSaved.Create(T(L"&Load draft",L"載入草稿 (&L)"),r,this,LoadSaved);
        refreshSaved.Create(T(L"Refresh",L"重新整理"),r,this,RefreshSaved);
        nameLabel.Create(T(L"Collection &name",L"儲存名稱 (&N)"),child,r,this);
        savedName.CreateEx(0,L"EDIT",L"",child|WS_TABSTOP|ES_AUTOHSCROLL,r,this,SavedName);savedName.SetLimitText(96);
        saveNew.Create(T(L"Save new",L"儲存新項目"),r,this,SaveNew);
        renameSaved.Create(T(L"Rename",L"重新命名"),r,this,RenameSaved);
        deleteSaved.Create(T(L"Delete",L"刪除"),r,this,DeleteSaved);
        recoverSaved.Create(T(L"Recover backup",L"還原備份"),r,this,RecoverSaved);
        exportSaved.Create(T(L"Export collection...",L"匯出收藏..."),r,this,ExportSaved);
        importSaved.Create(T(L"Import collection...",L"匯入收藏..."),r,this,ImportSaved);
        heading.Create(T(L"Build a filter for this search",L"建立此搜尋專用篩選"),child,r,this);
        headingSecondary.Create(L"",child,r,this);
        engine.Create(L"TrafficMonitor bounded NFA v1 | UTF-16 | ASCII case folding | leftmost-longest",child,r,this);
        patternLabel.Create(T(L"&Pattern / literal query",L"表示式／文字 (&P)"),child,r,this);
        pattern.CreateEx(0,L"EDIT",draft.query.c_str(),child|WS_TABSTOP|ES_AUTOHSCROLL,r,this,Pattern);pattern.SetLimitText(256);
        mode.Create(draft.regex?T(L"Mode: regex",L"模式：正規式"):T(L"Mode: literal",L"模式：文字"),r,this,Mode);
        caseButton.Create(draft.ignoreCase?T(L"ASCII case: ignore",L"忽略 ASCII 大小寫"):T(L"Case: exact",L"區分大小寫"),r,this,Case);
        any.Create(T(L"Any .",L"任意 ."),r,this,InsertAny);digit.Create(L"Digit [0-9]",r,this,InsertDigit);word.Create(L"Word \\w+",r,this,InsertWord);
        group.Create(L"Group (?:...)",r,this,InsertGroup);start.Create(L"Start ^",r,this,InsertStart);end.Create(L"End $",r,this,InsertEnd);
        help.Create(T(L"Supported: literals, . [a-z] [^a] \\d \\w \\s ^ $ | (?:...) ? * +. Escapes: \\n \\r \\t and punctuation. Dot excludes CR/LF. Groups do not capture. No lazy/count quantifiers, lookaround, backreferences, Unicode properties, other flavors or debugger. Full RegexBuddy parity is unfinished.",
            L"支援：文字、. [a-z] [^a] \\d \\w \\s ^ $ | (?:...) ? * +。跳脫：\\n \\r \\t 及標點。點號不包括換行。群組不擷取；暫不支援惰性／次數量詞、環視、反向參照、Unicode 屬性、其他引擎或偵錯。完整 RegexBuddy 功能仍未完成。"),child,r,this);
        subjectLabel.Create(T(L"&Example subject (local preview only)",L"範例文字，只供本機預覽 (&E)"),child,r,this);
        subject.CreateEx(0,L"EDIT",subjects.empty()?L"Ethernet 1\r\nWi-Fi 2":subjects.front().c_str(),child|WS_TABSTOP|ES_MULTILINE|ES_AUTOVSCROLL,r,this,Subject);subject.SetLimitText(8192);
        replacementLabel.Create(T(L"&Replacement (literal text, no group substitution)",L"取代文字，不展開群組參照 (&R)"),child,r,this);
        replacement.CreateEx(0,L"EDIT",L"",child|WS_TABSTOP|ES_AUTOHSCROLL,r,this,Replacement);replacement.SetLimitText(4096);
        run.Create(T(L"Search",L"搜尋"),r,this,Run);replace.Create(T(L"Replace preview",L"取代預覽"),r,this,Replace);split.Create(T(L"Split preview",L"分割預覽"),r,this,Split);
        results.Create(r,this,Results);
        output.CreateEx(0,L"EDIT",L"",child|WS_TABSTOP|ES_MULTILINE|ES_READONLY|ES_AUTOVSCROLL,r,this,Output);
        note.Create(L"",child,r,this);
        apply.Create(T(L"Apply to search",L"套用至搜尋"),r,this,Apply);cancel.Create(T(L"Cancel",L"取消"),r,this,Cancel);
        viewport.Create(this, 55100);
        viewport.scroll = [this](UINT code,int position){OnVScroll(code,position,nullptr);};
        viewport.background = [this]{return static_cast<HBRUSH>(background.GetSafeHandle());};
        panel.Create(&viewport,55101);
        resultsLabel.Create(L"",child,r,&panel);
        outputLabel.Create(L"",child,r,&panel);
        options.Create(L"",r,this,OptionsToggle);
        for(int index=0;index<4;++index)pages[index].Create(L"",r,this,TestPage+index);
        for(CWnd* control:std::initializer_list<CWnd*>{&savedLabel,&savedList,&loadSaved,&refreshSaved,&nameLabel,&savedName,&saveNew,&renameSaved,&deleteSaved,&recoverSaved,&exportSaved,&importSaved,&collectionNote,&engine,&mode,&caseButton,&any,&digit,&word,&group,&start,&end,&help,&subjectLabel,&subject,&replacementLabel,&replacement,&run,&replace,&split,&results,&output})control->SetParent(&panel);
        Theme();
        Labels();Fonts();RECT size{0,0,Px(1280),Px(840)};AdjustWindowRectEx(&size,GetStyle(),FALSE,GetExStyle());
        MONITORINFO monitor{sizeof(monitor)};GetMonitorInfoW(MonitorFromWindow(anchor,MONITOR_DEFAULTTONEAREST),&monitor);
        int width=(std::min)(size.right-size.left,monitor.rcWork.right-monitor.rcWork.left),height=(std::min)(size.bottom-size.top,monitor.rcWork.bottom-monitor.rcWork.top);
        SetWindowPos(nullptr,0,0,width,height,SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE);Reanchor();
        Layout();loading=false;SetTimer(1,40,nullptr);Execute(Operation::Search,false);CollectionStart(CollectionAction::Read);pattern.SetFocus();return FALSE;
    }
    void Workbench::Labels()
    {
        SetWindowText(T(L"Search pattern workbench",L"搜尋表示式工作台"));
        savedLabel.SetWindowText(T(L"Saved expressions (local only)",L"已儲存表示式，只限本機"));
        loadSaved.SetLabel(T(L"&Load draft",L"載入草稿 (&L)"));
        refreshSaved.SetLabel(T(L"Refresh",L"重新整理"));
        nameLabel.SetWindowText(T(L"Collection &name",L"儲存名稱 (&N)"));
        saveNew.SetLabel(T(L"Save new",L"儲存新項目"));
        renameSaved.SetLabel(T(L"Rename",L"重新命名"));
        deleteSaved.SetLabel(T(L"Delete",L"刪除"));
        recoverSaved.SetLabel(T(L"Recover backup",L"還原備份"));
        exportSaved.SetLabel(T(L"Export collection...",L"匯出收藏..."));
        importSaved.SetLabel(T(L"Import collection...",L"匯入收藏..."));
        const bool bilingual=NativePreferences::Current().values.language==NativePreferences::Language::Bilingual;
        heading.SetWindowText(bilingual?CString(L"Build a filter for this search"):T(L"Build a filter for this search",L"建立此搜尋專用篩選"));
        headingSecondary.SetWindowText(bilingual?L"建立此搜尋專用篩選":L"");
        patternLabel.SetWindowText(T(L"&Pattern / literal query",L"表示式／文字 (&P)"));
        any.SetLabel(T(L"Any .",L"任意 ."));
        help.SetWindowText(T(L"Supported: literals, . [a-z] [^a] \\d \\w \\s ^ $ | (?:...) ? * +. Escapes: \\n \\r \\t and punctuation. Dot excludes CR/LF. Groups do not capture. No lazy/count quantifiers, lookaround, backreferences, Unicode properties, other flavors or debugger. Full RegexBuddy parity is unfinished.",L"支援：文字、. [a-z] [^a] \\d \\w \\s ^ $ | (?:...) ? * +。跳脫：\\n \\r \\t 及標點。點號不包括換行。群組不擷取；暫不支援惰性／次數量詞、環視、反向參照、Unicode 屬性、其他引擎或偵錯。完整 RegexBuddy 功能仍未完成。"));
        subjectLabel.SetWindowText(T(L"&Example subject (local preview only)",L"範例文字，只供本機預覽 (&E)"));
        replacementLabel.SetWindowText(T(L"&Replacement (literal text, no group substitution)",L"取代文字，不展開群組參照 (&R)"));
        run.SetLabel(T(L"Search",L"搜尋"));
        replace.SetLabel(T(L"Replace preview",L"取代預覽"));
        split.SetLabel(T(L"Split preview",L"分割預覽"));
        apply.SetLabel(T(L"Apply to search",L"套用至搜尋"));
        cancel.SetLabel(T(L"Cancel",L"取消"));
        mode.SetLabel(draft.regex?T(L"Mode: regex",L"模式：正規式"):T(L"Mode: literal",L"模式：文字"));caseButton.SetLabel(draft.ignoreCase?T(L"ASCII case: ignore",L"忽略 ASCII 大小寫"):T(L"Case: exact",L"區分大小寫"));
        digit.SetLabel(T(L"Digit [0-9]",L"數字 [0-9]"));word.SetLabel(T(L"Word \\w+",L"字元 \\w+"));group.SetLabel(T(L"Group (?:...)",L"群組 (?:...)"));start.SetLabel(T(L"Start ^",L"開頭 ^"));end.SetLabel(T(L"End $",L"結尾 $"));
        const wchar_t* titles[]={L"Test",L"Replace",L"Explain",L"Library"};
        const wchar_t* translations[]={L"測試",L"取代",L"說明",L"收藏"};
        for(int index=0;index<4;++index)pages[index].SetLabel(T(titles[index],translations[index]));
        options.SetLabel(T(L"Options",L"選項"));
        resultsLabel.SetWindowText(T(L"Matches",L"符合結果"));
        outputLabel.SetWindowText(T(L"Preview output (read only)",L"預覽輸出，只供閱讀"));
        savedList.SetPrompt(T(L"Choose a saved expression",L"選取已儲存表示式"));
        viewport.SetWindowText(T(L"Workbench task",L"工作台內容"));
        viewport.SetScrollName(T(L"Scroll workbench task",L"捲動工作台內容"));
        for(const auto& entry:messages)if(::IsWindow(entry.first))::SetWindowTextW(entry.first,T(entry.second.first.c_str(),entry.second.second.c_str()));
    }
    void Workbench::Theme()
    {
        NativeMaterial::ApplyNativeFrameAppearance(m_hWnd,appearance);
        for(auto* button:{&mode,&caseButton,&run,&replace,&split,&apply,&cancel,&any,&digit,&word,&group,&start,&end,&saveNew,&loadSaved,&renameSaved,&deleteSaved,&recoverSaved,&refreshSaved,&options})
        {
            button->SetAppearance(appearance);
            button->SetVariant(button==&apply||button==&run?NativeMaterial::ButtonVariant::Primary:NativeMaterial::ButtonVariant::Tonal);
        }
        cancel.SetVariant(NativeMaterial::ButtonVariant::Text);
        deleteSaved.SetVariant(NativeMaterial::ButtonVariant::Text);
        for(auto& button:pages)button.SetAppearance(appearance);
        for(auto* field:{&pattern,&replacement,&savedName})field->SetAppearance(appearance);
        subject.SetAppearance(appearance);
        output.SetAppearance(appearance);
        savedList.SetAppearance(appearance);
        results.SetAppearance(appearance);
        viewport.SetAppearance(appearance);
        panel.SetAppearance(appearance);
        for(auto* button:{&mode,&caseButton,&run,&replace,&split,&any,&digit,&word,&group,&start,&end,&saveNew,&loadSaved,&renameSaved,&deleteSaved,&recoverSaved,&refreshSaved})
            button->SetSurfaceColor(panel.SurfaceColor());
        savedList.SetSurfaceColor(panel.SurfaceColor());
    }
    LRESULT Workbench::OnPreferences(WPARAM generation,LPARAM)
    {
        if(!preferences.Accepts(generation))return 0;
        appearance=NativePreferences::EffectiveAppearance();
        chinese=NativePreferences::Current().values.language==NativePreferences::Language::Cantonese;
        const auto colors=NativeMaterial::Palette::Resolve(appearance);
        background.DeleteObject();
        background.CreateSolidBrush(RGB(BYTE(colors.surface.r*255),BYTE(colors.surface.g*255),BYTE(colors.surface.b*255)));
        Labels();Theme();Layout();RevealFocus();
        return 0;
    }
    void Workbench::Finish(bool commit)
    {
        const Options value=draft;auto callback=commit?std::move(accept):std::function<void(const Options&)>{};
        HWND target=anchor,parent=owner;HWND foreground=::GetForegroundWindow();
        const bool returnFocus=foreground==m_hWnd || (foreground && ::IsChild(m_hWnd,foreground));
        session.Cancel();DestroyWindow();
        // The dialog has deleted itself. Only copied values may be used below.
        if(callback && ::IsWindow(parent) && ::IsWindow(target))callback(value);
        if(returnFocus && ::IsWindow(parent) && ::IsWindow(target))
        {::SetForegroundWindow(parent);::SetFocus(target);}
    }
    void Workbench::Reanchor()
    {
        RECT origin{};if(!::GetWindowRect(anchor,&origin)||EqualRect(&origin,&previousAnchor))return;previousAnchor=origin;
        RECT bounds{};GetWindowRect(&bounds);const int width=bounds.right-bounds.left,height=bounds.bottom-bounds.top;
        MONITORINFO monitor{sizeof(monitor)};if(!GetMonitorInfoW(MonitorFromWindow(anchor,MONITOR_DEFAULTTONEAREST),&monitor))return;
        LONG x=origin.left,y=origin.bottom;
        if(y+height>monitor.rcWork.bottom && origin.top-height>=monitor.rcWork.top)y=origin.top-height;
        x=(std::max)(monitor.rcWork.left,(std::min)(x,monitor.rcWork.right-width));
        y=(std::max)(monitor.rcWork.top,(std::min)(y,monitor.rcWork.bottom-height));
        SetWindowPos(nullptr,x,y,0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE);
    }
    void Workbench::Fonts()
    {
        HFONT old=static_cast<HFONT>(font.Detach());font.CreateFont(-Px(14),0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,L"Segoe UI");
        ::EnumChildWindows(m_hWnd,[](HWND child,LPARAM value)->BOOL{::SendMessageW(child,WM_SETFONT,value,TRUE);return TRUE;},reinterpret_cast<LPARAM>(font.GetSafeHandle()));
        HFONT oldHeading=static_cast<HFONT>(headingFont.Detach());
        headingFont.CreateFont(-Px(24),0,0,0,FW_SEMIBOLD,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,L"Segoe UI");
        heading.SetFont(&headingFont);
        if(oldHeading)DeleteObject(oldHeading);
        if(old)DeleteObject(old);
    }
    void Workbench::RevealFocus()
    {
        HWND focused=::GetFocus();
        if(!viewport.GetSafeHwnd()||!focused||!::IsChild(viewport,focused))return;
        CRect rectangle;::GetWindowRect(focused,rectangle);viewport.ScreenToClient(rectangle);
        const int top=MulDiv(rectangle.top,96,dpi),bottom=MulDiv(rectangle.bottom,96,dpi);
        int next=scroll;
        if(top<0||bottom-top>bodyHeight)next+=top;
        else if(bottom>bodyHeight)next+=bottom-bodyHeight;
        next=(std::max)(0,(std::min)(next,(std::max)(0,contentHeight-bodyHeight)));
        if(next!=scroll){scroll=next;Layout();}
    }
    void Workbench::Layout()
    {
        if(!viewport.GetSafeHwnd())return;
        CRect bounds;GetClientRect(bounds);
        const int width=MulDiv(bounds.Width(),96,dpi),height=MulDiv(bounds.Height(),96,dpi);
        CClientDC dc(this);auto* previous=dc.SelectObject(&font);
        auto measure=[&](CWnd& control,int available,int minimum)
        {
            CString value;control.GetWindowText(value);
            CRect text(0,0,Px((std::max)(1,available)),0);
            CFont* previousFont=control.GetFont()?dc.SelectObject(control.GetFont()):nullptr;
            dc.DrawText(value,text,DT_CALCRECT|DT_WORDBREAK|DT_NOPREFIX);
            if(previousFont)dc.SelectObject(previousFont);
            return (std::max)(minimum,MulDiv(text.Height(),96,dpi));
        };
        auto move=[&](CWnd& control,int x,int y,int w,int h){control.MoveWindow(Px(x),Px(y),Px(w),Px(h));};
        const bool compact=height<600;
        int y=compact?8:16;
        heading.ShowWindow(compact?SW_HIDE:SW_SHOW);
        const bool bilingual=NativePreferences::Current().values.language==NativePreferences::Language::Bilingual;
        headingSecondary.ShowWindow(!compact&&bilingual?SW_SHOW:SW_HIDE);
        int h=0;
        if(!compact)
        {
            h=measure(heading,width-40,28);move(heading,20,y,width-40,h);y+=h+2;
            if(bilingual){h=measure(headingSecondary,width-40,20);move(headingSecondary,20,y,width-40,h);y+=h;}
            y+=10;
        }
        h=measure(patternLabel,width-40,20);move(patternLabel,20,y,width-40,h);y+=h+6;
        const int buttonHeight=(std::max)(44,measure(options,100,20)+20);
        move(pattern,20,y,width-172,buttonHeight);move(options,width-140,y,120,buttonHeight);y+=buttonHeight+10;
        const int tabWidth=(width-64)/4;
        int tabHeight=44;
        for(auto& button:pages)tabHeight=(std::max)(tabHeight,measure(button,tabWidth-24,20)+16);
        for(int index=0;index<4;++index)
        {
            pages[index].SetVariant(index==page?NativeMaterial::ButtonVariant::Tonal:NativeMaterial::ButtonVariant::Text);
            move(pages[index],20+index*(tabWidth+8),y,tabWidth,tabHeight);
        }
        y+=tabHeight+(compact?6:12);
        const int actionHeight=(std::max)(44,measure(apply,180,20)+16);
        const int noteHeight=measure(note,width-40,20);
        const int footer=noteHeight+actionHeight+(compact?20:28);
        move(note,20,height-footer+8,width-40,noteHeight);
        move(cancel,width-360,height-actionHeight-12,140,actionHeight);
        move(apply,width-208,height-actionHeight-12,188,actionHeight);
        bodyHeight=(std::max)(24,height-footer-y);
        move(viewport,0,y,width,bodyHeight);
        struct Box{CWnd* control;int x,y,w,h;};std::vector<Box> boxes;
        std::vector<CWnd*> bodyControls{&savedLabel,&savedList,&loadSaved,&refreshSaved,&nameLabel,&savedName,&saveNew,&renameSaved,&deleteSaved,&recoverSaved,&exportSaved,&importSaved,&collectionNote,&engine,&mode,&caseButton,&any,&digit,&word,&group,&start,&end,&help,&subjectLabel,&subject,&replacementLabel,&replacement,&run,&replace,&split,&results,&output,&resultsLabel,&outputLabel};
        const int left=36,available=width-88;
        int cursor=16;
        auto place=[&](CWnd& control,int minimum,bool fixed=false)
        {
            int size=fixed?minimum:measure(control,available,minimum);
            boxes.push_back({&control,left,cursor,available,size});cursor+=size+12;
        };
        auto row=[&](std::initializer_list<NativeMaterial::Button*> controls)
        {
            const int cell=(std::min)(260,(available-8*(static_cast<int>(controls.size())-1))/static_cast<int>(controls.size()));
            int rowHeight=44;for(auto* button:controls)rowHeight=(std::max)(rowHeight,measure(*button,cell-24,20)+20);
            int x=left;for(auto* button:controls){boxes.push_back({button,x,cursor,cell,rowHeight});x+=cell+8;}cursor+=rowHeight+12;
        };
        if(optionsVisible){row({&mode,&caseButton});row({&any,&digit,&word});row({&group,&start,&end});}
        if(page==0)
        {
            if(width>=880)
            {
                const int column=(available-24)/2;
                const int labelHeight=(std::max)(measure(subjectLabel,column,20),measure(resultsLabel,column,20));
                boxes.push_back({&subjectLabel,left,cursor,column,labelHeight});
                boxes.push_back({&resultsLabel,left+column+24,cursor,column,labelHeight});cursor+=labelHeight+8;
                boxes.push_back({&subject,left,cursor,column,260});
                boxes.push_back({&results,left+column+24,cursor,column,260});cursor+=272;
            }
            else{place(subjectLabel,20);place(subject,160,true);place(resultsLabel,20);place(results,160,true);}
            row({&run});
        }
        else if(page==1)
        {
            place(replacementLabel,20);place(replacement,48,true);
            place(subjectLabel,20);place(subject,140,true);row({&replace,&split});
            place(outputLabel,20);place(output,200,true);
        }
        else if(page==2){place(engine,36);place(help,80);}
        else
        {
            place(savedLabel,20);place(savedList,buttonHeight,true);row({&loadSaved,&refreshSaved});
            place(nameLabel,20);place(savedName,48,true);row({&saveNew,&renameSaved});row({&deleteSaved,&recoverSaved});row({&exportSaved,&importSaved});place(collectionNote,36);
        }
        contentHeight=cursor+16;
        scroll=(std::max)(0,(std::min)(scroll,(std::max)(0,contentHeight-bodyHeight)));
        viewport.SetMetrics(contentHeight,bodyHeight,scroll);
        move(panel,20,-scroll,width-56,contentHeight);
        panel.SetWindowPos(&CWnd::wndBottom,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);
        for(auto* control:bodyControls)
        {
            const bool visible=std::any_of(boxes.begin(),boxes.end(),[control](const Box& box){return box.control==control;});
            if(visible!=((control->GetStyle()&WS_VISIBLE)!=0))control->ShowWindow(visible?SW_SHOW:SW_HIDE);
        }
        for(const auto& box:boxes)move(*box.control,box.x-20,box.y,box.w,box.h);
        subject.SyncScroll();output.SyncScroll();results.SyncScroll();
        dc.SelectObject(previous);Invalidate(FALSE);
    }
    Options Workbench::Read(){CString text;pattern.GetWindowText(text);auto value=draft;value.query=text.GetString();return value;}
    void Workbench::Changed(){if(loading)return;collectionValidation.Cancel();validatingCollection=false;confirmDelete=false;confirmRecovery=false;session.Cancel();ready=false;applying=false;apply.EnableWindow(FALSE);Note(note,L"Draft changed. Run a preview before applying.",L"草稿已更改。請先執行預覽再套用。");}
    void Workbench::Execute(Operation requestedOperation,bool commit)
    {
        if(!commit)operation=requestedOperation;
        CString sample,repl;subject.GetWindowText(sample);replacement.GetWindowText(repl);
        if(commit)subjects=readSubjects();
        applying=commit;ready=false;apply.EnableWindow(FALSE);Note(note,L"Evaluating locally; Cancel retains the accepted search.",L"正在本機處理；取消會保留已套用搜尋。");
        session.Request(Read(),commit?subjects:std::vector<std::wstring>{sample.GetString()},commit?Operation::Filter:requestedOperation,repl.GetString());
    }
    void Workbench::OnTimer(UINT_PTR id)
    {
        if(id!=1){CDialog::OnTimer(id);return;}
        if(!::IsWindow(owner)||!::IsWindow(anchor)||!::IsWindowVisible(owner)){Finish(false);return;}
        Reanchor();CollectionPoll();
        if(!::IsWindowEnabled(owner))
        {session.Cancel();applying=false;ready=false;apply.EnableWindow(FALSE);Note(note,L"Owner is busy. Run a preview again when it is available.",L"來源視窗忙碌中。待可用時請重新預覽。");return;}
        Result result;if(!session.Poll(result))return;
        if(!result.complete){ready=false;applying=false;messages.erase(note.GetSafeHwnd());note.SetWindowText(result.error.c_str());Layout();return;}
        if(applying)
        {
            if(subjects!=readSubjects()){applying=false;ready=true;apply.EnableWindow(TRUE);Note(note,L"Search rows changed. Apply again to validate the current rows.",L"搜尋項目已更改。請再次套用以驗證目前項目。");return;}
            draft=Read();Finish(true);return;
        }
        last=std::move(result);ready=true;apply.EnableWindow(TRUE);results.ResetContent();
        for(size_t i=0;i<last.matches.size();++i){CString line;line=NativePreferences::Format(L"Match %zu: offset %zu, length %zu",L"符合 %zu：位置 %zu，長度 %zu",i+1,last.matches[i].start,last.matches[i].length);results.AddString(line);}
        output.SetWindowText(last.output.c_str());Note(note,L"Preview complete. Select a match to highlight its UTF-16 span. Apply validates all owner rows.",L"預覽完成。選取結果可標示 UTF-16 範圍。套用時會驗證全部來源項目。");
    }
    BOOL Workbench::OnCommand(WPARAM w,LPARAM l)
    {
        UINT id=LOWORD(w),code=HIWORD(w);
        if(code==BN_CLICKED&&id>=TestPage&&id<=LibraryPage)
        {
            offsets[page]=scroll;page=id-TestPage;scroll=offsets[page];Layout();return TRUE;
        }
        if(code==BN_CLICKED&&id==OptionsToggle){optionsVisible=!optionsVisible;Layout();return TRUE;}
        if(code==EN_SETFOCUS){RevealFocus();}
        if(code==EN_UPDATE){subject.SyncScroll();output.SyncScroll();}
        if(id==SavedList&&code==CBN_SELCHANGE){confirmDelete=false;confirmRecovery=false;int selected=savedList.GetCurSel();if(selected>=0&&static_cast<size_t>(selected)<savedRecords.size())savedName.SetWindowText(savedRecords[selected].name.c_str());return TRUE;}
        if(id==SavedName&&code==EN_CHANGE){collectionValidation.Cancel();validatingCollection=false;confirmDelete=false;confirmRecovery=false;return TRUE;}
        if((id==Pattern||id==Subject||id==Replacement)&&code==EN_CHANGE){Changed();return TRUE;}
        if(id==Results&&code==LBN_SELCHANGE){int row=results.GetCurSel();if(row>=0&&static_cast<size_t>(row)<last.matches.size()){auto s=last.matches[row];subject.SetSel(static_cast<int>(s.start),static_cast<int>(s.start+s.length));subject.SetFocus();}return TRUE;}
        if(code!=BN_CLICKED)return CDialog::OnCommand(w,l);
        switch(id)
        {
        case SaveNew:CollectionValidate(true);return TRUE;
        case LoadSaved:CollectionValidate(false);return TRUE;
        case RefreshSaved:CollectionStart(CollectionAction::Read);return TRUE;
        case ExportSaved:CollectionTransfer(false);return TRUE;
        case ImportSaved:CollectionTransfer(true);return TRUE;
        case RecoverSaved:
            if(!confirmRecovery){confirmRecovery=true;Note(collectionNote,L"Press Recover backup again to replace saved records with the previous file. Draft stays unchanged.",L"再次按還原備份，以之前版本取代儲存項目；草稿不變。");}
            else{confirmRecovery=false;CollectionStart(CollectionAction::Recover);}return TRUE;
        case RenameSaved:case DeleteSaved:
        {
            int selected=savedList.GetCurSel();if(selected<0||static_cast<size_t>(selected)>=savedRecords.size()){Note(note,L"Select a saved expression first.",L"請先選取已儲存表示式。");return TRUE;}
            if(id==DeleteSaved&&!confirmDelete){confirmDelete=true;Note(collectionNote,L"Press Delete again to remove the selected saved expression. Draft stays unchanged.",L"再次按刪除以移除已選項目；草稿不變。");return TRUE;}
            confirmDelete=false;CString name;savedName.GetWindowText(name);CollectionStart(id==DeleteSaved?CollectionAction::Delete:CollectionAction::Rename,savedRecords[selected],name.GetString());return TRUE;
        }
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
    void Workbench::CollectionControls(bool enabled)
    {
        for(auto* button:{&saveNew,&loadSaved,&renameSaved,&deleteSaved,&recoverSaved,&refreshSaved,&exportSaved,&importSaved})button->EnableWindow(enabled);
        savedList.EnableWindow(enabled);savedName.EnableWindow(enabled);
    }
    void Workbench::CollectionStart(CollectionAction action,const SavedExpression& record,const std::wstring& newName)
    {
        if(collectionWorker.valid())return;
        collectionValidation.Cancel();validatingCollection=false;confirmDelete=false;confirmRecovery=false;
        CollectionControls(false);const auto directory=theApp.m_config_dir;
        try{collectionWorker=std::async(std::launch::async,[directory,action,record,newName]{return UpdateCollection(directory,action,record,newName);});}
        catch(...){CollectionControls(true);Note(collectionNote,L"Cannot start collection storage. Draft is unchanged.",L"無法啟動儲存工作；草稿不變。");}
    }
    void Workbench::CollectionTransfer(bool import)
    {
        if(collectionWorker.valid())return;
        confirmDelete=false;confirmRecovery=false;collectionValidation.Cancel();validatingCollection=false;
        const CString filter=T(L"TrafficMonitor regex collection (*.tmregex)|*.tmregex|All files (*.*)|*.*||",L"TrafficMonitor 表示式收藏 (*.tmregex)|*.tmregex|所有檔案 (*.*)|*.*||");
        CFileDialog dialog(import?TRUE:FALSE,L"tmregex",nullptr,import?(OFN_FILEMUSTEXIST|OFN_PATHMUSTEXIST|OFN_HIDEREADONLY):(OFN_OVERWRITEPROMPT|OFN_PATHMUSTEXIST|OFN_HIDEREADONLY),filter,this);
        if(dialog.DoModal()!=IDOK)return;
        const std::wstring path=dialog.GetPathName().GetString();
        ImportMode importMode=ImportMode::Merge;
        if(import)
        {
            const int choice=MessageBoxW(T(L"Merge into the saved collection? Yes merges (existing names stay; conflicting names are rejected). No replaces the whole saved collection. Cancel keeps everything unchanged.",L"是否併入已儲存收藏？「是」併入（現有名稱保留，名稱衝突會被拒絕）；「否」取代整個已儲存收藏；「取消」不作任何更改。"),T(L"Import collection",L"匯入收藏"),MB_YESNOCANCEL|MB_ICONQUESTION|MB_DEFBUTTON1);
            if(choice==IDCANCEL)return;
            if(choice==IDNO)
            {
                if(MessageBoxW(T(L"Replace overwrites every saved expression with the imported file. Replace the saved collection?",L"取代會以匯入檔案覆蓋所有已儲存表示式。確定取代？"),T(L"Confirm replace",L"確認取代"),MB_YESNO|MB_ICONWARNING|MB_DEFBUTTON2)!=IDYES)return;
                importMode=ImportMode::Replace;
            }
        }
        CollectionControls(false);const auto directory=theApp.m_config_dir;
        try{collectionWorker=std::async(std::launch::async,[directory,path,import,importMode]{return import?ImportCollection(directory,path,importMode):ExportCollection(directory,path);});}
        catch(...){CollectionControls(true);Note(collectionNote,L"Cannot start collection transfer. Draft is unchanged.",L"無法啟動收藏傳輸；草稿不變。");}
    }
    void Workbench::CollectionValidate(bool save)
    {
        if(collectionWorker.valid())return;
        confirmDelete=false;confirmRecovery=false;
        session.Cancel();applying=false;ready=false;apply.EnableWindow(FALSE);
        if(save){CString name,repl;savedName.GetWindowText(name);replacement.GetWindowText(repl);pendingRecord={name.GetString(),Read(),operation,repl.GetString()};}
        else{int selected=savedList.GetCurSel();if(selected<0||static_cast<size_t>(selected)>=savedRecords.size()){Note(collectionNote,L"Select a saved expression first.",L"請先選取已儲存表示式。");return;}pendingRecord=savedRecords[selected];}
        savingCollection=save;validatingCollection=true;
        collectionValidation.Request(pendingRecord.options,{},Operation::Search,pendingRecord.replacement);
        Note(collectionNote,L"Validating the named expression locally; owner filter is unchanged.",L"正在本機驗證表示式；來源篩選不變。");
    }
    void Workbench::CollectionPoll()
    {
        if(collectionWorker.valid()&&collectionWorker.wait_for(std::chrono::milliseconds(0))==std::future_status::ready)
        {
            auto result=collectionWorker.get();CollectionControls(true);
            if(!result.complete){collectionNote.SetWindowText(result.error.c_str());return;}
            savedRecords=std::move(result.records);savedList.ResetContent();for(const auto& item:savedRecords)savedList.AddString(item.name.c_str());
            Note(collectionNote,L"Saved collection ready. Select and Load draft explicitly; Apply remains separate.",L"儲存項目已就緒。請選取並載入草稿；套用為獨立操作。");
        }
        Result checked;if(!validatingCollection||!collectionValidation.Poll(checked))return;validatingCollection=false;
        if(!checked.complete){collectionNote.SetWindowText(checked.error.c_str());return;}
        if(savingCollection){CollectionStart(CollectionAction::Save,pendingRecord);return;}
        loading=true;draft=pendingRecord.options;operation=pendingRecord.operation;
        pattern.SetWindowText(draft.query.c_str());replacement.SetWindowText(pendingRecord.replacement.c_str());
        mode.SetLabel(draft.regex?T(L"Mode: regex",L"模式：正規式"):T(L"Mode: literal",L"模式：文字"));
        caseButton.SetLabel(draft.ignoreCase?T(L"ASCII case: ignore",L"忽略 ASCII 大小寫"):T(L"Case: exact",L"區分大小寫"));
        loading=false;Changed();
        collectionNote.SetWindowText(operation==Operation::Replace?T(L"Loaded draft: Replace. Owner filter unchanged.",L"已載入取代草稿；來源篩選不變。"):operation==Operation::Split?T(L"Loaded draft: Split. Owner filter unchanged.",L"已載入分割草稿；來源篩選不變。"):T(L"Loaded draft: Search. Owner filter unchanged.",L"已載入搜尋草稿；來源篩選不變。"));
        Execute(operation,false);pattern.SetFocus();
    }
    HBRUSH Workbench::OnCtlColor(CDC* dc,CWnd* control,UINT)
    {
        if(auto* field=DYNAMIC_DOWNCAST(NativeMaterial::TextField,control))return field->Color(dc);
        auto c=NativeMaterial::Palette::Resolve(appearance);dc->SetTextColor(RGB(BYTE(c.onSurface.r*255),BYTE(c.onSurface.g*255),BYTE(c.onSurface.b*255)));dc->SetBkColor(RGB(BYTE(c.surface.r*255),BYTE(c.surface.g*255),BYTE(c.surface.b*255)));return static_cast<HBRUSH>(background.GetSafeHandle());}
    void Workbench::OnGetMinMaxInfo(MINMAXINFO* info){RECT r{0,0,Px(640),Px(400)};AdjustWindowRectEx(&r,GetStyle(),FALSE,GetExStyle());info->ptMinTrackSize={r.right-r.left,r.bottom-r.top};}
    LRESULT Workbench::OnDpi(WPARAM w,LPARAM l){dpi=HIWORD(w);auto r=reinterpret_cast<RECT*>(l);SetWindowPos(nullptr,r->left,r->top,r->right-r->left,r->bottom-r->top,SWP_NOZORDER|SWP_NOACTIVATE);Fonts();Layout();return 0;}
}
Editor::Editor():m_state(std::make_shared<EditorState>()){}
Editor::~Editor(){Close();}
bool Editor::IsOpen() const{return m_state->window && ::IsWindow(m_state->window);}
bool Editor::Contains(HWND window) const{return IsOpen() && (window==m_state->window || ::IsChild(m_state->window,window));}
void Editor::Close(){if(IsOpen())::SendMessageW(m_state->window,WM_CLOSE,0,0);}
bool Editor::Translate(MSG* message)
{
    if(!Contains(message->hwnd))return false;
    auto* editor=static_cast<Workbench*>(CWnd::FromHandlePermanent(m_state->window));
    return editor && editor->Translate(message);
}
bool Editor::Open(HWND owner,HWND anchor,const Options& accepted,std::function<std::vector<std::wstring>()> subjects,
    std::function<void(const Options&)> apply,bool chinese,NativeMaterial::Appearance appearance)
{
    if(IsOpen()){::SetForegroundWindow(m_state->window);return true;}
    if(!::IsWindow(owner)||!::IsWindow(anchor))return false;
    struct Template {DLGTEMPLATE dialog;WORD menu{},className{},title{};} layout{};
    layout.dialog.style=WS_POPUP|WS_CAPTION|WS_SYSMENU|WS_THICKFRAME|WS_CLIPCHILDREN;
    layout.dialog.dwExtendedStyle=WS_EX_CONTROLPARENT|WS_EX_TOOLWINDOW;layout.dialog.cx=500;layout.dialog.cy=450;
    auto owned=std::make_unique<Workbench>();auto* editor=owned.get();editor->state=m_state;editor->draft=accepted;editor->subjects=subjects();editor->readSubjects=std::move(subjects);
    editor->accept=std::move(apply);editor->anchor=anchor;editor->owner=owner;editor->chinese=chinese;editor->appearance=appearance;
    if(!editor->CreateIndirect(&layout.dialog,CWnd::FromHandle(owner)))return false;
    editor->autoDelete=true;owned.release();
    editor->ShowWindow(SW_SHOW);editor->SetForegroundWindow();return true;
}
}
