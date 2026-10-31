#include "../stdafx.h"
#include "../NativeMaterial/ResponsivenessDiagnostics.h"
#include "Workspace.h"
#include "../NativePreferences/Text.h"
#include "../NativePreferences/Subscription.h"
#include "../NativeSettings/Settings.h"
#include "ViewModel.h"
#include "Presentation.h"
#include "SetupFlow.h"
#include "../NativeDashboardController.h"
#include "../NativeMaterial/Controls.h"
#include "../NativeMaterial/AlertDialog.h"
#include "../NativeMaterial/Label.h"
#include "../NativeMaterial/TextField.h"
#include "../NativeMaterial/ScrollableTextField.h"
#include "../NativeMaterial/Navigation.h"
#include "../NativeMaterial/ScrollViewport.h"
#include "../NativeMaterial/Panel.h"
#include "../NativeMaterial/TitleBar.h"
#include "../NativeMenus/SearchableMenu.h"
#include "../NativeRegex/Workbench.h"
#include "../TrafficMonitor.h"
#include <afxcmn.h>
#include <imm.h>
#include <map>
#include <array>
#include <sstream>

namespace NativeDashboard
{
    namespace
    {
        constexpr UINT SnapshotMessage = WM_APP + 0x452;
        enum ControlId : UINT { HistoryId = 52001, PluginsId, SettingsId, FloatingId, ThemeId,
            PauseId, RefreshId, SearchId, ClearId, TabsId, AdaptersId, SamplesId, DetailsId, StatusId, SummaryId, RegexId, MonitorId, SetupId, SamplesPageId, DetailsPageId };
        class Workspace;
        std::map<HWND, Workspace*> windows;
        COLORREF Color(const D2D1_COLOR_F& value)
        {
            return RGB(static_cast<BYTE>(value.r * 255), static_cast<BYTE>(value.g * 255), static_cast<BYTE>(value.b * 255));
        }
        class Workspace final : public CWnd
        {
        public:
            Workspace(HWND owner, DWORD interval, std::function<void()> callback)
                : m_owner(owner), m_interval((std::max)(100UL, (std::min)(60000UL, interval))), m_floating(std::move(callback)) {}
            bool CreateWorkspace();
            void Adopt() { m_autoDelete = true; }
        protected:
            afx_msg int OnCreate(LPCREATESTRUCT);
            afx_msg void OnPaint();
            afx_msg BOOL OnEraseBkgnd(CDC*) { return TRUE; }
            afx_msg void OnSize(UINT type, int width, int height);
            afx_msg void OnGetMinMaxInfo(MINMAXINFO* info);
            afx_msg void OnVScroll(UINT code,UINT,CScrollBar*)
            {SCROLLINFO info{sizeof(info),SIF_TRACKPOS};GetScrollInfo(SB_VERT,&info);if(code==SB_LINEUP)m_scroll-=32;else if(code==SB_LINEDOWN)m_scroll+=32;else if(code==SB_PAGEUP)m_scroll-=200;else if(code==SB_PAGEDOWN)m_scroll+=200;else if(code==SB_THUMBTRACK||code==SB_THUMBPOSITION)m_scroll=info.nTrackPos;Layout();}
            afx_msg BOOL OnMouseWheel(UINT,short delta,CPoint){m_scroll-=delta/3;Layout();return TRUE;}
            afx_msg void OnClose();
            afx_msg void OnDestroy();
            afx_msg void OnTimer(UINT_PTR id) { if(id==0x4E92 && m_model.PollFilter()) RefreshRows(); else CWnd::OnTimer(id); }
            afx_msg LRESULT OnSnapshot(WPARAM generation, LPARAM);
            afx_msg LRESULT OnDpi(WPARAM dpi, LPARAM bounds);
            afx_msg HBRUSH OnCtlColor(CDC* dc, CWnd*, UINT);
            afx_msg void OnContextMenu(CWnd* source, CPoint point);
            BOOL OnCommand(WPARAM wparam, LPARAM lparam) override;
            BOOL PreTranslateMessage(MSG* message) override;
            void PostNcDestroy() override;
            DECLARE_MESSAGE_MAP()
        private:
            CString T(const wchar_t* english, const wchar_t* chinese) const { return NativePreferences::Pair(english,chinese); }
            NativePreferences::Subscription m_preferences;
            NativeMaterial::TitleBar m_titleBar;
            NativeMaterial::TitleBar::Labels Captions() const{NativeMaterial::TitleBar::Labels l;l.minimize=T(L"Minimize",L"最小化");l.maximize=T(L"Maximize",L"最大化");l.restore=T(L"Restore",L"還原");l.close=T(L"Close",L"關閉");return l;}
            LRESULT OnPreferences(WPARAM,LPARAM);
            void Labels();
            LRESULT WindowProc(UINT message,WPARAM w,LPARAM l) override {if(message==WM_SETTINGCHANGE||message==WM_THEMECHANGED){NativeDiagnostics::Scope messageDiagnostic(message==WM_SETTINGCHANGE?NativeDiagnostics::Operation::WorkspaceSettingMessage:NativeDiagnostics::Operation::WorkspaceThemeMessage);NativePreferences::RefreshSystemAppearance();}return CWnd::WindowProc(message,w,l);}
            int Px(int value) const { return MulDiv(value, m_dpi, 96); }
            void Layout();void RevealFocus(HWND);
            int m_scroll{},m_contentHeight{},m_metricsTop{},m_chartTop{},m_chartBottom{};bool m_layout{};
            void ApplyTheme();
            void SetFonts();
            void RefreshRows();
            void RefreshDetails();
            void Status();
            void Start();
            void ShowPage();
            HWND m_owner{};
            DWORD m_interval{};
            UINT m_dpi{ 96 };
            bool m_chinese{}, m_autoDelete{}, m_rendering{};
            HRESULT m_startResult{ S_OK };
            std::function<void()> m_floating;
            NativeDashboardController m_controller;
            ViewModel m_model;
            NativeMaterial::Canvas m_canvas;
            NativeMaterial::Appearance m_appearance{ NativeMaterial::Appearance::Light };
            CFont m_font;
            CBrush m_background, m_navigationFill;
            NativeMaterial::Button m_history, m_plugins, m_settings, m_floatButton, m_theme, m_pause, m_refresh, m_clear, m_regex;
            NativeMaterial::TextField m_search;
            NativeMaterial::ScrollableTextField m_details;
            NativeMaterial::ScrollViewport m_viewport;
            NativeMaterial::Panel m_panel;
            DataView m_data;
            NativeMaterial::Button m_monitor, m_setup;
            std::array<NativeMaterial::Button,3> m_pages;
            int m_page{},m_bodyTop{},m_bodyHeight{};
            NativeMaterial::Label m_heading,m_brand;
            CFont m_headingFont;
            NativeRegex::Editor m_regexEditor;
            NativeMaterial::Label m_status, m_summary;
            NativeMaterial::ResultList m_adapters, m_samples;
        };
        BEGIN_MESSAGE_MAP(Workspace, CWnd)
            ON_WM_CREATE()
            ON_WM_TIMER()
            ON_WM_PAINT()
            ON_WM_ERASEBKGND()
            ON_WM_SIZE()
            ON_WM_GETMINMAXINFO()
            ON_WM_CLOSE()
            ON_WM_VSCROLL()
            ON_WM_MOUSEWHEEL()
            ON_WM_DESTROY()
            ON_WM_CTLCOLOR()
            ON_WM_CONTEXTMENU()
            ON_MESSAGE(SnapshotMessage, &Workspace::OnSnapshot)
            ON_MESSAGE(WM_DPICHANGED, &Workspace::OnDpi)
            ON_MESSAGE(NativePreferences::ChangedMessage,&Workspace::OnPreferences)
        END_MESSAGE_MAP()

