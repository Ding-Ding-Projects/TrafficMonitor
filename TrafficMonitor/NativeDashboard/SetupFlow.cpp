#include "../stdafx.h"
#include "SetupFlow.h"
#include "../NativePreferences/Editor.h"
#include "../NativeMaterial/ActionSounds.h"
#include "../NativePreferences/SetupState.h"
#include "../NativePreferences/Subscription.h"
#include "../NativeSettings/SettingsViewport.h"
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
            NativePreferences::Draft preferences;bool protectedSchema{};
            int step{}, language{}, english{5}, cantonese{5}, taskbar{}, startup{};
            Selection selection;
            std::array<Result, 3> results{};
            bool complete{};
        };
        Draft Load()
        {
            Draft d;
            const auto accepted=NativePreferences::Current();d.preferences={accepted.values,accepted.base,false,false};
            d.language=static_cast<int>(accepted.values.language);
            if (auto* monitor = CTrafficMonitorDlg::Instance()) d.selection = monitor->SetupSelection();
            WIN32_FILE_ATTRIBUTE_DATA attributes{};
            if(!GetFileAttributesExW(Path().c_str(),GetFileExInfoStandard,&attributes)){d.protectedSchema=GetLastError()!=ERROR_FILE_NOT_FOUND;return d;}
            if(attributes.nFileSizeHigh||attributes.nFileSizeLow>16384){d.protectedSchema=true;return d;}
            CIniHelper ini(Path());
            const auto schemaText=ini.GetString(L"setup",L"schemaVersion",L"");const int schema=schemaText==L"1"?1:schemaText==L"2"?2:0;
            if(schema!=1&&schema!=2){d.protectedSchema=true;return d;}
            std::wstring selected;const bool hasLanguage=ini.GetString(L"setup",L"language",selected)&&NativePreferences::ParseSavedLanguage(selected,d.preferences.values.language);
            if(hasLanguage)d.preferences.values.language=static_cast<NativePreferences::Language>(selected[0]-L'0');
            if(schema==2)
            {
                if(!NativePreferences::ReadSetupPreferences(ini,d.preferences)){d.protectedSchema=true;return d;}
                // Action sounds are not part of setup progress; follow the accepted value so a resumed draft never reports a false conflict.
                d.preferences.values.actionSounds=d.preferences.base.values.actionSounds=accepted.values.actionSounds;
                NativePreferences::ReconcileResume(d.preferences,accepted);
            }
            else d.preferences.dirty=d.preferences.values!=accepted.values;

            d.step = Bound(ini.GetInt(L"setup", L"step", 0), 0, 3);
            d.language=static_cast<int>(d.preferences.values.language);
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
            if(d.protectedSchema)return false;
            const auto temp = Path() + L".tmp";
            DeleteFileW(temp.c_str()); CIniHelper output(temp);
            NativePreferences::WriteSetupPreferences(output,d.preferences);output.WriteInt(L"setup",L"step",d.step); output.WriteInt(L"setup", L"englishStyle", d.english);
            output.WriteInt(L"setup", L"cantoneseStyle", d.cantonese); output.WriteInt(L"setup", L"taskbar", d.taskbar);
            output.WriteInt(L"setup", L"startup", d.startup); output.WriteInt(L"setup", L"mode", static_cast<int>(d.selection.mode));
            output.WriteString(L"setup", L"adapter", d.selection.guid); output.WriteBool(L"setup", L"complete", d.complete);
            for (int i = 0; i < 3; ++i) { const auto key = L"result" + std::to_wstring(i); output.WriteInt(L"setup", key.c_str(), static_cast<int>(d.results[i])); }
            if (!output.Save() || !MoveFileExW(temp.c_str(), Path().c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) return false;
            CIniHelper stored(Path());
            return stored.GetInt(L"setup", L"schemaVersion", 0) == 2 && stored.GetInt(L"setup", L"step", -1) == d.step
                && stored.GetBool(L"setup", L"complete", !d.complete) == d.complete
                && stored.GetInt(L"setup",L"language",-1)==d.language
                && stored.GetInt(L"setup",L"appearance",-1)==static_cast<int>(d.preferences.values.appearance)
                && stored.GetInt(L"setup",L"baseLanguage",-1)==static_cast<int>(d.preferences.base.values.language)
                && stored.GetInt(L"setup",L"baseAppearance",-1)==static_cast<int>(d.preferences.base.values.appearance)
                && stored.GetString(L"setup",L"baseFingerprint",L"")==d.preferences.base.fingerprint
                && stored.GetString(L"setup",L"preferenceDirty",L"")==std::wstring(d.preferences.dirty?L"true":L"false");
        }
        bool StartupEnabled() { return theApp.GetAutoRun(nullptr, theApp.m_installed_mode ? false : theApp.m_general_data.auto_run_by_task_scheduler); }
        bool active{};
        // Setup ranges notify their immediate parent, unlike the forwarded commands.
        class StepViewport final : public NativeSettings::SettingsViewport
        {
            LRESULT WindowProc(UINT message, WPARAM w, LPARAM l) override
            {
                if(message==WM_HSCROLL)return GetParent()->SendMessage(message,w,l);
                return NativeSettings::SettingsViewport::WindowProc(message,w,l);
            }
        };
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
            afx_msg void OnSize(UINT, int, int) { if(!m_changingDpi)Layout(); }
            afx_msg void OnGetMinMaxInfo(MINMAXINFO* info);
            afx_msg HBRUSH OnCtlColor(CDC*, CWnd*, UINT);
            afx_msg LRESULT OnDpi(WPARAM, LPARAM);
            afx_msg void OnHScroll(UINT, UINT, CScrollBar*);
            DECLARE_MESSAGE_MAP()
        private:
            NativePreferences::Editor m_preferences;NativePreferences::Subscription m_subscription;
            StepViewport m_body;int m_scroll{},m_contentHeight{},m_viewHeight{},m_layoutStep{-1};
            LRESULT OnPreferences(WPARAM generation,LPARAM);
            LRESULT WindowProc(UINT message,WPARAM w,LPARAM l) override {if(message==WM_NCDESTROY)m_subscription.Detach();if(message==WM_SETTINGCHANGE||message==WM_THEMECHANGED)NativePreferences::RefreshSystemAppearance();return CDialog::WindowProc(message,w,l);}
            CString T(Copy id) const { return Text(id, m_draft.language); }
            int Px(int n) const { return MulDiv(n, m_dpi, 96); }
            void Labels(); void Layout(bool preserveAnchor=true); void RevealFocus(); void Fonts(); void ReadControls(); void ReloadAdapters();
            void Advance(); void Apply(); bool ValidAdapter(); void Review(); void Note(Copy id);
            Draft m_draft; Discovery m_discovery;
            UINT m_dpi{96}; bool m_loading{}, m_applying{}, m_changingDpi{};
            NativeMaterial::Appearance m_appearance{NativeMaterial::Appearance::Light};
            CFont m_font, m_titleFont, m_labelFont; CBrush m_background;
            CStatic m_heading, m_info, m_note, m_styleScope;
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
            ON_MESSAGE(NativePreferences::ChangedMessage,&Wizard::OnPreferences)
            ON_MESSAGE(WM_DPICHANGED, &Wizard::OnDpi)
        END_MESSAGE_MAP()
        BOOL Wizard::OnInitDialog()
        {
            CDialog::OnInitDialog(); m_dpi = GetDpiForWindow(m_hWnd); if (!m_dpi) m_dpi = 96;
            m_subscription.Attach(m_hWnd);m_appearance=NativePreferences::EffectiveAppearance();
            const auto palette = NativeMaterial::Palette::Resolve(m_appearance);
            m_background.CreateSolidBrush(RGB(BYTE(palette.surface.r*255), BYTE(palette.surface.g*255), BYTE(palette.surface.b*255)));
            CRect box(0, 0, 100, 40); DWORD child = WS_CHILD | WS_VISIBLE;
            if(!m_body.Create(this)||!m_preferences.Create(&m_body)){EndDialog(-1);return TRUE;}
            m_preferences.draft=m_draft.preferences;m_preferences.Refresh();
            m_body.background=[this]{return static_cast<HBRUSH>(m_background.GetSafeHandle());};
            m_body.scroll=[this](UINT code,int position){if(code==SB_LINEUP)m_scroll-=48;else if(code==SB_LINEDOWN)m_scroll+=48;else if(code==SB_PAGEUP)m_scroll-=m_viewHeight;else if(code==SB_PAGEDOWN)m_scroll+=m_viewHeight;else if(code==SB_THUMBTRACK||code==SB_THUMBPOSITION)m_scroll=position;else if(code==SB_TOP)m_scroll=0;else if(code==SB_BOTTOM)m_scroll=m_contentHeight;Layout(false);};
            m_heading.Create(L"", child, box, this); m_info.Create(L"", child, box, &m_body); m_note.Create(L"", child, box, this);
            m_styleScope.Create(L"",child,box,&m_body);
            m_review.Create(box, &m_body, 55030);m_review.ModifyStyle(WS_VSCROLL,0,SWP_FRAMECHANGED);m_review.SetAppearance(m_appearance);
            for (size_t i = 0; i < m_choices.size(); ++i)
            {
                m_labels[i].Create(L"", child, box, &m_body);
                if(i==1||i==2){m_ranges[i-1].Create(box,&m_body,Language+static_cast<UINT>(i));m_ranges[i-1].SetAppearance(m_appearance);}
                else{m_choices[i].Create(child|WS_TABSTOP|CBS_DROPDOWNLIST|WS_VSCROLL, box, &m_body, Language + static_cast<UINT>(i));m_choices[i].SetAppearance(m_appearance);}
            }
            m_back.Create(L"", box, this, Back); m_next.Create(L"", box, this, Next); m_save.Create(L"", box, this, SaveClose);
            m_refresh.Create(L"", box, &m_body, Refresh); m_keep.Create(L"", box, &m_body, KeepRemaining);
            m_back.SetVariant(NativeMaterial::ButtonVariant::Text);m_save.SetVariant(NativeMaterial::ButtonVariant::Text);
            for (auto* button : {&m_back, &m_next, &m_save, &m_refresh, &m_keep}) button->SetAppearance(m_appearance);
            Fonts(); Labels(); ReloadAdapters();
            RECT r{0,0,Px(760),Px(640)}; AdjustWindowRectEx(&r,GetStyle(),FALSE,GetExStyle());
            SetWindowPos(nullptr,0,0,r.right-r.left,r.bottom-r.top,SWP_NOMOVE|SWP_NOZORDER); CenterWindow(); Layout();
            if(m_draft.protectedSchema){Note(Copy::Protected);m_save.SetFocus();}else m_next.SetFocus(); return FALSE;
        }
        void Wizard::Fonts()
        {
            NONCLIENTMETRICS metrics{sizeof(metrics)}; LOGFONT font{};
            if (SystemParametersInfoW(SPI_GETNONCLIENTMETRICS,sizeof(metrics),&metrics,0)) font=metrics.lfMessageFont;
            else wcscpy_s(font.lfFaceName,L"Segoe UI"); font.lfHeight=-Px(14);
            auto next=CreateFontIndirectW(&font); if(!next)return;
            auto previous=static_cast<HFONT>(m_font.Detach());m_font.Attach(next);
            for(CWnd* child=GetWindow(GW_CHILD);child;child=child->GetNextWindow())child->SetFont(&m_font);
            for(CWnd* child=m_body.GetWindow(GW_CHILD);child;child=child->GetNextWindow())child->SetFont(&m_font);
            m_preferences.SetFont(&m_font);
            if(previous)DeleteObject(previous);
            m_titleFont.DeleteObject();font.lfHeight=-Px(22);font.lfWeight=FW_SEMIBOLD;m_titleFont.CreateFontIndirect(&font);m_heading.SetFont(&m_titleFont);
            m_labelFont.DeleteObject();font.lfHeight=-Px(14);font.lfWeight=FW_MEDIUM;m_labelFont.CreateFontIndirect(&font);
            for(auto& label:m_labels)label.SetFont(&m_labelFont);
        }
        void Wizard::Labels()
        {
            m_loading=true; SetWindowText(T(Copy::Title));
            std::array<bool,7> dropped{};
            for(int i:{0,3,4,5,6})dropped[i]=m_choices[i].GetDroppedState()!=FALSE;
            const Copy labels[] = {Copy::Language,Copy::EnglishStyle,Copy::CantoneseStyle,Copy::ModeLabel,Copy::AdapterLabel,Copy::TaskbarLabel,Copy::StartupLabel};
            for(size_t i=0;i<m_labels.size();++i)m_labels[i].SetWindowText(T(labels[i]));
            for (int i : {0,3,5,6}) m_choices[i].ResetContent();
            for(const auto* label:{L"English",L"廣東話",L"English / 廣東話"})m_choices[0].AddString(label);
            m_ranges[0].SetWindowText(T(Copy::EnglishStyle));m_ranges[1].SetWindowText(T(Copy::CantoneseStyle));
            for(auto label:{Copy::Automatic,Copy::All,Copy::One})m_choices[3].AddString(T(label));
            for(int i:{5,6})for(auto label:{Copy::Keep,Copy::Enable,Copy::Disable})m_choices[i].AddString(T(label));
            m_choices[0].SetCurSel(m_draft.language);m_ranges[0].SetPos(m_draft.english);m_ranges[1].SetPos(m_draft.cantonese);
            m_choices[3].SetCurSel(static_cast<int>(m_draft.selection.mode));m_choices[5].SetCurSel(m_draft.taskbar);m_choices[6].SetCurSel(m_draft.startup);
            m_back.SetLabel(T(Copy::Back));m_save.SetLabel(T(m_draft.protectedSchema?Copy::CloseUnsaved:Copy::SaveClose));m_refresh.SetLabel(T(Copy::Refresh));m_keep.SetLabel(T(Copy::KeepRemaining));
            for(int i:{0,3,4,5,6})if(dropped[i])m_choices[i].ShowDropDown(TRUE);
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
            m_draft.preferences=m_preferences.draft;m_draft.language=static_cast<int>(m_preferences.draft.values.language);
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
        void Wizard::Layout(bool preserveAnchor)
        {
            if(!m_next.GetSafeHwnd())return;
            HWND focus=::GetFocus();CRect before;
            const bool anchored=preserveAnchor&&m_layoutStep==m_draft.step&&focus&&::IsChild(m_body,focus)&&::IsWindowVisible(focus);
            if(anchored){::GetWindowRect(focus,&before);m_body.ScreenToClient(before);}
            if(m_layoutStep!=m_draft.step){m_scroll=0;m_layoutStep=m_draft.step;}
            CRect r;GetClientRect(r);const int width=MulDiv(r.Width(),96,m_dpi),height=MulDiv(r.Height(),96,m_dpi);
            const int bodyWidth=(std::min)(856,width-48),contentWidth=bodyWidth-24;
            auto place=[&](CWnd& child,int x,int y,int w,int h){child.MoveWindow(Px(x),Px(y),Px((std::max)(1,w)),Px((std::max)(1,h)));};
            auto textHeight=[&](CWnd& control,int available,int minimum)
            {
                CClientDC dc(&control);auto old=dc.SelectObject(control.GetFont());
                CString text;control.GetWindowText(text);CRect measured(0,0,Px((std::max)(1,available)),0);
                dc.DrawText(text,measured,DT_CALCRECT|DT_WORDBREAK|DT_NOPREFIX);dc.SelectObject(old);
                return (std::max)(minimum,(measured.Height()*96+static_cast<int>(m_dpi)-1)/static_cast<int>(m_dpi)+4);
            };
            m_heading.SetWindowText(T(static_cast<Copy>(static_cast<int>(Copy::LanguageStep)+m_draft.step)));
            m_info.SetWindowText(T(static_cast<Copy>(static_cast<int>(Copy::LanguageInfo)+m_draft.step)));
            m_styleScope.SetWindowText(T(Copy::StyleScope));
            m_next.SetLabel(T(m_draft.step==3?Copy::Apply:Copy::Next));
            const int headingHeight=textHeight(m_heading,width-48,32),bodyTop=16+headingHeight+12;
            place(m_heading,24,16,width-48,headingHeight);
            // Reserve the measured status and action region before allocating the body.
            const int buttonWidth=(width-72)/3;
            int buttonHeight=44;
            for(auto* button:{&m_back,&m_save,&m_next})buttonHeight=(std::max)(buttonHeight,textHeight(*button,buttonWidth-32,24)+20);
            CString note;m_note.GetWindowText(note);
            const int noteHeight=note.IsEmpty()?0:textHeight(m_note,width-48,20);
            const int buttonTop=height-18-buttonHeight,noteTop=buttonTop-(noteHeight?noteHeight+12:0);
            place(m_note,24,noteTop,width-48,noteHeight);m_note.ShowWindow(noteHeight?SW_SHOW:SW_HIDE);
            place(m_back,24,buttonTop,buttonWidth,buttonHeight);place(m_save,36+buttonWidth,buttonTop,buttonWidth,buttonHeight);place(m_next,48+2*buttonWidth,buttonTop,buttonWidth,buttonHeight);
            m_viewHeight=(std::max)(1,noteTop-12-bodyTop);place(m_body,24,bodyTop,bodyWidth,m_viewHeight);
            m_body.SetAppearance(m_appearance);m_body.SetScrollName(T(Copy::Title));
            int y=-m_scroll;
            const int infoHeight=textHeight(m_info,contentWidth,20);place(m_info,0,y,contentWidth,infoHeight);y+=infoHeight+16;
            y+=m_preferences.Layout(0,y,contentWidth,m_dpi,m_draft.step==0);
            for(int i=0;i<7;++i)
            {
                const bool visible=(m_draft.step==0&&i>0&&i<3)||(m_draft.step==1&&(i==3||i==4))||(m_draft.step==2&&i>=5);
                CWnd& choice=(i==1||i==2)?static_cast<CWnd&>(m_ranges[i-1]):static_cast<CWnd&>(m_choices[i]);
                m_labels[i].ShowWindow(visible?SW_SHOW:SW_HIDE);choice.ShowWindow(visible?SW_SHOW:SW_HIDE);
                if(visible)
                {
                    const int labelHeight=textHeight(m_labels[i],contentWidth,24);
                    place(m_labels[i],0,y,contentWidth,labelHeight);y+=labelHeight+8;
                    place(choice,0,y,contentWidth,(i==1||i==2)?48:220);y+=48+16;
                }
            }
            m_styleScope.ShowWindow(m_draft.step==0?SW_SHOW:SW_HIDE);
            if(m_draft.step==0){const int h=textHeight(m_styleScope,contentWidth,20);place(m_styleScope,0,y,contentWidth,h);y+=h+12;}
            m_choices[4].EnableWindow(m_draft.selection.mode==Mode::One&&!m_applying);
            m_refresh.ShowWindow(m_draft.step==1?SW_SHOW:SW_HIDE);
            if(m_draft.step==1){const int h=textHeight(m_refresh,contentWidth-32,24)+20;place(m_refresh,0,y,contentWidth,h);y+=h+12;}
            m_review.ShowWindow(m_draft.step>=2?SW_SHOW:SW_HIDE);
            if(m_draft.step>=2){Review();const int h=textHeight(m_review,contentWidth-24,120)+24;place(m_review,0,y,contentWidth,h);y+=h+16;}
            m_keep.ShowWindow(m_draft.step==3?SW_SHOW:SW_HIDE);
            if(m_draft.step==3){const int h=textHeight(m_keep,contentWidth-32,24)+20;place(m_keep,0,y,contentWidth,h);y+=h+12;}
            m_contentHeight=y+m_scroll;
            int nextScroll=m_scroll;
            if(anchored){CRect after;::GetWindowRect(focus,&after);m_body.ScreenToClient(after);nextScroll+=MulDiv(after.top-before.top,96,m_dpi);}
            nextScroll=Bound(nextScroll,0,(std::max)(0,m_contentHeight-m_viewHeight));
            if(nextScroll!=m_scroll){m_scroll=nextScroll;Layout(false);return;}
            m_body.SetMetrics(m_contentHeight,m_viewHeight,m_scroll);
            m_back.EnableWindow(m_draft.step>0&&!m_applying);m_save.EnableWindow(!m_applying);m_next.EnableWindow(!m_applying&&!m_draft.protectedSchema);
            RedrawWindow(nullptr,nullptr,RDW_INVALIDATE|RDW_ERASE|RDW_ALLCHILDREN);
        }
        void Wizard::RevealFocus()
        {
            const HWND focus=::GetFocus();if(!focus||!::IsChild(m_body,focus)||!::IsWindowVisible(focus))return;
            // The rail is fixed to the viewport, not part of its scrolling content.
            // Revealing it would move the range back after its own End/arrow action.
            if(focus==::GetDlgItem(m_body.GetSafeHwnd(),53998))return;
            CRect bounds;::GetWindowRect(focus,&bounds);m_body.ScreenToClient(bounds);
            const int top=MulDiv(bounds.top,96,m_dpi),bottom=MulDiv(bounds.bottom,96,m_dpi);
            const int delta=top<8?top-8:bottom>m_viewHeight-8?(std::min)(top-8,bottom-m_viewHeight+8):0;
            const int position=Bound(m_scroll+delta,0,(std::max)(0,m_contentHeight-m_viewHeight));
            if(position!=m_scroll){m_scroll=position;Layout(false);}
        }
        void Wizard::Note(Copy id)
        {
            m_note.SetWindowText(T(id)+L"\r\n"+StyleNote(m_draft.language,m_draft.english,m_draft.cantonese));
            Layout();
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
            CString previous;m_review.GetWindowText(previous);
            if(previous!=summary)
            {
                int start{},end{};m_review.GetSel(start,end);m_review.SetWindowText(summary);
                m_review.SetSel((std::min)(start,summary.GetLength()),(std::min)(end,summary.GetLength()),TRUE);
            }
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
            if(m_preferences.draft.dirty){m_draft.step=0;m_note.SetWindowText(NativePreferences::Text(NativePreferences::TextId::Draft));Layout();return;}
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
            if(m_draft.complete){NativeSounds::Notify(NativeSounds::Action::SetupCompleted);EndDialog(IDOK);}else{Layout();Note(Copy::Incomplete);}
        }
        void Wizard::OnHScroll(UINT code,UINT position,CScrollBar* bar)
        {
            CDialog::OnHScroll(code,position,bar);
            if(m_loading||m_applying)return;ReadControls();
            m_note.SetWindowText(StyleNote(m_draft.language,m_draft.english,m_draft.cantonese));Layout();
        }
        BOOL Wizard::OnCommand(WPARAM w,LPARAM l)
        {
            UINT id=LOWORD(w);if(m_applying)return TRUE;
            if(HIWORD(w)==BN_CLICKED&&m_preferences.Command(id)){ReadControls();Labels();Layout();RevealFocus();return TRUE;}
            if(id>=Language&&id<=StartupChoice&&HIWORD(w)==CBN_SELCHANGE)
            {ReadControls();Labels();Layout();m_note.SetWindowText(StyleNote(m_draft.language,m_draft.english,m_draft.cantonese));Layout();return TRUE;}
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
        LRESULT Wizard::OnPreferences(WPARAM generation,LPARAM)
        {
            if(!m_subscription.Accepts(generation))return 0;
            const HWND focus=::GetFocus();wchar_t className[32]{};
            const bool edit=focus&&::IsChild(m_body,focus)&&GetClassNameW(focus,className,_countof(className))&&_wcsicmp(className,L"Edit")==0;
            DWORD selectionStart{},selectionEnd{};
            if(edit)::SendMessage(focus,EM_GETSEL,reinterpret_cast<WPARAM>(&selectionStart),reinterpret_cast<LPARAM>(&selectionEnd));
            m_appearance=NativePreferences::EffectiveAppearance();
            const auto colors=NativeMaterial::Palette::Resolve(m_appearance);m_background.DeleteObject();
            m_background.CreateSolidBrush(RGB(BYTE(colors.surface.r*255),BYTE(colors.surface.g*255),BYTE(colors.surface.b*255)));
            m_preferences.Refresh();
            for(auto* button:{&m_back,&m_next,&m_save,&m_refresh,&m_keep})button->SetAppearance(m_appearance);
            m_review.SetAppearance(m_appearance);
            for(auto& range:m_ranges)range.SetAppearance(m_appearance);
            for(auto& choice:m_choices)if(choice.GetSafeHwnd())choice.SetAppearance(m_appearance);
            ReadControls();Labels();Layout();
            if(edit&&::IsWindow(focus))::SendMessage(focus,EM_SETSEL,selectionStart,selectionEnd);
            RevealFocus();return 0;
        }
        void Wizard::OnCancel()
        {if(m_applying)return;if(m_draft.protectedSchema){EndDialog(IDCANCEL);return;}ReadControls();m_draft.complete=false;if(Save(m_draft))EndDialog(IDCANCEL);else Note(Copy::SaveFailed);}
        BOOL Wizard::PreTranslateMessage(MSG* msg)
        {
            if(msg->message==WM_KEYDOWN&&msg->wParam==VK_ESCAPE)
            {HIMC context=ImmGetContext(msg->hwnd);bool composing=context&&ImmGetCompositionStringW(context,GCS_COMPSTR,nullptr,0)>0;if(context)ImmReleaseContext(msg->hwnd,context);if(composing)return FALSE;}
            const BOOL handled=CDialog::PreTranslateMessage(msg);
            if(msg->message==WM_KEYDOWN||msg->message==WM_KEYUP)RevealFocus();
            return handled;
        }
        void Wizard::OnGetMinMaxInfo(MINMAXINFO* info)
        {CDialog::OnGetMinMaxInfo(info);RECT r{0,0,Px(760),Px(640)};AdjustWindowRectEx(&r,GetStyle(),FALSE,GetExStyle());info->ptMinTrackSize={r.right-r.left,r.bottom-r.top};}
        HBRUSH Wizard::OnCtlColor(CDC* dc,CWnd*,UINT)
        {const auto p=NativeMaterial::Palette::Resolve(m_appearance);dc->SetTextColor(RGB(BYTE(p.onSurface.r*255),BYTE(p.onSurface.g*255),BYTE(p.onSurface.b*255)));dc->SetBkColor(RGB(BYTE(p.surface.r*255),BYTE(p.surface.g*255),BYTE(p.surface.b*255)));return static_cast<HBRUSH>(m_background.GetSafeHandle());}
        LRESULT Wizard::OnDpi(WPARAM dpi,LPARAM value)
        {
            const HWND focus=::GetFocus();const bool anchored=focus&&::IsChild(m_body,focus)&&::IsWindowVisible(focus);
            int anchor{};if(anchored){CRect bounds;::GetWindowRect(focus,&bounds);m_body.ScreenToClient(bounds);anchor=MulDiv(bounds.top,96,m_dpi);}
            m_changingDpi=true;m_dpi=HIWORD(dpi);if(!m_dpi)m_dpi=96;
            const auto* r=reinterpret_cast<const RECT*>(value);
            SetWindowPos(nullptr,r->left,r->top,r->right-r->left,r->bottom-r->top,SWP_NOZORDER|SWP_NOACTIVATE);
            Fonts();m_changingDpi=false;Layout(false);
            if(anchored)
            {
                CRect bounds;::GetWindowRect(focus,&bounds);m_body.ScreenToClient(bounds);
                m_scroll=Bound(m_scroll+MulDiv(bounds.top,96,m_dpi)-anchor,0,(std::max)(0,m_contentHeight-m_viewHeight));Layout(false);
            }
            RevealFocus();return 0;
        }
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
