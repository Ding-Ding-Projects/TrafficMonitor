#include "../stdafx.h"
#include "SetupFlow.h"
#include "SetupFlowText.h"
#include "SetupFlowProgress.h"
#include "../TrafficMonitorDlg.h"
#include "../NativeMaterial/Controls.h"
#include <afxcmn.h>
#include <iphlpapi.h>
#include <algorithm>
#include <array>
#include <imm.h>

namespace NativeDashboard::Setup
{
    Discovery Discover()
    {
        Discovery result; result.allAllowed = !theApp.m_general_data.show_all_interface;
        PMIB_IF_TABLE2 table{}; result.error = GetIfTable2(&table);
        if (result.error != NO_ERROR) return result;
        if (table->NumEntries > 4096) { FreeMibTable(table); result.error = ERROR_BUFFER_OVERFLOW; return result; }
        auto* monitor = CTrafficMonitorDlg::Instance();
        for (ULONG i = 0; i < table->NumEntries; ++i)
        {
            const auto& row = table->Table[i]; wchar_t guid[40]{};
            if (row.InterfaceGuid == GUID_NULL || !monitor || !monitor->SetupAdapterEligible(row.InterfaceIndex)
                || !StringFromGUID2(row.InterfaceGuid, guid, _countof(guid))) continue;
            result.adapters.push_back({guid, row.Alias, row.InterfaceIndex, row.InterfaceLuid.Value});
        }
        FreeMibTable(table);
        std::sort(result.adapters.begin(), result.adapters.end(), [](const Adapter& a, const Adapter& b) { return a.guid < b.guid; });
        result.epoch = 1469598103934665603ULL;
        for (const auto& row : result.adapters)
        {
            for (auto c : row.guid) { result.epoch ^= c; result.epoch *= 1099511628211ULL; }
            result.epoch ^= row.index; result.epoch *= 1099511628211ULL;
            result.epoch ^= row.luid; result.epoch *= 1099511628211ULL;
        }
        return result;
    }
    namespace
    {
        enum : UINT { Language=55001, EnglishStyle, CantoneseStyle, ModeChoice, AdapterChoice, TaskbarChoice, StartupChoice,
            Back, Next, SaveClose, Refresh, KeepRemaining };
        using Result = OperationResult;
        std::wstring Path() { return theApp.m_config_dir + L"dashboard-setup.ini"; }
        int Bound(int value, int low, int high) { return (std::max)(low, (std::min)(high, value)); }
        struct Draft
        {
            int step{}, language{}, english{5}, cantonese{5}, taskbar{}, startup{};
            Selection selection;
            std::array<Result, 3> results{};
            bool complete{};
        };
        Draft Load()
        {
            Draft d;
            LANGID language = theApp.m_general_data.language.language_id;
            d.language = PRIMARYLANGID(language ? language : GetUserDefaultUILanguage()) == LANG_CHINESE ? 1 : 0;
            if (auto* monitor = CTrafficMonitorDlg::Instance()) d.selection = monitor->SetupSelection();
            WIN32_FILE_ATTRIBUTE_DATA attributes{};
            if (!GetFileAttributesExW(Path().c_str(), GetFileExInfoStandard, &attributes)
                || attributes.nFileSizeHigh || attributes.nFileSizeLow > 16384) return d;
            CIniHelper ini(Path());
            if (ini.GetInt(L"setup", L"schemaVersion", 0) != 1) return d;
            d.step = Bound(ini.GetInt(L"setup", L"step", 0), 0, 3);
            d.language = Bound(ini.GetInt(L"setup", L"language", d.language), 0, 2);
            d.english = Bound(ini.GetInt(L"setup", L"englishStyle", 5), 1, 5);
            d.cantonese = Bound(ini.GetInt(L"setup", L"cantoneseStyle", 5), 1, 5);
            d.taskbar = Bound(ini.GetInt(L"setup", L"taskbar", 0), 0, 2);
            d.startup = Bound(ini.GetInt(L"setup", L"startup", 0), 0, 2);
            d.selection.mode = static_cast<Mode>(Bound(ini.GetInt(L"setup", L"mode", static_cast<int>(d.selection.mode)), 0, 2));
            auto guid = ini.GetString(L"setup", L"adapter", L""); GUID parsed{};
            if (guid.size() == 38 && SUCCEEDED(CLSIDFromString(guid.c_str(), &parsed))) d.selection.guid = guid;
            else if (d.selection.mode == Mode::One) d.selection.guid.clear();
            for (int i = 0; i < 3; ++i) { const auto key = L"result" + std::to_wstring(i); d.results[i] = static_cast<Result>(Bound(ini.GetInt(L"setup", key.c_str(), 0), 0, 3)); }
            d.complete = ini.GetBool(L"setup", L"complete", false);
            return d;
        }
        bool Save(const Draft& d)
        {
            const auto temp = Path() + L".tmp";
            DeleteFileW(temp.c_str()); CIniHelper output(temp);
            output.WriteInt(L"setup", L"schemaVersion", 1); output.WriteInt(L"setup", L"step", d.step);
            output.WriteInt(L"setup", L"language", d.language); output.WriteInt(L"setup", L"englishStyle", d.english);
            output.WriteInt(L"setup", L"cantoneseStyle", d.cantonese); output.WriteInt(L"setup", L"taskbar", d.taskbar);
            output.WriteInt(L"setup", L"startup", d.startup); output.WriteInt(L"setup", L"mode", static_cast<int>(d.selection.mode));
            output.WriteString(L"setup", L"adapter", d.selection.guid); output.WriteBool(L"setup", L"complete", d.complete);
            for (int i = 0; i < 3; ++i) { const auto key = L"result" + std::to_wstring(i); output.WriteInt(L"setup", key.c_str(), static_cast<int>(d.results[i])); }
            if (!output.Save() || !MoveFileExW(temp.c_str(), Path().c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) return false;
            CIniHelper stored(Path());
            return stored.GetInt(L"setup", L"schemaVersion", 0) == 1 && stored.GetInt(L"setup", L"step", -1) == d.step
                && stored.GetBool(L"setup", L"complete", !d.complete) == d.complete;
        }
        bool StartupEnabled() { return theApp.GetAutoRun(nullptr, theApp.m_installed_mode ? false : theApp.m_general_data.auto_run_by_task_scheduler); }
        bool active{};
        class Wizard final : public CDialog
        {
        public:
            explicit Wizard(Draft draft) : m_draft(std::move(draft)) {}
        protected:
            BOOL OnInitDialog() override;
            BOOL OnCommand(WPARAM, LPARAM) override;
            void OnCancel() override;
            void OnOK() override { Advance(); }
            BOOL PreTranslateMessage(MSG* msg) override;
            afx_msg void OnPaint() { CPaintDC dc(this); CRect r; GetClientRect(r); dc.FillRect(r, &m_background); }
            afx_msg BOOL OnEraseBkgnd(CDC*) { return TRUE; }
            afx_msg void OnSize(UINT, int, int) { Layout(); }
            afx_msg void OnGetMinMaxInfo(MINMAXINFO* info);
            afx_msg HBRUSH OnCtlColor(CDC*, CWnd*, UINT);
            afx_msg LRESULT OnDpi(WPARAM, LPARAM);
            afx_msg void OnHScroll(UINT, UINT, CScrollBar*);
            DECLARE_MESSAGE_MAP()
        private:
            CString T(Copy id) const { return Text(id, m_draft.language); }
            int Px(int n) const { return MulDiv(n, m_dpi, 96); }
            void Labels(); void Layout(); void Fonts(); void ReadControls(); void ReloadAdapters();
            void Advance(); void Apply(); bool ValidAdapter(); void Review(); void Note(Copy id);
            Draft m_draft; Discovery m_discovery;
            UINT m_dpi{96}; bool m_loading{}, m_applying{};
            NativeMaterial::Appearance m_appearance{NativeMaterial::Appearance::Light};
            CFont m_font, m_titleFont, m_labelFont; CBrush m_background;
            CStatic m_heading, m_info, m_note;
            NativeMaterial::ReadOnlyText m_review;
            std::array<CStatic, 7> m_labels;
            std::array<NativeMaterial::Choice, 7> m_choices;
            std::array<NativeMaterial::RangeValue, 2> m_ranges;
            NativeMaterial::Button m_back, m_next, m_save, m_refresh, m_keep;
        };
        BEGIN_MESSAGE_MAP(Wizard, CDialog)
            ON_WM_PAINT()
            ON_WM_ERASEBKGND()
            ON_WM_SIZE()
            ON_WM_GETMINMAXINFO()
            ON_WM_CTLCOLOR()
            ON_WM_HSCROLL()
            ON_MESSAGE(WM_DPICHANGED, &Wizard::OnDpi)
        END_MESSAGE_MAP()
        BOOL Wizard::OnInitDialog()
        {
            CDialog::OnInitDialog(); m_dpi = GetDpiForWindow(m_hWnd); if (!m_dpi) m_dpi = 96;
            DWORD light = 1, bytes = sizeof(light);
            RegGetValueW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize", L"AppsUseLightTheme", RRF_RT_REG_DWORD, nullptr, &light, &bytes);
            m_appearance = light ? NativeMaterial::Appearance::Light : NativeMaterial::Appearance::Dark;
            const auto palette = NativeMaterial::Palette::Resolve(m_appearance);
            m_background.CreateSolidBrush(RGB(BYTE(palette.surface.r*255), BYTE(palette.surface.g*255), BYTE(palette.surface.b*255)));
            CRect box(0, 0, 100, 40); DWORD child = WS_CHILD | WS_VISIBLE;
            m_heading.Create(L"", child, box, this); m_info.Create(L"", child, box, this); m_note.Create(L"", child, box, this);
            m_review.Create(box, this, 55030);m_review.SetAppearance(m_appearance);
            for (size_t i = 0; i < m_choices.size(); ++i)
            {
                m_labels[i].Create(L"", child, box, this);
                if(i==1||i==2){m_ranges[i-1].Create(box,this,Language+static_cast<UINT>(i));m_ranges[i-1].SetAppearance(m_appearance);}
                else{m_choices[i].Create(child|WS_TABSTOP|CBS_DROPDOWNLIST|WS_VSCROLL, box, this, Language + static_cast<UINT>(i));m_choices[i].SetAppearance(m_appearance);}
            }
            m_back.Create(L"", box, this, Back); m_next.Create(L"", box, this, Next); m_save.Create(L"", box, this, SaveClose);
            m_refresh.Create(L"", box, this, Refresh); m_keep.Create(L"", box, this, KeepRemaining);
            for (auto* button : {&m_back, &m_next, &m_save, &m_refresh, &m_keep}) button->SetAppearance(m_appearance);
            Fonts(); Labels(); ReloadAdapters();
            RECT r{0,0,Px(760),Px(640)}; AdjustWindowRectEx(&r,GetStyle(),FALSE,GetExStyle());
            SetWindowPos(nullptr,0,0,r.right-r.left,r.bottom-r.top,SWP_NOMOVE|SWP_NOZORDER); CenterWindow(); Layout();
            m_next.SetFocus(); return FALSE;
        }
        void Wizard::Fonts()
        {
            NONCLIENTMETRICS metrics{sizeof(metrics)}; LOGFONT font{};
            if (SystemParametersInfoW(SPI_GETNONCLIENTMETRICS,sizeof(metrics),&metrics,0)) font=metrics.lfMessageFont;
            else wcscpy_s(font.lfFaceName,L"Segoe UI"); font.lfHeight=-Px(14);
            auto next=CreateFontIndirectW(&font); if(!next)return;
            auto previous=static_cast<HFONT>(m_font.Detach());m_font.Attach(next);
            for(CWnd* child=GetWindow(GW_CHILD);child;child=child->GetNextWindow())child->SetFont(&m_font);
            if(previous)DeleteObject(previous);
            m_titleFont.DeleteObject();font.lfHeight=-Px(22);font.lfWeight=FW_SEMIBOLD;m_titleFont.CreateFontIndirect(&font);m_heading.SetFont(&m_titleFont);
            m_labelFont.DeleteObject();font.lfHeight=-Px(14);font.lfWeight=FW_MEDIUM;m_labelFont.CreateFontIndirect(&font);
            for(auto& label:m_labels)label.SetFont(&m_labelFont);
        }
        void Wizard::Labels()
        {
            m_loading=true; SetWindowText(T(Copy::Title));
            const Copy labels[] = {Copy::Language,Copy::EnglishStyle,Copy::CantoneseStyle,Copy::ModeLabel,Copy::AdapterLabel,Copy::TaskbarLabel,Copy::StartupLabel};
            for(size_t i=0;i<m_labels.size();++i)m_labels[i].SetWindowText(T(labels[i]));
            for (int i : {0,3,5,6}) m_choices[i].ResetContent();
            for(const auto* label:{L"English",L"廣東話",L"English / 廣東話"})m_choices[0].AddString(label);
            m_ranges[0].SetWindowText(T(Copy::EnglishStyle));m_ranges[1].SetWindowText(T(Copy::CantoneseStyle));
            for(auto label:{Copy::Automatic,Copy::All,Copy::One})m_choices[3].AddString(T(label));
            for(int i:{5,6})for(auto label:{Copy::Keep,Copy::Enable,Copy::Disable})m_choices[i].AddString(T(label));
            m_choices[0].SetCurSel(m_draft.language);m_ranges[0].SetPos(m_draft.english);m_ranges[1].SetPos(m_draft.cantonese);
            m_choices[3].SetCurSel(static_cast<int>(m_draft.selection.mode));m_choices[5].SetCurSel(m_draft.taskbar);m_choices[6].SetCurSel(m_draft.startup);
            m_back.SetLabel(T(Copy::Back));m_save.SetLabel(T(Copy::SaveClose));m_refresh.SetLabel(T(Copy::Refresh));m_keep.SetLabel(T(Copy::KeepRemaining));
            m_loading=false;
        }
        void Wizard::ReloadAdapters()
        {
            m_discovery=Discover();m_choices[4].ResetContent();int selected=-1;
            for(const auto& adapter:m_discovery.adapters)
            {
                int row=m_choices[4].AddString((adapter.name+L" · "+adapter.guid).c_str());
                if(adapter.guid==m_draft.selection.guid)selected=row;
            }
            m_choices[4].SetCurSel(selected);
        }
        void Wizard::ReadControls()
        {
            if(m_loading)return;
            m_draft.language=Bound(m_choices[0].GetCurSel(),0,2);
            m_draft.english=Bound(m_ranges[0].GetPos(),1,5);m_draft.cantonese=Bound(m_ranges[1].GetPos(),1,5);
            const Mode mode=static_cast<Mode>(Bound(m_choices[3].GetCurSel(),0,2));
            std::wstring guid=m_draft.selection.guid;const int row=m_choices[4].GetCurSel();
            if(row>=0&&static_cast<size_t>(row)<m_discovery.adapters.size())guid=m_discovery.adapters[row].guid;
            if(mode!=m_draft.selection.mode||guid!=m_draft.selection.guid)m_draft.results[0]=Result::Pending;
            m_draft.selection={mode,guid};
            int taskbar=Bound(m_choices[5].GetCurSel(),0,2),startup=Bound(m_choices[6].GetCurSel(),0,2);
            if(taskbar!=m_draft.taskbar)m_draft.results[1]=Result::Pending;if(startup!=m_draft.startup)m_draft.results[2]=Result::Pending;
            m_draft.taskbar=taskbar;m_draft.startup=startup;
        }
        void Wizard::Layout()
        {
            if(!m_next.GetSafeHwnd())return;
            CRect r;GetClientRect(r);int width=MulDiv(r.Width(),96,m_dpi),height=MulDiv(r.Height(),96,m_dpi);
            auto place=[&](CWnd& child,int x,int y,int w,int h){child.MoveWindow(Px(x),Px(y),Px((std::max)(1,w)),Px(h));};
            m_heading.SetWindowText(T(static_cast<Copy>(static_cast<int>(Copy::LanguageStep)+m_draft.step)));
            m_info.SetWindowText(T(static_cast<Copy>(static_cast<int>(Copy::LanguageInfo)+m_draft.step)));
            auto textHeight=[&](CWnd& control,int minimum)
            {
                CClientDC dc(&control);auto old=dc.SelectObject(control.GetFont());
                CString text;control.GetWindowText(text);CRect measured(0,0,Px(width-48),0);
                dc.DrawText(text,measured,DT_CALCRECT|DT_WORDBREAK|DT_NOPREFIX);
                dc.SelectObject(old);return (std::max)(minimum,MulDiv(measured.Height(),96,m_dpi)+4);
            };
            const int headingHeight=textHeight(m_heading,30),infoTop=16+headingHeight+12;
            const int infoHeight=textHeight(m_info,48),contentTop=infoTop+infoHeight+12;
            place(m_heading,24,16,width-48,headingHeight);place(m_info,24,infoTop,width-48,infoHeight);
            int y=contentTop;
            for(int i=0;i<7;++i)
            {
                bool visible=(m_draft.step==0&&i<3)||(m_draft.step==1&&(i==3||i==4))||(m_draft.step==2&&i>=5);
                CWnd& choice=(i==1||i==2)?static_cast<CWnd&>(m_ranges[i-1]):static_cast<CWnd&>(m_choices[i]);
                m_labels[i].ShowWindow(visible?SW_SHOW:SW_HIDE);choice.ShowWindow(visible?SW_SHOW:SW_HIDE);
                if(visible){place(m_labels[i],24,y,width-48,34);place(choice,24,y+32,width-48,(i==1||i==2)?44:220);y+=80;}
            }
            m_choices[4].EnableWindow(m_draft.selection.mode==Mode::One&&!m_applying);
            m_refresh.ShowWindow(m_draft.step==1?SW_SHOW:SW_HIDE);place(m_refresh,24,y+8,width-48,44);
            m_review.ShowWindow(m_draft.step>=2?SW_SHOW:SW_HIDE);
            if(m_draft.step>=2){Review();place(m_review,24,m_draft.step==2?contentTop+168:contentTop,width-48,height-(m_draft.step==2?contentTop+168:contentTop)-208);}
            m_keep.ShowWindow(m_draft.step==3?SW_SHOW:SW_HIDE);place(m_keep,24,height-200,width-48,44);
            place(m_note,24,height-148,width-48,78);
            const int button=(width-72)/3;place(m_back,24,height-62,button,44);place(m_save,36+button,height-62,button,44);place(m_next,48+2*button,height-62,button,44);
            m_back.EnableWindow(m_draft.step>0&&!m_applying);m_save.EnableWindow(!m_applying);m_next.EnableWindow(!m_applying);
            m_next.SetLabel(T(m_draft.step==3?Copy::Apply:Copy::Next));
            RedrawWindow(nullptr,nullptr,RDW_INVALIDATE|RDW_ERASE|RDW_ALLCHILDREN|RDW_UPDATENOW);
        }
        void Wizard::Note(Copy id)
        {
            m_note.SetWindowText(T(id)+L"\r\n"+StyleNote(m_draft.language,m_draft.english,m_draft.cantonese));
            ::NotifyWinEvent(EVENT_OBJECT_NAMECHANGE,m_note.GetSafeHwnd(),OBJID_CLIENT,CHILDID_SELF);
        }
        void Wizard::Review()
        {
            CString summary=T(Copy::ModeLabel)+L": "+T(m_draft.selection.mode==Mode::Automatic?Copy::Automatic:m_draft.selection.mode==Mode::All?Copy::All:Copy::One);
            if(m_draft.selection.mode==Mode::One)summary+=L"\r\n"+CString(m_draft.selection.guid.c_str());
            summary+=L"\r\n"+T(Copy::TaskbarLabel)+L": "+T(m_draft.taskbar==0?Copy::Keep:m_draft.taskbar==1?Copy::Enable:Copy::Disable)
                +L" ("+T(theApp.m_cfg_data.m_show_task_bar_wnd?Copy::CurrentOn:Copy::CurrentOff)+L")";
            summary+=L"\r\n"+T(Copy::StartupLabel)+L": "+T(m_draft.startup==0?Copy::Keep:m_draft.startup==1?Copy::Enable:Copy::Disable)
                +L" ("+T(StartupEnabled()?Copy::CurrentOn:Copy::CurrentOff)+L")";
            const Copy names[]={Copy::ModeLabel,Copy::TaskbarLabel,Copy::StartupLabel};
            for(int i=0;i<3;++i)summary+=L"\r\n"+T(names[i])+L": "+T(m_draft.results[i]==Result::Applied?Copy::Applied:m_draft.results[i]==Result::Applying?Copy::Applying:m_draft.results[i]==Result::Failed?Copy::Failed:Copy::NotStarted);
            m_review.SetWindowText(summary);
        }
        bool Wizard::ValidAdapter()
        {
            const auto fresh=Discover();
            bool valid=!fresh.error&&fresh.epoch==m_discovery.epoch&&(m_draft.selection.mode!=Mode::All||fresh.allAllowed);
            if(m_draft.selection.mode==Mode::One)valid=valid&&std::any_of(fresh.adapters.begin(),fresh.adapters.end(),[&](const Adapter& a){return a.guid==m_draft.selection.guid;});
            if(!valid){m_draft.results[0]=Result::Pending;m_draft.step=1;ReloadAdapters();Layout();Note(Copy::AdapterChanged);}
            return valid;
        }
        void Wizard::Advance()
        {
            if(m_applying)return;ReadControls();
            if(m_draft.step==3){Apply();return;}
            if(m_draft.step==1&&m_draft.results[0]!=Result::Applied&&!ValidAdapter())return;
            ++m_draft.step;if(!Save(m_draft)){--m_draft.step;Note(Copy::SaveFailed);return;}Layout();m_next.SetFocus();
        }
        void Wizard::Apply()
        {
            auto* monitor=CTrafficMonitorDlg::Instance();if(m_applying||!monitor)return;
            if(m_draft.results[0]!=Result::Applied&&!ValidAdapter())return;
            m_applying=true;m_draft.complete=false;Layout();Note(Copy::Applying);
            if(!Save(m_draft)){m_applying=false;Layout();Note(Copy::SaveFailed);return;}
            for(int i=0;i<3;++i)
            {
                const bool saved=RunOperation(m_draft.results[i],[&]{return Save(m_draft);},[&]
                {
                    if(i==0)
                    {
                        const auto current=monitor->SetupSelection();
                        return current.mode==m_draft.selection.mode
                            &&(current.mode!=Mode::One||current.guid==m_draft.selection.guid);
                    }
                    if(i==1)return m_draft.taskbar==0||monitor->SetupTaskbarMatches(m_draft.taskbar==1);
                    return m_draft.startup==0||StartupEnabled()==(m_draft.startup==1);
                },[&]
                {
                Review();m_review.UpdateWindow();
                bool ok=false;
                if(i==0)ok=monitor->ApplySetupSelection(m_draft.selection,m_discovery.epoch);
                else if(i==1)ok=m_draft.taskbar==0||monitor->SetSetupTaskbar(m_draft.taskbar==1);
                else if(m_draft.startup==0)ok=true;
                else
                {
                    const bool desired=m_draft.startup==1;
                    ok=StartupEnabled()==desired;
                    if(!ok)ok=theApp.SetAutoRun(desired,theApp.m_installed_mode?false:theApp.m_general_data.auto_run_by_task_scheduler);
                    ok=ok&&StartupEnabled()==desired;
                    if(ok){theApp.m_general_data.auto_run=desired;theApp.SaveConfig();}
                }
                return ok;
                });
                if(!saved){m_applying=false;Layout();Note(Copy::SaveFailed);return;}
            }
            m_applying=false;
            m_draft.complete=std::all_of(m_draft.results.begin(),m_draft.results.end(),[](Result r){return r==Result::Applied;});
            if(!Save(m_draft)){m_draft.complete=false;Layout();Note(Copy::SaveFailed);return;}
            if(m_draft.complete)EndDialog(IDOK);else{Layout();Note(Copy::Incomplete);}
        }
        void Wizard::OnHScroll(UINT code,UINT position,CScrollBar* bar)
        {
            CDialog::OnHScroll(code,position,bar);
            if(m_loading||m_applying)return;ReadControls();
            m_note.SetWindowText(StyleNote(m_draft.language,m_draft.english,m_draft.cantonese));
        }
        BOOL Wizard::OnCommand(WPARAM w,LPARAM l)
        {
            UINT id=LOWORD(w);if(m_applying)return TRUE;
            if(id>=Language&&id<=StartupChoice&&HIWORD(w)==CBN_SELCHANGE)
            {ReadControls();Labels();Layout();m_note.SetWindowText(StyleNote(m_draft.language,m_draft.english,m_draft.cantonese));return TRUE;}
            if(HIWORD(w)!=BN_CLICKED)return CDialog::OnCommand(w,l);
            if(id==Next){Advance();return TRUE;}if(id==SaveClose){OnCancel();return TRUE;}
            if(id==Back){ReadControls();if(m_draft.step>0)--m_draft.step;Layout();return TRUE;}
            if(id==Refresh){ReadControls();ReloadAdapters();Layout();return TRUE;}
            if(id==KeepRemaining)
            {
                ReadControls();if(m_draft.results[0]!=Result::Applied)if(auto* monitor=CTrafficMonitorDlg::Instance())m_draft.selection=monitor->SetupSelection();
                if(m_draft.results[1]!=Result::Applied)m_draft.taskbar=0;if(m_draft.results[2]!=Result::Applied)m_draft.startup=0;
                ReloadAdapters();Labels();Layout();return TRUE;
            }
            return CDialog::OnCommand(w,l);
        }
        void Wizard::OnCancel()
        {if(m_applying)return;ReadControls();m_draft.complete=false;if(Save(m_draft))EndDialog(IDCANCEL);else Note(Copy::SaveFailed);}
        BOOL Wizard::PreTranslateMessage(MSG* msg)
        {
            if(msg->message==WM_KEYDOWN&&msg->wParam==VK_ESCAPE)
            {HIMC context=ImmGetContext(msg->hwnd);bool composing=context&&ImmGetCompositionStringW(context,GCS_COMPSTR,nullptr,0)>0;if(context)ImmReleaseContext(msg->hwnd,context);if(composing)return FALSE;}
            return CDialog::PreTranslateMessage(msg);
        }
        void Wizard::OnGetMinMaxInfo(MINMAXINFO* info)
        {CDialog::OnGetMinMaxInfo(info);RECT r{0,0,Px(720),Px(600)};AdjustWindowRectEx(&r,GetStyle(),FALSE,GetExStyle());info->ptMinTrackSize={r.right-r.left,r.bottom-r.top};}
        HBRUSH Wizard::OnCtlColor(CDC* dc,CWnd*,UINT)
        {const auto p=NativeMaterial::Palette::Resolve(m_appearance);dc->SetTextColor(RGB(BYTE(p.onSurface.r*255),BYTE(p.onSurface.g*255),BYTE(p.onSurface.b*255)));dc->SetBkColor(RGB(BYTE(p.surface.r*255),BYTE(p.surface.g*255),BYTE(p.surface.b*255)));return static_cast<HBRUSH>(m_background.GetSafeHandle());}
        LRESULT Wizard::OnDpi(WPARAM dpi,LPARAM value)
        {m_dpi=HIWORD(dpi);const auto* r=reinterpret_cast<const RECT*>(value);SetWindowPos(nullptr,r->left,r->top,r->right-r->left,r->bottom-r->top,SWP_NOZORDER|SWP_NOACTIVATE);Fonts();Layout();return 0;}
    }
    bool Show(CWnd* owner,bool explicitResume)
    {
        if(!owner||!::IsWindow(owner->GetSafeHwnd())||active)return false;
        Draft draft=Load();if(draft.complete&&!explicitResume)return true;
        if(draft.complete){draft.complete=false;draft.step=0;draft.taskbar=draft.startup=0;draft.results={};if(auto* monitor=CTrafficMonitorDlg::Instance())draft.selection=monitor->SetupSelection();}
        struct ActiveScope { ActiveScope(){active=true;} ~ActiveScope(){active=false;} } activeScope;
        struct Template { DLGTEMPLATE dialog; WORD menu{},className{},title{}; } layout{};
        layout.dialog.style=WS_POPUP|WS_CAPTION|WS_SYSMENU|WS_THICKFRAME|WS_CLIPCHILDREN|DS_MODALFRAME;
        layout.dialog.dwExtendedStyle=WS_EX_CONTROLPARENT;layout.dialog.cx=540;layout.dialog.cy=440;
        Wizard wizard(draft);if(!wizard.InitModalIndirect(&layout.dialog,owner))return false;
        return wizard.DoModal()==IDOK;
    }
}
