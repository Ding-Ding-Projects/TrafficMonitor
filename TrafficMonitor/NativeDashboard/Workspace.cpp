#include "../stdafx.h"
#include "Workspace.h"
#include "ViewModel.h"
#include "Presentation.h"
#include "../NativeDashboardController.h"
#include "../NativeMaterial/Controls.h"
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
            PauseId, RefreshId, SearchId, ClearId, TabsId, AdaptersId, SamplesId, DetailsId, StatusId, SummaryId, RegexId };
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
            afx_msg void OnClose();
            afx_msg void OnDestroy();
            afx_msg void OnTimer(UINT_PTR id) { if(id==0x4E92 && m_model.PollFilter()) RefreshRows(); else CWnd::OnTimer(id); }
            afx_msg LRESULT OnSnapshot(WPARAM generation, LPARAM);
            afx_msg LRESULT OnDpi(WPARAM dpi, LPARAM bounds);
            afx_msg HBRUSH OnCtlColor(CDC* dc, CWnd*, UINT);
            afx_msg void OnAdapterChanged(NMHDR*, LRESULT* result);
            afx_msg void OnTabChanged(NMHDR*, LRESULT* result);
            afx_msg void OnContextMenu(CWnd* source, CPoint point);
            BOOL OnCommand(WPARAM wparam, LPARAM lparam) override;
            BOOL PreTranslateMessage(MSG* message) override;
            void PostNcDestroy() override;
            DECLARE_MESSAGE_MAP()
        private:
            CString T(const wchar_t* english, const wchar_t* chinese) const { return m_chinese ? chinese : english; }
            int Px(int value) const { return MulDiv(value, m_dpi, 96); }
            void Layout();
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
            CBrush m_background;
            NativeMaterial::Button m_history, m_plugins, m_settings, m_floatButton, m_theme, m_pause, m_refresh, m_clear, m_regex;
            CEdit m_search, m_details;
            CStatic m_status, m_summary;
            CTabCtrl m_tabs;
            CListCtrl m_adapters, m_samples;
        };
        BEGIN_MESSAGE_MAP(Workspace, CWnd)
            ON_WM_CREATE()
            ON_WM_TIMER()
            ON_WM_PAINT()
            ON_WM_ERASEBKGND()
            ON_WM_SIZE()
            ON_WM_GETMINMAXINFO()
            ON_WM_CLOSE()
            ON_WM_DESTROY()
            ON_WM_CTLCOLOR()
            ON_WM_CONTEXTMENU()
            ON_MESSAGE(SnapshotMessage, &Workspace::OnSnapshot)
            ON_MESSAGE(WM_DPICHANGED, &Workspace::OnDpi)
            ON_NOTIFY(LVN_ITEMCHANGED, AdaptersId, &Workspace::OnAdapterChanged)
            ON_NOTIFY(TCN_SELCHANGE, TabsId, &Workspace::OnTabChanged)
        END_MESSAGE_MAP()

        bool Workspace::CreateWorkspace()
        {
            LANGID language = theApp.m_general_data.language.language_id;
            if (!language) language = GetUserDefaultUILanguage();
            m_chinese = PRIMARYLANGID(language) == LANG_CHINESE;
            DWORD light = 1, bytes = sizeof(light);
            RegGetValueW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
                L"AppsUseLightTheme", RRF_RT_REG_DWORD, nullptr, &light, &bytes);
            m_appearance = light ? NativeMaterial::Appearance::Light : NativeMaterial::Appearance::Dark;
            const CString className = AfxRegisterWndClass(CS_HREDRAW | CS_VREDRAW, ::LoadCursor(nullptr, IDC_ARROW), nullptr, AfxGetApp()->LoadIcon(IDR_MAINFRAME));
            MONITORINFO monitor{ sizeof(monitor) };
            GetMonitorInfoW(MonitorFromWindow(m_owner, MONITOR_DEFAULTTONEAREST), &monitor);
            m_dpi = GetDpiForWindow(m_owner);
            if (!m_dpi) m_dpi = 96;
            const int width = (std::min)(Px(1120), int(monitor.rcWork.right - monitor.rcWork.left));
            const int height = (std::min)(Px(760), int(monitor.rcWork.bottom - monitor.rcWork.top));
            const int x = monitor.rcWork.left + (monitor.rcWork.right - monitor.rcWork.left - width) / 2;
            const int y = monitor.rcWork.top + (monitor.rcWork.bottom - monitor.rcWork.top - height) / 2;
            return CreateEx(WS_EX_CONTROLPARENT, className, T(L"TrafficMonitor - Network workspace", L"TrafficMonitor - 網絡工作區"),
                WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN, x, y, width, height, m_owner, nullptr) != FALSE;
        }
        int Workspace::OnCreate(LPCREATESTRUCT create)
        {
            if (CWnd::OnCreate(create) == -1) return -1;
            m_dpi = GetDpiForWindow(m_hWnd); if (!m_dpi) m_dpi = 96;
            if (FAILED(m_canvas.Attach(m_hWnd, static_cast<float>(m_dpi)))) return -1;
            const CRect initial(0, 0, 100, 40);
            if (!m_history.Create(T(L"History", L"流量記錄"), initial, this, HistoryId)
                || !m_plugins.Create(T(L"Plugins", L"外掛程式"), initial, this, PluginsId)
                || !m_settings.Create(T(L"Settings", L"設定"), initial, this, SettingsId)
                || !m_floatButton.Create(T(L"Toggle floating monitor", L"顯示／隱藏浮動監察器"), initial, this, FloatingId)
                || !m_theme.Create(T(L"Switch appearance", L"切換外觀"), initial, this, ThemeId)
                || !m_pause.Create(T(L"Pause display", L"暫停顯示"), initial, this, PauseId)
                || !m_refresh.Create(T(L"Restart samples", L"重新採樣"), initial, this, RefreshId)
                || !m_regex.Create(T(L"Regex", L"正規式"), initial, this, RegexId)
                || !m_clear.Create(T(L"Clear", L"清除"), initial, this, ClearId)) return -1;
            const DWORD child = WS_CHILD | WS_VISIBLE | WS_TABSTOP;
            if (!m_search.CreateEx(WS_EX_CLIENTEDGE, L"EDIT", L"", child | ES_AUTOHSCROLL, initial, this, SearchId)
                || !m_tabs.Create(child, initial, this, TabsId)
                || !m_adapters.Create(child | LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS, initial, this, AdaptersId)
                || !m_samples.Create(child | LVS_REPORT | LVS_SINGLESEL, initial, this, SamplesId)
                || !m_details.Create(child | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL | WS_VSCROLL, initial, this, DetailsId)
                || !m_status.Create(L"", WS_CHILD | WS_VISIBLE, initial, this, StatusId)
                || !m_summary.Create(L"", WS_CHILD | WS_VISIBLE, initial, this, SummaryId)) return -1;
            m_search.SendMessage(EM_SETCUEBANNER, TRUE, reinterpret_cast<LPARAM>(m_chinese ? L"搜尋網絡介面名稱或描述" : L"Search adapter name or description"));
            m_search.SetLimitText(512);
            m_adapters.SetWindowText(T(L"Observed network adapters", L"已觀察網絡介面"));
            m_samples.SetWindowText(T(L"Selected adapter sample history", L"所選網絡介面採樣記錄"));
            m_details.SetWindowText(T(L"Waiting for adapter discovery", L"正在等待網絡介面資料"));
            m_adapters.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_LABELTIP);
            m_samples.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_LABELTIP);
            m_tabs.InsertItem(0, T(L"Adapters", L"網絡介面"));
            m_tabs.InsertItem(1, T(L"Sample history", L"採樣記錄"));
            m_tabs.InsertItem(2, T(L"Details", L"詳細資料"));
            m_adapters.InsertColumn(0, T(L"Adapter", L"網絡介面"), LVCFMT_LEFT, 240);
            m_adapters.InsertColumn(1, T(L"State", L"狀態"), LVCFMT_LEFT, 180);
            m_adapters.InsertColumn(2, T(L"Receiving", L"接收"), LVCFMT_LEFT, 130);
            m_adapters.InsertColumn(3, T(L"Sending", L"傳送"), LVCFMT_LEFT, 130);
            m_adapters.InsertColumn(4, T(L"Interval", L"間隔"), LVCFMT_LEFT, 100);
            m_samples.InsertColumn(0, T(L"Relative time", L"相對時間"), LVCFMT_LEFT, 160);
            m_samples.InsertColumn(1, T(L"Receiving", L"接收"), LVCFMT_LEFT, 160);
            m_samples.InsertColumn(2, T(L"Sending", L"傳送"), LVCFMT_LEFT, 160);
            m_samples.InsertColumn(3, T(L"State", L"狀態"), LVCFMT_LEFT, 240);
            SetFonts(); ApplyTheme(); Layout(); Start(); SetTimer(0x4E92,40,nullptr);
            return 0;
        }
        void Workspace::SetFonts()
        {
            LOGFONT font{};
            NONCLIENTMETRICS metrics{ sizeof(metrics) };
            if (SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(metrics), &metrics, 0)) font = metrics.lfMessageFont;
            else wcscpy_s(font.lfFaceName, L"Segoe UI");
            font.lfHeight = -Px(14);
            HFONT replacement = CreateFontIndirectW(&font);
            if (!replacement) return;
            HFONT previous = static_cast<HFONT>(m_font.Detach()); m_font.Attach(replacement);
            for (CWnd* control : std::array<CWnd*, 7>{&m_search, &m_tabs, &m_adapters, &m_samples, &m_details, &m_status, &m_summary}) control->SetFont(&m_font);
            if (previous) DeleteObject(previous);
        }
        void Workspace::ApplyTheme()
        {
            const auto palette = NativeMaterial::Palette::Resolve(m_appearance);
            m_background.DeleteObject(); m_background.CreateSolidBrush(Color(palette.surface));
            for (auto* button : {&m_history, &m_plugins, &m_settings, &m_floatButton, &m_theme, &m_pause, &m_refresh, &m_clear, &m_regex}) button->SetAppearance(m_appearance);
            for (auto* list : {&m_adapters, &m_samples})
            {
                list->SetBkColor(Color(palette.container)); list->SetTextBkColor(Color(palette.container)); list->SetTextColor(Color(palette.onSurface));
            }
            RedrawWindow(nullptr, nullptr, RDW_INVALIDATE | RDW_ALLCHILDREN);
        }
        void Workspace::Layout()
        {
            if (!m_search.GetSafeHwnd()) return;
            CRect client; GetClientRect(client); const int width = MulDiv(client.Width(), 96, m_dpi), height = MulDiv(client.Height(), 96, m_dpi);
            auto place = [&](CWnd& child, int x, int y, int w, int h) { child.MoveWindow(Px(x), Px(y), Px((std::max)(1, w)), Px((std::max)(1, h))); };
            place(m_summary, 24, 42, width - 48, 28);
            place(m_history, 24, 76, 104, 40); place(m_plugins, 140, 76, 104, 40); place(m_settings, 256, 76, 104, 40);
            place(m_floatButton, 372, 76, 198, 40); place(m_theme, 582, 76, width - 606, 40);
            place(m_pause, 24, 128, 140, 40); place(m_refresh, 176, 128, 150, 40);
            place(m_search, 342, 130, width - 554, 36); place(m_regex,width-200,128,84,40); place(m_clear, width - 108, 128, 84, 40);
            place(m_status, 24, 171, width - 48, 17);
            const int tabY = (std::max)(404, (std::min)(440, height - 164));
            place(m_tabs, 24, tabY, width - 48, height - tabY - 20);
            CRect body; m_tabs.GetClientRect(body); m_tabs.AdjustRect(FALSE, body); m_tabs.MapWindowPoints(this, body);
            body.DeflateRect(Px(4), Px(4));
            for (CWnd* page : std::array<CWnd*, 3>{&m_adapters, &m_samples, &m_details}) page->MoveWindow(body);
            const int tableWidth = (std::max)(700, width - 72);
            const int adapterWidths[] = { tableWidth * 30 / 100, tableWidth * 25 / 100, tableWidth * 17 / 100, tableWidth * 17 / 100, tableWidth * 11 / 100 };
            for (int i = 0; i < 5; ++i) m_adapters.SetColumnWidth(i, Px(adapterWidths[i]));
            const int sampleWidths[] = { tableWidth * 22 / 100, tableWidth * 20 / 100, tableWidth * 20 / 100, tableWidth * 38 / 100 };
            for (int i = 0; i < 4; ++i) m_samples.SetColumnWidth(i, Px(sampleWidths[i]));
            ShowPage(); Invalidate(FALSE);
        }
        void Workspace::ShowPage()
        {
            const int page = m_tabs.GetCurSel();
            m_adapters.ShowWindow(page == 0 ? SW_SHOW : SW_HIDE);
            m_samples.ShowWindow(page == 1 ? SW_SHOW : SW_HIDE);
            m_details.ShowWindow(page == 2 ? SW_SHOW : SW_HIDE);
        }
        void Workspace::Start()
        {
            m_startResult = m_controller.Start(m_hWnd, SnapshotMessage, m_interval);
            Status();
        }
        void Workspace::Status()
        {
            CString status;
            if (FAILED(m_startResult)) status.Format(T(L"Sampling could not start (0x%08X)", L"未能開始採樣 (0x%08X)"), static_cast<unsigned>(m_startResult));
            else if (m_controller.WorkerError()) status.Format(T(L"Sampling worker error: %lu", L"採樣工作錯誤：%lu"), m_controller.WorkerError());
            else if (!m_model.Displayed()) status = T(L"Collecting first sample; restart affects display history only", L"正在收集首個樣本；重新採樣只影響顯示記錄");
            else
            {
                const auto& snapshot = *m_model.Displayed();
                if (snapshot.state == NativeMonitoring::SnapshotState::Unavailable)
                    status.Format(T(L"Discovery unavailable (%u); last rows are stale", L"未能取得介面資料 (%u)；舊讀數已過時"), snapshot.error);
                else if (snapshot.adapters.empty()) status = T(L"No adapters were discovered", L"未發現網絡介面");
                else if (m_model.Rows().empty()) status = T(L"No adapters match this search", L"沒有符合搜尋的網絡介面");
                else status.Format(T(L"%zu adapters shown · observation %llu · %s", L"顯示 %zu 個介面 · 觀察 %llu · %s"), m_model.Rows().size(), snapshot.sequence,
                    m_model.Paused() ? T(L"Display paused; collection continues", L"顯示已暫停；採樣繼續").GetString() : T(L"Live display", L"即時顯示").GetString());
            }
            if(!m_model.search.status.empty()) status+=L" | "+CString(m_model.search.status.c_str());
            m_status.SetWindowText(status);
            const auto* selected = m_model.Selected();
            std::wstring summary = m_chinese ? L"所選介面：" : L"Selected adapter: ";
            summary += selected ? selected->alias : (m_model.SelectedIdentity().empty() ? (m_chinese ? L"尚未選擇" : L"None selected") : (m_chinese ? L"已不存在" : L"No longer present"));
            if (selected) summary += L" · " + TimeText(selected->acquiredAtUtcFileTime) + L" · " + RateStateText(selected->rateState, m_chinese);
            m_summary.SetWindowText(summary.c_str());
        }
        void Workspace::RefreshRows()
        {
            const int top = m_adapters.GetTopIndex();
            m_rendering = true; m_adapters.SetRedraw(FALSE); m_adapters.DeleteAllItems();
            for (std::size_t i = 0; i < m_model.Rows().size(); ++i)
            {
                const auto* row = m_model.Row(i); const int index = m_adapters.InsertItem(static_cast<int>(i), row->alias.c_str());
                m_adapters.SetItemText(index, 1, RateStateText(row->rateState, m_chinese).c_str());
                m_adapters.SetItemText(index, 2, Rate(row->receivedBytesPerSecond, m_chinese).c_str());
                m_adapters.SetItemText(index, 3, Rate(row->sentBytesPerSecond, m_chinese).c_str());
                CString interval; interval.Format(L"%.2f s", row->intervalSeconds); m_adapters.SetItemText(index, 4, interval);
            }
            const int selected = m_model.SelectedRow();
            if (selected >= 0) m_adapters.SetItemState(selected, LVIS_SELECTED, LVIS_SELECTED);
            if (top >= 0 && top < m_adapters.GetItemCount()) m_adapters.EnsureVisible(top, FALSE);
            m_adapters.SetRedraw(TRUE); m_adapters.Invalidate(); m_rendering = false;
            RefreshDetails(); Status(); Invalidate(FALSE);
        }
        void Workspace::RefreshDetails()
        {
            const auto* row = m_model.Selected();
            m_details.SetWindowText(Details(row, !m_model.SelectedIdentity().empty(), m_chinese).c_str());
            m_samples.SetRedraw(FALSE); m_samples.DeleteAllItems();
            if (row && !row->history.empty())
            {
                const auto latest = row->history.back().monotonicMilliseconds;
                for (auto it = row->history.rbegin(); it != row->history.rend(); ++it)
                {
                    CString relative; relative.Format(L"-%.2f s", double(latest - it->monotonicMilliseconds) / 1000);
                    const int index = m_samples.InsertItem(m_samples.GetItemCount(), relative);
                    m_samples.SetItemText(index, 1, Rate(it->receivedBytesPerSecond, m_chinese).c_str());
                    m_samples.SetItemText(index, 2, Rate(it->sentBytesPerSecond, m_chinese).c_str());
                    m_samples.SetItemText(index, 3, RateStateText(it->state, m_chinese).c_str());
                }
            }
            m_samples.SetRedraw(TRUE); m_samples.Invalidate();
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
        void Workspace::OnAdapterChanged(NMHDR* header, LRESULT* result)
        {
            *result = 0; if (m_rendering) return;
            auto* change = reinterpret_cast<NMLISTVIEW*>(header);
            if ((change->uChanged & LVIF_STATE) && (change->uNewState & LVIS_SELECTED) && !(change->uOldState & LVIS_SELECTED) && change->iItem >= 0)
            { m_model.Select(static_cast<std::size_t>(change->iItem)); RefreshDetails(); Status(); Invalidate(FALSE); }
        }
        void Workspace::OnTabChanged(NMHDR*, LRESULT* result) { *result = 0; ShowPage(); }
        BOOL Workspace::OnCommand(WPARAM wparam, LPARAM lparam)
        {
            const UINT id = LOWORD(wparam);
            if (id == SearchId && HIWORD(wparam) == EN_CHANGE)
            { CString query; m_search.GetWindowText(query); m_model.Filter(query.GetString()); RefreshRows(); return TRUE; }
            if (HIWORD(wparam) != BN_CLICKED) return CWnd::OnCommand(wparam, lparam);
            switch (id)
            {
            case HistoryId: case PluginsId: case SettingsId:
                if (::IsWindow(m_owner)) ::PostMessageW(m_owner, WM_COMMAND, id == HistoryId ? ID_TRAFFIC_HISTORY : (id == PluginsId ? ID_PLUGIN_MANAGE : ID_OPTIONS), 0);
                return TRUE;
            case FloatingId: if (::IsWindow(m_owner) && m_floating) m_floating(); return TRUE;
            case ThemeId: m_appearance = m_appearance == NativeMaterial::Appearance::Light ? NativeMaterial::Appearance::Dark : NativeMaterial::Appearance::Light; ApplyTheme(); return TRUE;
            case PauseId:
                m_model.SetPaused(!m_model.Paused()); m_pause.SetLabel(m_model.Paused() ? T(L"Resume display", L"恢復顯示") : T(L"Pause display", L"暫停顯示")); RefreshRows(); return TRUE;
            case RefreshId:
                m_controller.Stop(); m_model.Reset(); m_pause.SetLabel(T(L"Pause display", L"暫停顯示")); RefreshRows(); Start(); return TRUE;
            case RegexId:
                if(NativeRegex::Edit(m_hWnd,m_search.GetSafeHwnd(),m_model.search.options,m_model.SearchSubjects(),m_chinese,m_appearance))
                { m_search.SetWindowText(m_model.search.options.query.c_str());m_model.Refilter(); }
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
            const UINT command = NativeMenus::Track(&menu, TPM_RETURNCMD | TPM_RIGHTBUTTON, point.x, point.y, this);
            if (command == 1 && m_model.Selected()) { m_tabs.SetCurSel(2); ShowPage(); m_details.SetFocus(); }
            else if (command == 2) OnCommand(PauseId, 0);
            else if (command == 3) OnCommand(RefreshId, 0);
        }
        BOOL Workspace::PreTranslateMessage(MSG* message)
        {
            if (message->message == WM_KEYDOWN)
            {
                if (message->wParam == VK_TAB)
                {
                    HWND next = ::GetNextDlgTabItem(m_hWnd, ::GetFocus(), (GetKeyState(VK_SHIFT) & 0x8000) != 0);
                    if (next) ::SetFocus(next);
                    return TRUE;
                }
                if (message->wParam == VK_F6) { m_tabs.SetFocus(); return TRUE; }
                if (message->wParam == 'L' && (GetKeyState(VK_CONTROL) & 0x8000)) { m_search.SetFocus(); return TRUE; }
                if (message->wParam == VK_ESCAPE)
                {
                    HIMC context = ImmGetContext(message->hwnd);
                    const bool composing = context && ImmGetCompositionStringW(context, GCS_COMPSTR, nullptr, 0) > 0;
                    if (context) ImmReleaseContext(message->hwnd, context);
                    if (composing) return CWnd::PreTranslateMessage(message);
                    CString query; m_search.GetWindowText(query);
                    if (::GetFocus() == m_search.GetSafeHwnd() && !query.IsEmpty()) m_search.SetWindowText(L"");
                    else if (m_tabs.GetCurSel() != 0) { m_tabs.SetCurSel(0); ShowPage(); m_adapters.SetFocus(); }
                    else OnClose();
                    return TRUE;
                }
            }
            return CWnd::PreTranslateMessage(message);
        }
        void Workspace::OnPaint()
        {
            CPaintDC paint(this); CRect bounds; GetClientRect(bounds);
            if (!m_canvas.Target() && FAILED(m_canvas.Attach(m_hWnd, static_cast<float>(m_dpi)))) return;
            m_canvas.Resize(bounds.Width(), bounds.Height(), static_cast<float>(m_dpi));
            const auto colors = NativeMaterial::Palette::Resolve(m_appearance);
            if (FAILED(m_canvas.Begin(colors.surface))) return;
            Microsoft::WRL::ComPtr<IDWriteTextFormat> title;
            if (SUCCEEDED(m_canvas.TextFormat(23, DWRITE_FONT_WEIGHT_SEMI_BOLD, title.GetAddressOf())))
            {
                const CString text = T(L"Network workspace", L"網絡工作區");
                m_canvas.Target()->DrawText(text, text.GetLength(), title.Get(), D2D1::RectF(24, 10, 740, 42), m_canvas.Brush(colors.onSurface));
            }
            const float width = bounds.Width() * 96.0f / m_dpi;
            const int height = MulDiv(bounds.Height(), 96, m_dpi);
            const float bottom = static_cast<float>((std::max)(404, (std::min)(440, height - 164)) - 12);
            DrawData(m_canvas, colors, width, 284, bottom, m_model.Selected(), m_chinese);
            if (FAILED(m_canvas.End())) Invalidate(FALSE);
        }
        void Workspace::OnSize(UINT type, int width, int height)
        {
            CWnd::OnSize(type, width, height);
            if (type != SIZE_MINIMIZED) { m_canvas.Resize(width, height, static_cast<float>(m_dpi)); Layout(); }
        }
        void Workspace::OnGetMinMaxInfo(MINMAXINFO* info)
        {
            CWnd::OnGetMinMaxInfo(info);
            RECT bounds{ 0, 0, Px(800), Px(600) }; AdjustWindowRectEx(&bounds, WS_OVERLAPPEDWINDOW, FALSE, WS_EX_CONTROLPARENT);
            info->ptMinTrackSize = { bounds.right - bounds.left, bounds.bottom - bounds.top };
        }
        LRESULT Workspace::OnDpi(WPARAM dpi, LPARAM value)
        {
            m_dpi = HIWORD(dpi); const auto* bounds = reinterpret_cast<const RECT*>(value);
            SetWindowPos(nullptr, bounds->left, bounds->top, bounds->right - bounds->left, bounds->bottom - bounds->top, SWP_NOZORDER | SWP_NOACTIVATE);
            SetFonts(); Layout(); return 0;
        }
        HBRUSH Workspace::OnCtlColor(CDC* dc, CWnd*, UINT)
        {
            const auto colors = NativeMaterial::Palette::Resolve(m_appearance);
            dc->SetTextColor(Color(colors.onSurface)); dc->SetBkColor(Color(colors.surface));
            return static_cast<HBRUSH>(m_background.GetSafeHandle());
        }
        void Workspace::OnClose() { ShowWindow(SW_HIDE); m_controller.Stop(); DestroyWindow(); }
        void Workspace::OnDestroy()
        {
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
        const HWND ownerHandle = owner->GetSafeHwnd();
        const auto found = windows.find(ownerHandle);
        if (found != windows.end() && ::IsWindow(found->second->GetSafeHwnd()))
        { found->second->ShowWindow(SW_RESTORE); found->second->SetForegroundWindow(); return true; }
        auto* workspace = new Workspace(ownerHandle, intervalMilliseconds, std::move(floatingMonitor));
        if (!workspace->CreateWorkspace())
        {
            delete workspace;
            owner->MessageBox(L"The monitoring workspace could not be created. Existing monitoring remains active.", L"TrafficMonitor", MB_OK | MB_ICONERROR);
            return false;
        }
        workspace->Adopt(); windows[ownerHandle] = workspace;
        workspace->ShowWindow(SW_SHOW); workspace->SetForegroundWindow();
        return true;
    }
}
