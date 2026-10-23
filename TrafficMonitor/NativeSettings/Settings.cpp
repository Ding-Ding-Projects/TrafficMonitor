#include "../stdafx.h"
#include "Settings.h"
#include "../NativeUpdate/SilentUpdater.h"
#include "../NativeMaterial/Controls.h"
#include "../NativeRegex/Workbench.h"
#include "../NativeMaterial/SettingsControls.h"
#include <array>
#include <cwctype>
#include <imm.h>
#include <memory>

namespace NativeSettings
{
    namespace
    {
        unsigned activeEditors{};
        struct EditorLifetime { EditorLifetime() { ++activeEditors; } ~EditorLifetime() { --activeEditors; } };
        enum : UINT { PageMonitor=54001, PageFloating, PageTaskbar, Search, Clear, Apply, Revert, Reset, Legacy, Interval, Theme, Regex, FirstCheck=54100 };
        std::wstring Fold(std::wstring value) { for(auto& c:value) c=static_cast<wchar_t>(std::towlower(c)); return value; }
        class Editor final : public CDialog
        {
        public:
            Editor(int page, std::function<void(const Values&)> apply) : m_page(page==1?2:1), m_apply(std::move(apply)) {}
        protected:
            BOOL OnInitDialog() override;
            BOOL OnCommand(WPARAM, LPARAM) override;
            void OnCancel() override;
            void OnOK() override { ApplyDraft(); }
            BOOL PreTranslateMessage(MSG*) override;
            afx_msg void OnPaint();
            afx_msg void OnTimer(UINT_PTR id) { if(id==0x4E92){if(m_filter.Poll())Layout();}else if(id==0x4E91) UpdateInfo(); else CDialog::OnTimer(id); }
            afx_msg BOOL OnEraseBkgnd(CDC*) { return TRUE; }
            afx_msg void OnSize(UINT,int,int) { Layout(); }
            afx_msg void OnGetMinMaxInfo(MINMAXINFO* info);
            afx_msg HBRUSH OnCtlColor(CDC* dc,CWnd*,UINT);
            afx_msg LRESULT OnDpi(WPARAM dpi,LPARAM rect);
            DECLARE_MESSAGE_MAP()
        private:
            struct Entry { int page; const wchar_t* english; const wchar_t* chinese; bool Values::*member; std::unique_ptr<NativeMaterial::SettingsCheck> control; };
            void Add(int page,const wchar_t* english,const wchar_t* chinese,bool Values::*member);
            CString T(const wchar_t* english,const wchar_t* chinese) const { return m_chinese?chinese:english; }
            int Px(int n) const { return MulDiv(n,m_dpi,96); }
            void Layout(); void Reload(); void SetFonts(); void SetTheme(); void ApplyDraft(); void ResetPage();
            bool CanDiscard();
            void UpdateInfo();
            void Note(const CString& text) { m_note.SetWindowText(text); }
            int m_page{}; UINT m_dpi{96}; bool m_chinese{},m_loading{},m_dirty{};
            Values m_values;
            std::function<void(const Values&)> m_apply;
            NativeMaterial::Appearance m_appearance{NativeMaterial::Appearance::Light};
            CFont m_font; CBrush m_background;
            CStatic m_title,m_note,m_intervalLabel,m_results,m_updateInfo;
            CEdit m_search,m_interval;
            NativeRegex::Filter m_filter;
            NativeRegex::Editor m_regexEditor;
            std::vector<std::wstring> Subjects() const;
            NativeMaterial::Button m_monitor,m_floating,m_taskbar,m_clear,m_applyButton,m_revert,m_reset,m_legacy,m_theme,m_regex;
            std::vector<Entry> m_entries;
        };
        BEGIN_MESSAGE_MAP(Editor,CDialog)
            ON_WM_PAINT()
            ON_WM_TIMER()
            ON_WM_ERASEBKGND()
            ON_WM_SIZE()
            ON_WM_GETMINMAXINFO()
            ON_WM_CTLCOLOR()
            ON_MESSAGE(WM_DPICHANGED,&Editor::OnDpi)
        END_MESSAGE_MAP()
        void Editor::Add(int page,const wchar_t* en,const wchar_t* zh,bool Values::*member)
        {
            Entry entry{page,en,zh,member,std::make_unique<NativeMaterial::SettingsCheck>()};
            entry.control->Create(T(en,zh),this,FirstCheck+static_cast<UINT>(m_entries.size()));
            m_entries.push_back(std::move(entry));
        }
        BOOL Editor::OnInitDialog()
        {
            CDialog::OnInitDialog(); m_dpi=GetDpiForWindow(m_hWnd); if(!m_dpi)m_dpi=96;
            LANGID language=theApp.m_general_data.language.language_id; if(!language)language=GetUserDefaultUILanguage();
            m_chinese=PRIMARYLANGID(language)==LANG_CHINESE;
            DWORD light=1,bytes=sizeof(light); RegGetValueW(HKEY_CURRENT_USER,L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",L"AppsUseLightTheme",RRF_RT_REG_DWORD,nullptr,&light,&bytes);
            m_appearance=light?NativeMaterial::Appearance::Light:NativeMaterial::Appearance::Dark;
            SetWindowText(T(L"TrafficMonitor settings",L"TrafficMonitor 設定")); SetIcon(theApp.GetMenuIcon(IDI_SETTINGS),FALSE);
            const CRect box(0,0,100,40); const DWORD child=WS_CHILD|WS_VISIBLE;
            m_title.Create(T(L"Settings for your monitoring workflow",L"監察工作設定"),child,box,this);
            m_updateInfo.Create(L"",child,box,this);
            m_note.Create(L"",child,box,this); m_results.Create(L"",child,box,this);
            m_intervalLabel.Create(T(L"Sample interval (200-30000 ms, in steps of 100)",L"採樣間隔（200-30000 毫秒，每次 100）"),child,box,this);
            m_search.CreateEx(WS_EX_CLIENTEDGE,L"EDIT",L"",child|WS_TABSTOP|ES_AUTOHSCROLL,box,this,Search);
            m_search.SetLimitText(256);
            m_regex.Create(T(L"Regex",L"正規式"),box,this,Regex); m_search.SendMessage(EM_SETCUEBANNER,TRUE,reinterpret_cast<LPARAM>(m_chinese?L"搜尋目前設定組別":L"Search this settings group"));
            m_interval.CreateEx(WS_EX_CLIENTEDGE,L"EDIT",L"",child|WS_TABSTOP|ES_NUMBER|ES_AUTOHSCROLL,box,this,Interval); m_interval.SetLimitText(5);
            m_monitor.Create(T(L"Monitoring",L"監察"),box,this,PageMonitor); m_floating.Create(T(L"Floating monitor",L"浮動監察器"),box,this,PageFloating);
            m_taskbar.Create(T(L"Taskbar",L"工作列"),box,this,PageTaskbar); m_theme.Create(T(L"Appearance",L"外觀"),box,this,Theme);
            m_clear.Create(T(L"Clear",L"清除"),box,this,Clear); m_applyButton.Create(T(L"Apply",L"套用"),box,this,Apply);
            m_revert.Create(T(L"Revert",L"還原"),box,this,Revert); m_reset.Create(T(L"Reset page draft",L"重設本頁草稿"),box,this,Reset);
            m_legacy.Create(T(L"All settings (legacy)",L"所有設定（舊版）"),box,this,Legacy);
            Add(0,L"Show notification-area icon",L"顯示通知區域圖示",&Values::tray);
            Add(0,L"Check for updates automatically every five minutes",L"每五分鐘自動檢查更新",&Values::automaticCheck);
            Add(0,L"Restart automatically when an update is ready and the interface is idle",L"更新就緒且介面閒置時自動重新啟動",&Values::automaticRestart);
            Add(1,L"Compact network-speed text",L"精簡網速文字",&Values::floatingCompact);
            Add(1,L"Use bytes instead of bits",L"以位元組取代位元",&Values::floatingBytes);
            Add(1,L"Separate values and units with a space",L"數值與單位之間留空格",&Values::floatingSpace);
            Add(1,L"Show monitor tooltips",L"顯示監察器工具提示",&Values::floatingTips);
            Add(1,L"Keep floating monitor above other windows",L"浮動監察器保持最上層",&Values::top);
            Add(1,L"Lock floating monitor position",L"鎖定浮動監察器位置",&Values::locked);
            Add(2,L"Compact network-speed text",L"精簡網速文字",&Values::taskbarCompact);
            Add(2,L"Use bytes instead of bits",L"以位元組取代位元",&Values::taskbarBytes);
            Add(2,L"Separate values and units with a space",L"數值與單位之間留空格",&Values::taskbarSpace);
            Add(2,L"Show taskbar tooltips",L"顯示工作列工具提示",&Values::taskbarTips);
            Add(2,L"Align taskbar values to the right",L"工作列數值靠右對齊",&Values::right);
            Add(2,L"Arrange taskbar values horizontally",L"工作列數值橫向排列",&Values::horizontal);
            SetFonts(); SetTheme(); Reload();
            RECT size{0,0,Px(800),Px(680)}; AdjustWindowRectEx(&size,GetStyle(),FALSE,GetExStyle());
            SetWindowPos(nullptr,0,0,size.right-size.left,size.bottom-size.top,SWP_NOMOVE|SWP_NOZORDER); CenterWindow(); Layout();
            SetTimer(0x4E92,40,nullptr);SetTimer(0x4E91,2000,nullptr); UpdateInfo();
            m_search.SetFocus(); return FALSE;
        }
        void Editor::SetFonts()
        {
            LOGFONT font{}; NONCLIENTMETRICS metrics{sizeof(metrics)};
            if(SystemParametersInfoW(SPI_GETNONCLIENTMETRICS,sizeof(metrics),&metrics,0))font=metrics.lfMessageFont;
            else wcscpy_s(font.lfFaceName,L"Segoe UI"); font.lfHeight=-Px(14);
            HFONT next=CreateFontIndirectW(&font); if(!next)return;
            HFONT old=static_cast<HFONT>(m_font.Detach()); m_font.Attach(next);
            for(CWnd* child=GetWindow(GW_CHILD);child;child=child->GetNextWindow())child->SetFont(&m_font);
            if(old)DeleteObject(old);
        }
        void Editor::SetTheme()
        {
            const auto c=NativeMaterial::Palette::Resolve(m_appearance);
            m_background.DeleteObject(); m_background.CreateSolidBrush(RGB(BYTE(c.surface.r*255),BYTE(c.surface.g*255),BYTE(c.surface.b*255)));
            for(auto* button:{&m_monitor,&m_floating,&m_taskbar,&m_clear,&m_applyButton,&m_revert,&m_reset,&m_legacy,&m_theme,&m_regex})button->SetAppearance(m_appearance);
            for(auto& entry:m_entries)entry.control->SetAppearance(m_appearance);
            RedrawWindow(nullptr,nullptr,RDW_INVALIDATE|RDW_ALLCHILDREN);
        }
        void Editor::Reload()
        {
            m_loading=true; m_values=Values::Current();
            for(auto& entry:m_entries) { entry.control->SetCheck(m_values.*entry.member?BST_CHECKED:BST_UNCHECKED); entry.control->Invalidate(); }
            CString interval; interval.Format(L"%d",m_values.interval); m_interval.SetWindowText(interval);
            m_loading=false; m_dirty=false;
            Note(T(L"Current values loaded. Apply saves changes. All settings retains the remaining controls.",L"已載入目前數值。套用後儲存更改；其餘控制項仍在所有設定內。")); Layout();
        }
        std::vector<std::wstring> Editor::Subjects() const
        {std::vector<std::wstring> values{L"Sample interval milliseconds 採樣間隔毫秒"};for(const auto& e:m_entries)values.push_back(std::wstring(e.english)+L" "+e.chinese);return values;}
        void Editor::Layout()
        {
            if(!m_search.GetSafeHwnd())return;
            CRect client; GetClientRect(client); int width=MulDiv(client.Width(),96,m_dpi),height=MulDiv(client.Height(),96,m_dpi);
            auto place=[&](CWnd& child,int x,int y,int w,int h){child.MoveWindow(Px(x),Px(y),Px((std::max)(1,w)),Px(h));};
            place(m_title,24,16,width-48,32);
            place(m_monitor,24,58,152,40);place(m_floating,188,58,180,40);place(m_taskbar,380,58,152,40);place(m_theme,544,58,width-568,40);
            place(m_search,24,112,width-264,36);place(m_regex,width-228,110,96,40);place(m_clear,width-120,110,96,40);
            CString query; m_search.GetWindowText(query); m_filter.options.query=query.GetString();m_filter.Request(Subjects());
            int y=164,count=0;
            size_t row=0;
            const bool showInterval=m_page==0&&m_filter.Visible(0);
            m_intervalLabel.ShowWindow(showInterval?SW_SHOW:SW_HIDE);m_interval.ShowWindow(showInterval?SW_SHOW:SW_HIDE);
            if(showInterval){place(m_intervalLabel,24,y,width-232,48);place(m_interval,width-184,y,160,36);y+=60;++count;}
            for(auto& entry:m_entries)
            {
                ++row;bool show=entry.page==m_page&&m_filter.Visible(row);entry.control->ShowWindow(show?SW_SHOW:SW_HIDE);
                if(show){place(*entry.control,24,y,width-48,48);y+=56;++count;}
            }
            CString results; results.Format(T(L"%d controls shown in this group",L"本組顯示 %d 個控制項"),count);
            if(!count)results=T(L"No settings match this search. Clear to show the group.",L"沒有符合搜尋的設定。清除後顯示本組。");
            if(!m_filter.status.empty())results+=L" | "+CString(m_filter.status.c_str());
            m_results.SetWindowText(results); place(m_results,24,y+4,width-48,26);
            place(m_note,24,height-112,width-48,48);
            place(m_applyButton,24,height-52,96,40);place(m_revert,132,height-52,96,40);place(m_reset,240,height-52,180,40);place(m_legacy,432,height-52,width-456,40);
            m_updateInfo.ShowWindow(m_page==0?SW_SHOW:SW_HIDE);
            if(m_page==0)place(m_updateInfo,24,y+34,width-48,88);
            UpdateInfo(); Invalidate(FALSE);
        }
        void Editor::UpdateInfo()
        {
            if(!m_updateInfo.GetSafeHwnd())return;
            const auto status=NativeUpdate::GetStatus(); CString phase;
            switch(status.phase)
            {
            case NativeUpdate::Phase::Disabled:phase=T(L"Disabled",L"已停用");break;
            case NativeUpdate::Phase::Idle:phase=T(L"Waiting for next check",L"等待下次檢查");break;
            case NativeUpdate::Phase::Checking:phase=T(L"Checking",L"檢查中");break;
            case NativeUpdate::Phase::Downloading:phase=T(L"Downloading",L"下載中");break;
            case NativeUpdate::Phase::Ready:phase=T(L"Ready; restart deferred while Settings is open",L"已就緒；設定開啟期間延後重新啟動");break;
            case NativeUpdate::Phase::Applying:phase=T(L"Applying",L"套用中");break;
            case NativeUpdate::Phase::Failed:phase.Format(T(L"Update unavailable (code %lu)",L"更新暫不可用（代碼 %lu）"),status.error);break;
            default:phase=T(L"Stopped",L"已停止");break;
            }
            CString text=T(L"Silent checks run every five minutes when enabled. Turning off automatic restart keeps staging enabled; pending updates apply on the next launch. Current service: ",
                L"啟用後每五分鐘靜默檢查。關閉自動重新啟動仍可準備更新；待處理更新會在下次啟動套用。目前狀態：");
            text+=phase; m_updateInfo.SetWindowText(text);
        }
        void Editor::ApplyDraft()
        {
            CString input;m_interval.GetWindowText(input); wchar_t* end{};long value=wcstol(input,&end,10);
            if(input.IsEmpty()||!end||*end||value<MONITOR_TIME_SPAN_MIN||value>MONITOR_TIME_SPAN_MAX||value%100)
            {Note(T(L"Enter an interval from 200 to 30000 ms in steps of 100. Nothing was applied.",L"請輸入 200 至 30000 毫秒，並以 100 為倍數。尚未套用任何更改。"));m_page=0;m_search.SetWindowText(L"");Layout();m_interval.SetFocus();return;}
            m_values.interval=static_cast<int>(value);
            if(m_values.automaticRestart != NativeUpdate::AutomaticRestartEnabled()
                && !NativeUpdate::SaveAutomaticRestart(m_values.automaticRestart))
            {
                m_dirty=true;
                Note(T(L"Automatic restart preference could not be saved and read back. Draft retained; other settings were not applied. Retry Apply or Revert.",L"未能儲存並讀回自動重新啟動偏好。草稿已保留，其餘設定未套用。請重試套用或還原。"));
                return;
            }
            m_apply(m_values); Reload();
            Note(T(L"Applied through the existing settings consumer. A required recovery tray icon stays visible.",L"已經原有設定流程套用。復原操作所需嘅通知區域圖示會保持顯示。"));
        }
        void Editor::ResetPage()
        {
            const Values defaults;
            for(auto& entry:m_entries)if(entry.page==m_page){m_values.*entry.member=defaults.*entry.member;entry.control->SetCheck(m_values.*entry.member?BST_CHECKED:BST_UNCHECKED);entry.control->Invalidate();}
            if(m_page==0)m_interval.SetWindowText(L"1000");m_dirty=true;
            Note(T(L"Defaults staged for the whole current group, including filtered controls. Apply to save, or Revert.",L"已將本組所有控制項（包括搜尋隱藏項目）重設為預設草稿。套用以儲存，或還原。"));
        }
        bool Editor::CanDiscard()
        {
            return !m_dirty||MessageBox(T(L"Discard unapplied settings changes?",L"捨棄尚未套用嘅設定更改？"),T(L"Settings",L"設定"),MB_YESNO|MB_ICONQUESTION|MB_DEFBUTTON2)==IDYES;
        }
        void Editor::OnCancel(){if(CanDiscard()){m_regexEditor.Close();EndDialog(IDCANCEL);}}
        BOOL Editor::OnCommand(WPARAM w,LPARAM l)
        {
            const UINT id=LOWORD(w),code=HIWORD(w);
            if(m_loading)return CDialog::OnCommand(w,l);
            if(id==Search&&code==EN_CHANGE){m_regexEditor.Close();Layout();return TRUE;}
            if(id==Interval&&code==EN_CHANGE){m_dirty=true;return TRUE;}
            if(code==BN_CLICKED&&id>=FirstCheck&&id<FirstCheck+m_entries.size())
            {auto& entry=m_entries[id-FirstCheck];m_values.*entry.member=entry.control->GetCheck()==BST_CHECKED;m_dirty=true;entry.control->Invalidate();Note(T(L"Unapplied changes. Apply saves, Revert reloads current values.",L"有未套用更改。套用會儲存，還原會重新載入目前數值。"));return TRUE;}
            if(code!=BN_CLICKED)return CDialog::OnCommand(w,l);
            switch(id)
            {
            case PageMonitor:case PageFloating:case PageTaskbar:m_regexEditor.Close();m_page=id-PageMonitor;Layout();return TRUE;
            case Regex:
                m_regexEditor.Open(m_hWnd,m_search.GetSafeHwnd(),m_filter.options,[this]{return Subjects();},
                    [this](const NativeRegex::Options& options){m_filter.options=options;m_search.SetWindowText(options.query.c_str());Layout();},m_chinese,m_appearance);return TRUE;
            case Clear:m_filter.options.regex=false;m_search.SetWindowText(L"");m_search.SetFocus();return TRUE;
            case Apply:ApplyDraft();return TRUE;
            case Revert:Reload();return TRUE;
            case Reset:ResetPage();return TRUE;
            case Legacy:if(CanDiscard()){m_regexEditor.Close();EndDialog(IDRETRY);}return TRUE;
            case Theme:m_appearance=m_appearance==NativeMaterial::Appearance::Light?NativeMaterial::Appearance::Dark:NativeMaterial::Appearance::Light;SetTheme();return TRUE;
            default:return CDialog::OnCommand(w,l);
            }
        }
        BOOL Editor::PreTranslateMessage(MSG* msg)
        {
            if(msg->message==WM_KEYDOWN&&msg->wParam==VK_ESCAPE)
            {
                HIMC context=ImmGetContext(msg->hwnd);bool composing=context&&ImmGetCompositionStringW(context,GCS_COMPSTR,nullptr,0)>0;
                if(context)ImmReleaseContext(msg->hwnd,context);if(composing)return FALSE;
                CString query;m_search.GetWindowText(query);if(::GetFocus()==m_search.GetSafeHwnd()&&!query.IsEmpty()){m_search.SetWindowText(L"");return TRUE;}
            }
            if(msg->message==WM_KEYDOWN&&msg->wParam=='L'&&(GetKeyState(VK_CONTROL)&0x8000)){m_search.SetFocus();return TRUE;}
            return CDialog::PreTranslateMessage(msg);
        }
        void Editor::OnPaint(){CPaintDC dc(this);CRect rect;GetClientRect(rect);dc.FillRect(rect,&m_background);}
        HBRUSH Editor::OnCtlColor(CDC* dc,CWnd*,UINT)
        {
            const auto c=NativeMaterial::Palette::Resolve(m_appearance);
            dc->SetTextColor(RGB(BYTE(c.onSurface.r*255),BYTE(c.onSurface.g*255),BYTE(c.onSurface.b*255)));
            dc->SetBkColor(RGB(BYTE(c.surface.r*255),BYTE(c.surface.g*255),BYTE(c.surface.b*255)));return static_cast<HBRUSH>(m_background.GetSafeHandle());
        }
        void Editor::OnGetMinMaxInfo(MINMAXINFO* info)
        {CDialog::OnGetMinMaxInfo(info);RECT r{0,0,Px(760),Px(640)};AdjustWindowRectEx(&r,GetStyle(),FALSE,GetExStyle());info->ptMinTrackSize={r.right-r.left,r.bottom-r.top};}
        LRESULT Editor::OnDpi(WPARAM dpi,LPARAM value)
        {m_dpi=HIWORD(dpi);const auto* r=reinterpret_cast<const RECT*>(value);SetWindowPos(nullptr,r->left,r->top,r->right-r->left,r->bottom-r->top,SWP_NOZORDER|SWP_NOACTIVATE);SetFonts();Layout();return 0;}
    }
    bool IsSafeToRestart() noexcept { return activeEditors == 0; }
    bool Show(CWnd* owner,int initialPage,const std::function<void(const Values&)>& apply)
    {
        EditorLifetime lifetime;
        struct Template { DLGTEMPLATE dialog; WORD menu{},className{},title{}; } layout{};
        layout.dialog.style=WS_POPUP|WS_CAPTION|WS_SYSMENU|WS_THICKFRAME|WS_CLIPCHILDREN|DS_MODALFRAME;
        layout.dialog.dwExtendedStyle=WS_EX_CONTROLPARENT;layout.dialog.cx=500;layout.dialog.cy=400;
        Editor editor(initialPage,apply);if(!editor.InitModalIndirect(&layout.dialog,owner))return true;
        const INT_PTR result=editor.DoModal();return result==IDRETRY||result==-1;
    }
}