        bool Workspace::CreateWorkspace()
        {
            m_chinese=NativePreferences::Current().values.language==NativePreferences::Language::Cantonese;
            m_appearance=NativePreferences::EffectiveAppearance();
            const CString className = AfxRegisterWndClass(CS_HREDRAW | CS_VREDRAW, ::LoadCursor(nullptr, IDC_ARROW), nullptr, AfxGetApp()->LoadIcon(IDR_MAINFRAME));
            MONITORINFO monitor{ sizeof(monitor) };
            GetMonitorInfoW(MonitorFromWindow(m_owner, MONITOR_DEFAULTTONEAREST), &monitor);
            m_dpi = GetDpiForWindow(m_owner);
            if (!m_dpi) m_dpi = 96;
            const int width = (std::min)(Px(1296), int(monitor.rcWork.right - monitor.rcWork.left));
            const int height = (std::min)(Px(879), int(monitor.rcWork.bottom - monitor.rcWork.top));
            const int x = monitor.rcWork.left + (monitor.rcWork.right - monitor.rcWork.left - width) / 2;
            const int y = monitor.rcWork.top + (monitor.rcWork.bottom - monitor.rcWork.top - height) / 2;
            return CreateEx(WS_EX_CONTROLPARENT, className, T(L"TrafficMonitor - Network workspace", L"TrafficMonitor - 網絡工作區"),
                WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN, x, y, width, height, m_owner, nullptr) != FALSE;
        }
        int Workspace::OnCreate(LPCREATESTRUCT create)
        {
            if (CWnd::OnCreate(create) == -1) return -1;
            m_dpi = GetDpiForWindow(m_hWnd); if (!m_dpi) m_dpi = 96;
            const CRect initial(0, 0, 100, 40);
            if (!m_viewport.Create(this, 52100) || !m_panel.Create(&m_viewport, 52101)
                || !m_data.Create(&m_viewport, 52102)) return -1;
            m_viewport.background = [this] { return static_cast<HBRUSH>(m_background.GetSafeHandle()); };
            m_viewport.scroll = [this](UINT code, int value) {
                if (code == SB_THUMBTRACK) m_scroll = value;
                else if (code == SB_LINEUP) m_scroll -= Px(32);
                else if (code == SB_LINEDOWN) m_scroll += Px(32);
                else if (code == SB_PAGEUP) m_scroll -= m_bodyHeight;
                else if (code == SB_PAGEDOWN) m_scroll += m_bodyHeight;
                Layout();
            };
            struct Action { NativeMaterial::Button* button; UINT id; };
            for (const auto& action : std::array<Action,11>{{
                {&m_monitor,MonitorId},{&m_history,HistoryId},{&m_plugins,PluginsId},
                {&m_settings,SettingsId},{&m_setup,SetupId},{&m_theme,ThemeId},
                {&m_floatButton,FloatingId},{&m_pause,PauseId},{&m_refresh,RefreshId},
                {&m_clear,ClearId},{&m_regex,RegexId}}})
                if (!action.button->Create(L"", initial, this, action.id)) return -1;
            for (int i=0;i<3;++i)
                if (!m_pages[i].Create(L"",initial,&m_viewport,i==0?TabsId:(i==1?SamplesPageId:DetailsPageId))) return -1;
            const DWORD child=WS_CHILD|WS_VISIBLE|WS_TABSTOP;
            if (!m_search.Create(child|ES_AUTOHSCROLL,initial,this,SearchId)
                || !m_adapters.Create(initial,&m_panel,AdaptersId)
                || !m_samples.Create(initial,&m_panel,SamplesId)
                || !m_details.Create(child|ES_MULTILINE|ES_READONLY|ES_AUTOVSCROLL|WS_CLIPCHILDREN,initial,&m_panel,DetailsId)
                || !m_status.Create(L"",WS_CHILD|WS_VISIBLE,initial,this,StatusId)
                || !m_summary.Create(L"",WS_CHILD|WS_VISIBLE,initial,&m_viewport,SummaryId)
                || !m_heading.Create(L"",WS_CHILD|WS_VISIBLE,initial,this,52103)
                || !m_brand.Create(L"TrafficMonitor",WS_CHILD|WS_VISIBLE,initial,this,52104)) return -1;
            m_search.SetLimitText(512);
            m_details.SetLimitText(65535);
            m_preferences.Attach(m_hWnd);
            m_titleBar.Attach(this,m_appearance,Captions());
            {
                MONITORINFO monitor{ sizeof(monitor) };GetMonitorInfoW(MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTONEAREST), &monitor);
                const int workWidth=monitor.rcWork.right-monitor.rcWork.left,workHeight=monitor.rcWork.bottom-monitor.rcWork.top;
                SIZE outer=m_titleBar.OuterSizeForContent(Px(1296),Px(879));
                outer.cx=(std::min)(outer.cx,static_cast<LONG>(workWidth));outer.cy=(std::min)(outer.cy,static_cast<LONG>(workHeight));
                SetWindowPos(nullptr,monitor.rcWork.left+(workWidth-outer.cx)/2,monitor.rcWork.top+(workHeight-outer.cy)/2,outer.cx,outer.cy,SWP_NOZORDER|SWP_NOACTIVATE);
            }            Labels(); SetFonts(); ApplyTheme(); Layout(); Start(); SetTimer(0x4E92,40,nullptr);
            return 0;
        }
        void Workspace::Labels()
        {
            SetWindowText(T(L"TrafficMonitor - Network workspace",L"TrafficMonitor - 網絡工作區"));m_titleBar.SetLabels(Captions());
            m_heading.SetWindowText(T(L"Network overview",L"網絡總覽"));
            m_monitor.SetLabel(T(L"Monitor",L"監察"));
            m_history.SetLabel(T(L"History",L"流量記錄"));
            m_plugins.SetLabel(T(L"Plugins",L"外掛程式"));
            m_settings.SetLabel(T(L"Settings",L"設定"));
            m_setup.SetLabel(T(L"Setup",L"初始設定"));
            m_floatButton.SetLabel(T(L"Toggle floating monitor",L"顯示／隱藏浮動監察器"));
            m_theme.SetLabel(NativePreferences::Text(NativePreferences::TextId::Interface));
            m_pause.SetLabel(m_model.Paused()?T(L"Resume display",L"恢復顯示"):T(L"Pause display",L"暫停顯示"));
            m_refresh.SetLabel(T(L"Restart samples",L"重新採樣"));
            m_regex.SetLabel(T(L"Regex",L"正規式")); m_clear.SetLabel(T(L"Clear",L"清除"));
            // A single-line edit uses the primary language cue; its accessible name is complete.
            CString cue=m_chinese?L"搜尋網絡介面名稱或描述":L"Search adapter name or description";
            m_search.SendMessage(EM_SETCUEBANNER,TRUE,reinterpret_cast<LPARAM>(cue.GetString()));
            m_adapters.SetWindowText(T(L"Observed network adapters",L"已觀察網絡介面"));
            m_samples.SetWindowText(T(L"Selected adapter sample history",L"所選網絡介面採樣記錄"));
            m_viewport.SetWindowText(T(L"Monitoring data",L"監察資料"));
            m_viewport.SetScrollName(T(L"Scroll monitoring data",L"捲動監察資料"));
            const wchar_t* en[]={L"Adapters",L"Sample history",L"Details"};
            const wchar_t* zh[]={L"網絡介面",L"採樣記錄",L"詳細資料"};
            for(int i=0;i<3;++i)m_pages[i].SetLabel(T(en[i],zh[i]));
        }
        LRESULT Workspace::OnPreferences(WPARAM generation,LPARAM)
        {if(!m_preferences.Accepts(generation))return 0;NativeDiagnostics::Scope diagnostic(NativeDiagnostics::Operation::Receive);m_chinese=NativePreferences::Current().values.language==NativePreferences::Language::Cantonese;m_appearance=NativePreferences::EffectiveAppearance();Labels();ApplyTheme();Layout();RefreshRows();return 0;}
        void Workspace::SetFonts()
        {
            LOGFONT font{};font.lfHeight=-Px(14);font.lfCharSet=DEFAULT_CHARSET;
            wcscpy_s(font.lfFaceName,L"Segoe UI");
            HFONT replacement=CreateFontIndirectW(&font);
            if(!replacement)return;
            HFONT previous=static_cast<HFONT>(m_font.Detach());m_font.Attach(replacement);
            EnumChildWindows(m_hWnd,[](HWND child,LPARAM value)->BOOL {
                ::SendMessageW(child,WM_SETFONT,value,TRUE);return TRUE;
            },reinterpret_cast<LPARAM>(replacement));
            if(previous)DeleteObject(previous);
            font.lfHeight=-Px(24);font.lfWeight=FW_SEMIBOLD;
            replacement=CreateFontIndirectW(&font);
            if(replacement){previous=static_cast<HFONT>(m_headingFont.Detach());m_headingFont.Attach(replacement);m_heading.SetFont(&m_headingFont);if(previous)DeleteObject(previous);}
        }
        void Workspace::ApplyTheme()
        {
            const auto palette=NativeMaterial::Palette::Resolve(m_appearance);
            m_background.DeleteObject();m_background.CreateSolidBrush(Color(palette.surface));
            m_navigationFill.DeleteObject();m_navigationFill.CreateSolidBrush(Color(palette.container));
            NativeMaterial::ApplyNativeFrameAppearance(m_hWnd,m_appearance);m_titleBar.SetAppearance(m_appearance);
            for(auto* button:{&m_monitor,&m_history,&m_plugins,&m_settings,&m_setup,&m_floatButton,&m_theme,&m_pause,&m_refresh,&m_clear,&m_regex})
            {
                button->SetAppearance(m_appearance);
                button->SetVariant(NativeMaterial::ButtonVariant::Text);
            }
            for(auto* button:{&m_monitor,&m_history,&m_plugins,&m_settings,&m_setup,&m_theme})button->SetSurfaceColor(Color(palette.container));
            m_monitor.SetVariant(NativeMaterial::ButtonVariant::Tonal);
            m_pause.SetVariant(NativeMaterial::ButtonVariant::Tonal);
            m_regex.SetVariant(NativeMaterial::ButtonVariant::Tonal);
            for(auto* label:{&m_heading,&m_brand,&m_status,&m_summary})label->SetAppearance(m_appearance);
            m_brand.SetSurfaceColor(Color(palette.container));
            m_search.SetAppearance(m_appearance);m_details.SetAppearance(m_appearance);
            m_adapters.SetAppearance(m_appearance);m_samples.SetAppearance(m_appearance);
            m_viewport.SetAppearance(m_appearance);m_panel.SetAppearance(m_appearance);
            m_data.SetAppearance(m_appearance);
            for(auto& button:m_pages)button.SetAppearance(m_appearance);
            ShowPage();RedrawWindow(nullptr,nullptr,RDW_INVALIDATE|RDW_ALLCHILDREN);
        }
        void Workspace::Layout()
        {
            NativeDiagnostics::Scope diagnostic(NativeDiagnostics::Operation::Layout);
            if(!m_search.GetSafeHwnd()||m_layout)return;
            m_layout=true;
            CRect client;GetClientRect(client);const int bar=m_titleBar.HeightPx();
            const int nav=Px(176),inset=Px(24),left=nav+inset,right=client.right-inset;
            const int width=(std::max)(Px(200),right-left);
            CClientDC dc(this);
            auto measure=[&](CWnd& control,int available)->int{
                if(auto* label=dynamic_cast<NativeMaterial::Label*>(&control))return label->MeasureHeight((std::max)(1,available));
                CString text;control.GetWindowText(text);auto* font=control.GetFont();auto* old=font?dc.SelectObject(font):nullptr;
                CRect r(0,0,(std::max)(1,available),0);dc.DrawText(text,r,DT_CALCRECT|DT_WORDBREAK|DT_NOPREFIX);
                if(old)dc.SelectObject(old);return static_cast<int>(r.Height());
            };
            auto place=[](CWnd& control,int x,int y,int w,int h){control.MoveWindow(x,y,(std::max)(1,w),(std::max)(1,h));};
            m_brand.MoveWindow(Px(16),bar+Px(24),nav-Px(28),Px(32));
            int y=bar+Px(64);
            for(auto* button:{&m_monitor,&m_history,&m_plugins,&m_settings,&m_setup,&m_theme})
            {
                int h=(std::max)(Px(40),measure(*button,nav-Px(52))+Px(18));
                place(*button,Px(12),y,nav-Px(24),h);y+=h+Px(8);
            }
            const int headingHeight=(std::max)(Px(32),measure(m_heading,width));
            place(m_heading,left,bar+Px(16),width,headingHeight);
            y=bar+Px(16)+headingHeight+Px(16);
            const int searchHeight=Px(44),regexWidth=Px(96),clearWidth=Px(72);
            place(m_search,left,y,width-regexWidth-clearWidth-Px(16),searchHeight);
            place(m_regex,right-regexWidth-clearWidth-Px(8),y,regexWidth,searchHeight);
            place(m_clear,right-clearWidth,y,clearWidth,searchHeight);
            y+=searchHeight+Px(12);
            int x=left,rowHeight=0;
            for(auto* button:{&m_pause,&m_refresh,&m_floatButton})
            {
                CString label;button->GetWindowText(label);const int newline=label.Find(L"\r\n");if(newline>=0)label=label.Left(newline);
                auto* old=dc.SelectObject(&m_font);const int natural=dc.GetTextExtent(label).cx+Px(32);dc.SelectObject(old);
                const int w=(std::min)(width,(std::max)(Px(132),natural));
                const int h=(std::max)(Px(38),measure(*button,w-Px(24))+Px(16));
                if(x>left&&x+w>right){y+=rowHeight+Px(6);x=left;rowHeight=0;}
                place(*button,x,y,w,h);x+=w+Px(8);rowHeight=(std::max)(rowHeight,h);
            }
            y+=rowHeight+Px(8);
            const int statusHeight=(std::max)(Px(20),measure(m_status,width));
            place(m_status,left,y,width,statusHeight);y+=statusHeight+Px(10);
            m_bodyTop=y;m_bodyHeight=(std::max)(Px(80),int(client.bottom)-y-Px(16));
            place(m_viewport,nav,y,client.right-nav,m_bodyHeight);
            const int innerWidth=width,innerLeft=inset;
            int contentY=0;
            const int summaryHeight=(std::max)(Px(24),measure(m_summary,innerWidth));
            contentY+=summaryHeight+Px(8);
            const int dataHeight=Px(264);
            const int dataY=contentY;contentY+=dataHeight+Px(12);
            int pageHeight=Px(40);
            for(auto& button:m_pages)pageHeight=(std::max)(pageHeight,measure(button,(innerWidth-Px(16))/3-Px(24))+Px(16));
            const int tabsY=contentY;contentY+=pageHeight+Px(10);
            const int listHeight=(std::max)(Px(220),m_bodyHeight-contentY-Px(24));
            const int panelY=contentY;contentY+=listHeight+Px(24);
            m_contentHeight=contentY;m_scroll=(std::clamp)(m_scroll,0,(std::max)(0,m_contentHeight-m_bodyHeight));
            place(m_summary,innerLeft,-m_scroll,innerWidth,summaryHeight);
            place(m_data,innerLeft,dataY-m_scroll,innerWidth,dataHeight);
            for(int i=0;i<3;++i)place(m_pages[i],innerLeft+i*(innerWidth+Px(8))/3,tabsY-m_scroll,(innerWidth-Px(16))/3,pageHeight);
            place(m_panel,innerLeft,panelY-m_scroll,innerWidth,listHeight);
            for(CWnd* control:std::array<CWnd*,3>{&m_adapters,&m_samples,&m_details})place(*control,Px(12),Px(12),innerWidth-Px(24),listHeight-Px(24));
            m_viewport.SetMetrics(m_contentHeight,m_bodyHeight,m_scroll);
            m_adapters.SyncScroll();m_samples.SyncScroll();m_details.SyncScroll();
            m_layout=false;Invalidate(FALSE);
        }
        void Workspace::RevealFocus(HWND target)
        {
            if(!::IsWindow(target)||!::IsChild(m_viewport,target))return;
            CRect r,view;::GetWindowRect(target,r);m_viewport.GetWindowRect(view);
            const int old=m_scroll;
            if(r.Height()>view.Height()||r.top<view.top)m_scroll+=r.top-view.top;
            else if(r.bottom>view.bottom)m_scroll+=r.bottom-view.bottom;
            if(old!=m_scroll)Layout();
        }
        void Workspace::ShowPage()
        {
            m_adapters.ShowWindow(m_page==0?SW_SHOW:SW_HIDE);
            m_samples.ShowWindow(m_page==1?SW_SHOW:SW_HIDE);
            m_details.ShowWindow(m_page==2?SW_SHOW:SW_HIDE);
            for(int i=0;i<3;++i)m_pages[i].SetVariant(i==m_page?NativeMaterial::ButtonVariant::Tonal:NativeMaterial::ButtonVariant::Text);
        }
        void Workspace::Start()
        {
            m_startResult = m_controller.Start(m_hWnd, SnapshotMessage, m_interval);
            Status();
        }
        void Workspace::Status()
        {
            CString status;
            if (FAILED(m_startResult)) status=NativePreferences::Format(L"Sampling could not start (0x%08X)",L"未能開始採樣 (0x%08X)", static_cast<unsigned>(m_startResult));
            else if (m_controller.WorkerError()) status=NativePreferences::Format(L"Sampling worker error: %lu",L"採樣工作錯誤：%lu", m_controller.WorkerError());
            else if (!m_model.Displayed()) status = T(L"Collecting first sample; restart affects display history only", L"正在收集首個樣本；重新採樣只影響顯示記錄");
            else
            {
                const auto& snapshot = *m_model.Displayed();
                if (snapshot.state == NativeMonitoring::SnapshotState::Unavailable)
                    status=NativePreferences::Format(L"Discovery unavailable (%u); last rows are stale",L"未能取得介面資料 (%u)；舊讀數已過時", snapshot.error);
                else if (snapshot.adapters.empty()) status = T(L"No adapters were discovered", L"未發現網絡介面");
                else if (m_model.Rows().empty()) status = T(L"No adapters match this search", L"沒有符合搜尋的網絡介面");
                else status=NativePreferences::Format(L"%zu adapters shown · observation %llu · %s",L"顯示 %zu 個介面 · 觀察 %llu · %s", m_model.Rows().size(), snapshot.sequence,
                    m_model.Paused() ? T(L"Display paused; collection continues", L"顯示已暫停；採樣繼續").GetString() : T(L"Live display", L"即時顯示").GetString());
            }
            if(!m_model.search.status.empty()) status+=L" | "+CString(m_model.search.status.c_str());
            m_status.SetWindowText(status);
            const auto* selected = m_model.Selected();
            std::wstring summary=T(L"Selected adapter: ",L"所選介面：").GetString();
            summary += selected ? selected->alias : (m_model.SelectedIdentity().empty() ? std::wstring(T(L"None selected",L"尚未選擇").GetString()) : std::wstring(T(L"No longer present",L"已不存在").GetString()));
            if (selected) summary += L" · " + TimeText(selected->acquiredAtUtcFileTime) + L" · " + RateStateText(selected->rateState, m_chinese);
            m_summary.SetWindowText(summary.c_str());
        }
        void Workspace::RefreshRows()
        {
            NativeDiagnostics::Scope diagnostic(NativeDiagnostics::Operation::RefreshRows);
            const int top=m_adapters.GetTopIndex();
            m_rendering=true;m_adapters.SetRedraw(FALSE);m_adapters.ResetContent();
            for(std::size_t i=0;i<m_model.Rows().size();++i)
            {
                const auto* row=m_model.Row(i);CString interval;interval.Format(L"%.2f s",row->intervalSeconds);
                CString text(row->alias.c_str());text+=L"\r\n";
                text+=RateStateText(row->rateState,m_chinese).c_str();text+=L"  ·  ";
                text+=NativePreferences::InlinePair(L"Receive: ",L"接收：");text+=Rate(row->receivedBytesPerSecond,m_chinese).c_str();
                text+=L"  ·  ";text+=NativePreferences::InlinePair(L"Send: ",L"傳送：");text+=Rate(row->sentBytesPerSecond,m_chinese).c_str();
                text+=L"\r\n";text+=NativePreferences::InlinePair(L"Interval: ",L"間隔：");text+=interval;
                m_adapters.AddString(text);
            }
            m_adapters.SetCurSel(m_model.SelectedRow());
            if(top>=0&&top<m_adapters.GetCount())m_adapters.SetTopIndex(top);
            m_adapters.SetRedraw(TRUE);m_adapters.Invalidate();m_rendering=false;
            RefreshDetails();Status();Layout();
        }
        void Workspace::RefreshDetails()
        {
            NativeDiagnostics::Scope diagnostic(NativeDiagnostics::Operation::RefreshDetails);
            const auto* row=m_model.Selected();
            const CString details(Details(row,!m_model.SelectedIdentity().empty(),m_chinese).c_str());
            CString previous;m_details.GetWindowText(previous);
            if(previous!=details){int first,last;m_details.GetSel(first,last);const int top=m_details.GetFirstVisibleLine();m_details.SetWindowText(details);m_details.SetSel(first,last,TRUE);m_details.LineScroll(top-m_details.GetFirstVisibleLine());m_details.SyncScroll();}
            const int top=m_samples.GetTopIndex(),selected=m_samples.GetCurSel();
            m_samples.SetRedraw(FALSE);m_samples.ResetContent();
            if(row&&!row->history.empty())
            {
                const auto latest=row->history.back().monotonicMilliseconds;
                for(auto it=row->history.rbegin();it!=row->history.rend();++it)
                {
                    CString text;text.Format(L"-%.2f s",double(latest-it->monotonicMilliseconds)/1000);
                    text+=L"  ·  ";text+=RateStateText(it->state,m_chinese).c_str();
                    text+=L"\r\n";text+=NativePreferences::InlinePair(L"Receive: ",L"接收：");text+=Rate(it->receivedBytesPerSecond,m_chinese).c_str();
                    text+=L"  ·  ";text+=NativePreferences::InlinePair(L"Send: ",L"傳送：");text+=Rate(it->sentBytesPerSecond,m_chinese).c_str();
                    m_samples.AddString(text);
                }
            }
            if(selected>=0&&selected<m_samples.GetCount())m_samples.SetCurSel(selected);
            if(top>=0&&top<m_samples.GetCount())m_samples.SetTopIndex(top);
            m_samples.SetRedraw(TRUE);m_samples.Invalidate();m_data.SetSnapshot(row,m_chinese);
        }
        LRESULT Workspace::OnSnapshot(WPARAM generation, LPARAM)
        {
            if (auto snapshot = m_controller.Take(generation))
            {
                m_model.Receive(std::move(snapshot));
                if (!m_model.Paused()) RefreshRows();
                else Status();
            }
            else Status();
            return 0;
        }

        BOOL Workspace::OnCommand(WPARAM wparam, LPARAM lparam)
        {
            const UINT id = LOWORD(wparam);
            if(id==AdaptersId&&HIWORD(wparam)==LBN_SELCHANGE)
            {
                const int selected=m_adapters.GetCurSel();
                if(!m_rendering&&selected>=0){m_model.Select(selected);RefreshDetails();Status();Layout();}
                return TRUE;
            }
            if (id == SearchId && HIWORD(wparam) == EN_CHANGE)
            { m_regexEditor.Close(); CString query; m_search.GetWindowText(query); m_model.Filter(query.GetString()); RefreshRows(); return TRUE; }
            if (HIWORD(wparam) != BN_CLICKED) return CWnd::OnCommand(wparam, lparam);
            switch (id)
            {
            case MonitorId: m_page=0;ShowPage();Layout();return TRUE;
            case SetupId: if(::IsWindow(m_owner))Setup::Show(CWnd::FromHandle(m_owner),true);return TRUE;
            case TabsId: case SamplesPageId: case DetailsPageId:
                m_page=id==TabsId?0:(id==SamplesPageId?1:2);ShowPage();Layout();return TRUE;
            case HistoryId: case PluginsId: case SettingsId:
                if (::IsWindow(m_owner)) ::PostMessageW(m_owner, WM_COMMAND, id == HistoryId ? ID_TRAFFIC_HISTORY : (id == PluginsId ? ID_PLUGIN_MANAGE : ID_OPTIONS), 0);
                return TRUE;
            case FloatingId: if (::IsWindow(m_owner) && m_floating) m_floating(); return TRUE;
            case ThemeId: if(::IsWindow(m_owner))NativeSettings::OpenInterface(CWnd::FromHandle(m_owner));return TRUE;
            case PauseId:
                m_model.SetPaused(!m_model.Paused()); m_pause.SetLabel(m_model.Paused() ? T(L"Resume display", L"恢復顯示") : T(L"Pause display", L"暫停顯示")); RefreshRows(); return TRUE;
            case RefreshId:
                m_controller.Stop(); m_model.Reset(); m_pause.SetLabel(T(L"Pause display", L"暫停顯示")); RefreshRows(); Start(); return TRUE;
            case RegexId:
                m_regexEditor.Open(m_hWnd,m_search.GetSafeHwnd(),m_model.search.options,[this]{return m_model.SearchSubjects();},
                    [this](const NativeRegex::Options& options){m_model.search.options=options;m_search.SetWindowText(options.query.c_str());m_model.Refilter();},m_chinese,m_appearance);
                return TRUE;
            case ClearId: m_model.search.options.regex=false; m_search.SetWindowText(L""); m_search.SetFocus(); return TRUE;
            default: return CWnd::OnCommand(wparam, lparam);
            }
        }
        void Workspace::OnContextMenu(CWnd*, CPoint point)
        {
            if (point.x == -1 && point.y == -1) { point = CPoint(Px(32), Px(440)); ClientToScreen(&point); }
            CMenu menu; menu.CreatePopupMenu();
            menu.AppendMenu(MF_STRING | (m_model.Selected() ? MF_ENABLED : MF_GRAYED), 1, T(L"Show adapter details", L"顯示網絡介面詳情"));
            menu.AppendMenu(MF_STRING, 2, m_model.Paused() ? T(L"Resume display", L"恢復顯示") : T(L"Pause display", L"暫停顯示"));
            menu.AppendMenu(MF_STRING, 3, T(L"Restart observational samples", L"重新收集觀察樣本"));
            menu.AppendMenu(MF_STRING, 4, T(L"Resume setup", L"繼續設定"));
            const UINT command = NativeMenus::Track(&menu, TPM_RETURNCMD | TPM_RIGHTBUTTON, point.x, point.y, this);
            if (command == 1 && m_model.Selected()) { m_page=2; ShowPage();Layout(); m_details.SetFocus(); }
            else if (command == 2) OnCommand(PauseId, 0);
            else if (command == 3) OnCommand(RefreshId, 0);
            else if (command == 4 && ::IsWindow(m_owner)) Setup::Show(CWnd::FromHandle(m_owner), true);
        }
        BOOL Workspace::PreTranslateMessage(MSG* message)
        {
            if (message->message == WM_KEYDOWN)
            {
                if (message->wParam == VK_TAB)
                {
                    HWND next = ::GetNextDlgTabItem(m_hWnd, ::GetFocus(), (GetKeyState(VK_SHIFT) & 0x8000) != 0);
                    if (next){::SetFocus(next);RevealFocus(next);}
                    return TRUE;
                }
                if (message->wParam == VK_F6) { m_pages[m_page].SetFocus();RevealFocus(m_pages[m_page]); return TRUE; }
                if (message->wParam == 'L' && (GetKeyState(VK_CONTROL) & 0x8000)) { m_search.SetFocus();RevealFocus(m_search); return TRUE; }
                if (message->wParam == VK_ESCAPE)
                {
                    HIMC context = ImmGetContext(message->hwnd);
                    const bool composing = context && ImmGetCompositionStringW(context, GCS_COMPSTR, nullptr, 0) > 0;
                    if (context) ImmReleaseContext(message->hwnd, context);
                    if (composing) return CWnd::PreTranslateMessage(message);
                    CString query; m_search.GetWindowText(query);
                    if (::GetFocus() == m_search.GetSafeHwnd() && !query.IsEmpty()) m_search.SetWindowText(L"");
                    else if (m_page != 0) { m_page=0; ShowPage();Layout(); m_adapters.SetFocus(); }
                    else OnClose();
                    return TRUE;
                }
            }
            return CWnd::PreTranslateMessage(message);
        }
        void Workspace::OnPaint()
        {
            CPaintDC dc(this);CRect r;GetClientRect(r);
            const auto colors=NativeMaterial::Palette::Resolve(m_appearance);
            dc.FillSolidRect(r,Color(colors.surface));
            dc.FillSolidRect(0,m_titleBar.HeightPx(),Px(176),r.Height()-m_titleBar.HeightPx(),Color(colors.surfaceContainer));
        }
        void Workspace::OnSize(UINT type, int width, int height)
        {
            CWnd::OnSize(type, width, height);
            if (type != SIZE_MINIMIZED) { m_canvas.Resize(width, height, static_cast<float>(m_dpi)); Layout(); }
        }
        void Workspace::OnGetMinMaxInfo(MINMAXINFO* info)
        {
            CWnd::OnGetMinMaxInfo(info);
            const SIZE outer = m_titleBar.OuterSizeForContent(Px(800), Px(600));
            info->ptMinTrackSize = { outer.cx, outer.cy };
        }
        LRESULT Workspace::OnDpi(WPARAM dpi, LPARAM value)
        {
            m_dpi = HIWORD(dpi); const auto* bounds = reinterpret_cast<const RECT*>(value);
            SetWindowPos(nullptr, bounds->left, bounds->top, bounds->right - bounds->left, bounds->bottom - bounds->top, SWP_NOZORDER | SWP_NOACTIVATE);
            SetFonts(); Layout(); return 0;
        }
        HBRUSH Workspace::OnCtlColor(CDC* dc, CWnd* child, UINT)
        {
            if(auto* field=DYNAMIC_DOWNCAST(NativeMaterial::TextField,child))return field->Color(dc);
            const auto colors = NativeMaterial::Palette::Resolve(m_appearance);
            dc->SetTextColor(Color(colors.onSurface)); dc->SetBkColor(Color(colors.surface));
            return static_cast<HBRUSH>(m_background.GetSafeHandle());
        }
        void Workspace::OnClose() { ShowWindow(SW_HIDE); m_controller.Stop(); DestroyWindow(); }
        void Workspace::OnDestroy()
        {
            m_preferences.Detach();
            m_regexEditor.Close();
            m_controller.Stop(); m_floating = {}; m_canvas.Detach();
            CWnd::OnDestroy();
        }
        void Workspace::PostNcDestroy()
        {
            const auto found = windows.find(m_owner);
            if (found != windows.end() && found->second == this) windows.erase(found);
            if (m_autoDelete) delete this;
        }
    }
    bool Open(CWnd* owner, DWORD intervalMilliseconds, std::function<void()> floatingMonitor)
    {
        if (!owner || !::IsWindow(owner->GetSafeHwnd())) return false;
        if (!Setup::Show(owner)) return true;
        const HWND ownerHandle = owner->GetSafeHwnd();
        const auto found = windows.find(ownerHandle);
        if (found != windows.end() && ::IsWindow(found->second->GetSafeHwnd()))
        { found->second->ShowWindow(SW_RESTORE); found->second->SetForegroundWindow(); return true; }
        auto* workspace = new Workspace(ownerHandle, intervalMilliseconds, std::move(floatingMonitor));
        if (!workspace->CreateWorkspace())
        {
            delete workspace;
            NativeMaterial::Alert(owner->GetSafeHwnd(), L"The monitoring workspace could not be created. Existing monitoring remains active.", L"TrafficMonitor", MB_OK | MB_ICONERROR);
            return false;
        }
        workspace->Adopt(); windows[ownerHandle] = workspace;
        workspace->ShowWindow(SW_SHOW); workspace->SetForegroundWindow();
        return true;
    }
}
