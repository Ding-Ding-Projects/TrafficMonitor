#include "../stdafx.h"
#include "../NativeMaterial/ResponsivenessDiagnostics.h"
#include "Settings.h"
#include "../NativePreferences/Editor.h"
#include "../NativePreferences/Subscription.h"
#include "SettingCatalog.h"
#include "SettingsViewport.h"
#include "../NativeMaterial/Navigation.h"
#include "../NativeUpdate/SilentUpdater.h"
#include "../NativeMaterial/Controls.h"
#include "../NativeMaterial/SettingsControls.h"
#include "../NativeMenus/SearchableMenu.h"
#include "../NativeRegex/Workbench.h"
#include <array>
#include <imm.h>
#include <memory>

namespace NativeSettings
{
namespace
{
    unsigned activeEditors{};bool openInterface{};
    struct EditorLifetime { EditorLifetime(){++activeEditors;} ~EditorLifetime(){--activeEditors;} };
    enum : UINT { PageFirst=54001, Search=54010, Clear, Apply, Revert, Reset, Legacy, Theme, Regex, Results, OpenResult, ReturnFilter, FirstRow=54100 };
    constexpr int Interface=4, Discovery=5;
    struct Row
    {
        const Setting* setting{};
        NativeMaterial::SettingsCheck check;
        CStatic label,numberLabel,description,defaults;
        SettingEdit number;
        NativeMaterial::Button unit,details;
        bool expanded{},visible{};
        int top{},height{},labelHeight{},descriptionHeight{},defaultHeight{};
        std::array<CWnd*,8> Controls(){return {&check,&label,&numberLabel,&number,&unit,&details,&description,&defaults};}
    };
    struct SearchState { NativeRegex::Filter filter; int scroll{}; };
    class Editor final : public CDialog
    {
    public:
        Editor(int page,std::function<void(const Values&)> apply):m_page(openInterface?4:page==1?2:page==2?0:1),m_apply(std::move(apply)){}
    protected:
        BOOL OnInitDialog() override;
        BOOL OnCommand(WPARAM,LPARAM) override;
        void OnCancel() override;
        void OnOK() override {if(m_page==Discovery)OpenSelected();else ApplyDraft();}
        BOOL PreTranslateMessage(MSG*) override;
        afx_msg void OnPaint();
        afx_msg void OnTimer(UINT_PTR);
        afx_msg BOOL OnEraseBkgnd(CDC*){return TRUE;}
        afx_msg void OnSize(UINT,int,int){m_lastFocus=nullptr;Layout();}
        afx_msg void OnGetMinMaxInfo(MINMAXINFO*);
        afx_msg HBRUSH OnCtlColor(CDC*,CWnd*,UINT);
        afx_msg LRESULT OnDpi(WPARAM,LPARAM);
        afx_msg void OnContextMenu(CWnd*,CPoint);
        DECLARE_MESSAGE_MAP()
    private:
        CString T(const wchar_t* en,const wchar_t* zh)const{return NativePreferences::Pair(en,zh);}
        NativePreferences::Editor m_interface;NativePreferences::Subscription m_preferences;
        LRESULT OnPreferences(WPARAM,LPARAM);
        void Labels();
        LRESULT WindowProc(UINT message,WPARAM w,LPARAM l) override {if(message==WM_NCDESTROY)m_preferences.Detach();if(message==WM_SETTINGCHANGE||message==WM_THEMECHANGED){NativeDiagnostics::Scope messageDiagnostic(message==WM_SETTINGCHANGE?NativeDiagnostics::Operation::SettingsSettingMessage:NativeDiagnostics::Operation::SettingsThemeMessage);NativePreferences::RefreshSystemAppearance();}return CDialog::WindowProc(message,w,l);}
        int Px(int n)const{return MulDiv(n,m_dpi,96);}
        void Place(CWnd& c,int x,int y,int w,int h){c.MoveWindow(Px(x),Px(y),Px((std::max)(1,w)),Px((std::max)(1,h)));}
        void Layout();void SetFonts();void SetTheme();void Reload();void SyncRow(Row&);
        void RefreshRowText(Row&);
        void ApplyDraft();void ResetPage();bool CanDiscard();void SwitchGroup(int);
        void Scroll(UINT,int);void EnsureVisible(Row&);void Reveal(const std::wstring&);void OpenSelected();
        Row* Find(const std::wstring&);Row* Target(HWND,CPoint);
        void Menu(Row&,CPoint);void SetRowFrom(Row&,const Values&);void UpdateResults();
        void UpdateInfo();
        std::wstring DraftValue(const Row&,bool)const;
        std::vector<std::wstring> Subjects()const;
        enum class NoteKind { Loaded, Defaults, InvalidNumber, SaveFailed, Submitted, Explanation };
        void Note(NoteKind kind, const Setting* setting = nullptr)
        {
            m_noteKind = kind;
            m_noteSetting = setting;
            RefreshNote();
        }
        void RefreshNote();
        NoteKind m_noteKind{NoteKind::Loaded};
        const Setting* m_noteSetting{};
        int m_page{},m_contentHeight{},m_viewHeight{};UINT m_dpi{96};
        bool m_chinese{},m_loading{},m_dirty{},m_layout{};
        EditorBinding m_binding;HWND m_lastFocus{};
        std::wstring m_reveal;Values m_values;
        std::function<void(const Values&)> m_apply;
        NativeMaterial::Appearance m_appearance{NativeMaterial::Appearance::Light};
        CFont m_font,m_headingFont;CBrush m_background;CStatic m_title,m_note,m_status,m_updateInfo;NativeMaterial::TextField m_search;
        SettingsViewport m_body;NativeMaterial::ResultList m_results;
        NativeMaterial::Navigation m_navigation;
        NativeMaterial::Button m_clear,m_applyButton,m_revert,m_reset,m_legacy,m_theme,m_regex,m_open,m_return;
        std::array<SearchState,6> m_searches;NativeRegex::Editor m_regexEditor;
        std::vector<std::unique_ptr<Row>> m_rows;
    };
    BEGIN_MESSAGE_MAP(Editor,CDialog)
        ON_WM_PAINT()
        ON_WM_TIMER()
        ON_WM_ERASEBKGND()
        ON_WM_SIZE()
        ON_WM_GETMINMAXINFO()
        ON_WM_CTLCOLOR()
        ON_WM_CONTEXTMENU()
        ON_MESSAGE(WM_DPICHANGED,&Editor::OnDpi)
        ON_MESSAGE(NativePreferences::ChangedMessage,&Editor::OnPreferences)
    END_MESSAGE_MAP()
    BOOL Editor::OnInitDialog()
    {
        CDialog::OnInitDialog();m_dpi=GetDpiForWindow(m_hWnd);if(!m_dpi)m_dpi=96;
        openInterface=false;m_preferences.Attach(m_hWnd);m_chinese=NativePreferences::Current().values.language==NativePreferences::Language::Cantonese;
        m_appearance=NativePreferences::EffectiveAppearance();
        SetWindowText(T(L"TrafficMonitor settings",L"TrafficMonitor 設定"));SetIcon(theApp.GetMenuIcon(IDI_SETTINGS),FALSE);
        const CRect box(0,0,100,40);const DWORD child=WS_CHILD|WS_VISIBLE;
        m_title.Create(T(L"Settings",L"設定"),child,box,this);m_note.Create(L"",child,box,this);m_status.Create(L"",child,box,this);
        m_theme.Create(T(L"Appearance",L"外觀"),box,this,Theme);
        m_navigation.Create(box,this,PageFirst,true);
        m_search.CreateEx(WS_EX_CLIENTEDGE,L"EDIT",L"",child|WS_TABSTOP|ES_AUTOHSCROLL,box,this,Search);m_search.SetLimitText(256);
        m_search.SendMessage(EM_SETCUEBANNER,TRUE,reinterpret_cast<LPARAM>(m_chinese?L"搜尋名稱、解說同草稿數值":L"Search labels, explanations and draft values"));
        m_regex.Create(T(L"Regex",L"正規式"),box,this,Regex);m_clear.Create(T(L"Clear",L"清除"),box,this,Clear);
        m_applyButton.Create(T(L"Apply",L"套用"),box,this,Apply);m_revert.Create(T(L"Revert",L"還原"),box,this,Revert);m_reset.Create(T(L"Reset group draft",L"重設本組草稿"),box,this,Reset);
        m_legacy.Create(T(L"All settings (legacy)",L"所有設定（舊版）"),box,this,Legacy);m_open.Create(T(L"Open selected setting",L"開啟所選設定"),box,this,OpenResult);m_return.Create(T(L"Return to group filter",L"返回本組篩選"),box,this,ReturnFilter);
        if(!m_body.Create(this)){EndDialog(-1);return TRUE;}
        if(!m_interface.Create(&m_body,this)){EndDialog(-1);return TRUE;}
        m_body.scroll=[this](UINT code,int pos){Scroll(code,pos);};m_body.background=[this]{return static_cast<HBRUSH>(m_background.GetSafeHandle());};
        m_updateInfo.Create(L"",child,box,&m_body);
        m_results.Create(box,&m_body,Results);
        m_navigation.SetWindowText(T(L"Settings sections",L"設定分類"));m_body.SetScrollName(T(L"Scroll settings",L"捲動設定"));m_results.SetWindowText(T(L"Settings in all groups; select a result and choose Open",L"所有組別設定；選擇結果後按開啟"));
        size_t index=0;
        for(const auto& setting:Catalog())
        {
            auto row=std::make_unique<Row>();row->setting=&setting;UINT id=FirstRow+static_cast<UINT>(index++)*8;
            if(setting.enabled)row->check.Create(T(setting.english,setting.chinese),&m_body,id);else row->label.Create(T(setting.english,setting.chinese),child|SS_NOTIFY,box,&m_body,id);
            if(setting.number)
            {
                CString label;label=NativePreferences::Format(L"Value (%d to %d)",L"數值（%d 至 %d）",setting.minimum,setting.maximum);row->numberLabel.Create(label,child|SS_NOTIFY,box,&m_body,id+1);
                row->number.CreateEx(WS_EX_CLIENTEDGE,L"EDIT",L"",child|WS_TABSTOP|ES_AUTOHSCROLL,box,&m_body,id+2);row->number.SetLimitText(64);
                row->unit.Create(setting.trafficUnits?L"MB":setting.unit,box,&m_body,id+3);
            }
            row->details.Create(T(L"Details",L"詳情"),box,&m_body,id+4);row->description.Create(L"",child|SS_NOTIFY,box,&m_body,id+5);row->defaults.Create(L"",child|SS_NOTIFY,box,&m_body,id+6);m_rows.push_back(std::move(row));
        }
        SetFonts();Labels();SetTheme();Reload();UpdateInfo();
        RECT size{0,0,Px(1280),Px(840)};AdjustWindowRectEx(&size,GetStyle(),FALSE,GetExStyle());SetWindowPos(nullptr,0,0,size.right-size.left,size.bottom-size.top,SWP_NOMOVE|SWP_NOZORDER);CenterWindow();Layout();SetTimer(0x4E92,40,nullptr);m_search.SetFocus();return FALSE;
    }
    void Editor::Labels()
    {
        SetWindowText(T(L"TrafficMonitor settings",L"TrafficMonitor 設定"));m_title.SetWindowText(T(L"Settings",L"設定"));m_theme.SetLabel(NativePreferences::Text(NativePreferences::TextId::Interface));
        m_loading=true;m_navigation.ResetContent();for(int i=0;i<6;++i)m_navigation.AddString(T(GroupName(i,false),GroupName(i,true)));m_navigation.SetCurSel(m_page);m_loading=false;
        m_clear.SetLabel(T(L"Clear",L"清除"));m_regex.SetLabel(T(L"Regex",L"正規式"));m_applyButton.SetLabel(T(L"Apply",L"套用"));m_revert.SetLabel(T(L"Revert",L"還原"));m_reset.SetLabel(T(L"Reset group draft",L"重設本組草稿"));m_legacy.SetLabel(T(L"All settings (legacy)",L"所有設定（舊版）"));m_open.SetLabel(T(L"Open selected setting",L"開啟所選設定"));m_return.SetLabel(T(L"Return to group filter",L"返回本組篩選"));
        const auto mode=NativePreferences::Current().values.language;const CString cue=mode==NativePreferences::Language::Bilingual?L"Find a setting · 搜尋設定":mode==NativePreferences::Language::Cantonese?L"搜尋設定":L"Find a setting";m_search.SendMessage(EM_SETCUEBANNER,TRUE,reinterpret_cast<LPARAM>(cue.GetString()));
        for(auto& r:m_rows){const auto& s=*r->setting;if(s.enabled)r->check.SetWindowText(T(s.english,s.chinese));else r->label.SetWindowText(T(s.english,s.chinese));r->description.SetWindowText(T(s.explanation,s.chineseExplanation));r->details.SetLabel(T(L"Details",L"詳情"));RefreshRowText(*r);}
        m_navigation.SetWindowText(T(L"Settings sections",L"設定分類"));m_body.SetScrollName(T(L"Scroll settings",L"捲動設定"));RefreshNote();m_interface.Refresh();
    }
    LRESULT Editor::OnPreferences(WPARAM generation,LPARAM)
    {
        if (!m_preferences.Accepts(generation)) return 0;
        NativeDiagnostics::Scope diagnostic(NativeDiagnostics::Operation::SettingsReceive);
        m_chinese = NativePreferences::Current().values.language == NativePreferences::Language::Cantonese;
        m_appearance = NativePreferences::EffectiveAppearance();
        Labels();
        SetTheme();
        m_lastFocus = nullptr;
        Layout();
        return 0;
    }
    void Editor::SetFonts()
    {
        LOGFONT font{};NONCLIENTMETRICS metrics{sizeof(metrics)};if(SystemParametersInfoW(SPI_GETNONCLIENTMETRICS,sizeof(metrics),&metrics,0))font=metrics.lfMessageFont;else wcscpy_s(font.lfFaceName,L"Segoe UI");font.lfHeight=-Px(14);
        HFONT next=CreateFontIndirectW(&font);if(!next)return;HFONT old=static_cast<HFONT>(m_font.Detach());m_font.Attach(next);
        ::EnumChildWindows(m_hWnd,[](HWND child,LPARAM font)->BOOL{::SendMessageW(child,WM_SETFONT,font,TRUE);return TRUE;},reinterpret_cast<LPARAM>(next));if(old)DeleteObject(old);m_interface.SetFont(&m_font);font.lfHeight=-Px(24);font.lfWeight=FW_SEMIBOLD;HFONT heading=CreateFontIndirectW(&font);if(heading){HFONT previous=static_cast<HFONT>(m_headingFont.Detach());m_headingFont.Attach(heading);m_title.SetFont(&m_headingFont);if(previous)DeleteObject(previous);}
    }
    void Editor::SetTheme()
    {
        NativeMaterial::ApplyNativeFrameAppearance(m_hWnd, m_appearance);
        const auto c=NativeMaterial::Palette::Resolve(m_appearance);m_background.DeleteObject();m_background.CreateSolidBrush(RGB(BYTE(c.surface.r*255),BYTE(c.surface.g*255),BYTE(c.surface.b*255)));
        m_navigation.SetAppearance(m_appearance);m_results.SetAppearance(m_appearance);m_body.SetAppearance(m_appearance);m_search.SetAppearance(m_appearance);m_revert.SetVariant(NativeMaterial::ButtonVariant::Tonal);m_reset.SetVariant(NativeMaterial::ButtonVariant::Text);m_clear.SetVariant(NativeMaterial::ButtonVariant::Text);m_regex.SetVariant(NativeMaterial::ButtonVariant::Outlined);m_legacy.SetVariant(NativeMaterial::ButtonVariant::Text);for(auto* b:{&m_clear,&m_applyButton,&m_revert,&m_reset,&m_legacy,&m_theme,&m_regex,&m_open,&m_return})b->SetAppearance(m_appearance);
        for(auto& r:m_rows){if(r->check.GetSafeHwnd())r->check.SetAppearance(m_appearance);r->details.SetAppearance(m_appearance);if(r->setting->number){r->unit.SetAppearance(m_appearance);r->number.SetAppearance(m_appearance);}r->details.SetVariant(NativeMaterial::ButtonVariant::Text);}RedrawWindow(nullptr,nullptr,RDW_INVALIDATE|RDW_ALLCHILDREN);
    }
    void Editor::SyncRow(Row& r)
    {
        const auto& s=*r.setting;
        if(s.enabled){r.check.SetCheck(m_values.*s.enabled?BST_CHECKED:BST_UNCHECKED);r.check.EnableWindow(s.Available());r.check.Invalidate();}
        if(s.number){r.number.SetWindowText(std::to_wstring(m_values.*s.number).c_str());r.number.EnableWindow(s.Available());if(s.trafficUnits)r.unit.SetLabel(m_values.trafficUnit==0?CString(L"MB"):m_values.trafficUnit==1?CString(L"GB"):T(L"Invalid unit",L"無效單位"));r.unit.EnableWindow(s.trafficUnits&&s.Available());}
        RefreshRowText(r);
    }
    void Editor::RefreshRowText(Row& row)
    {
        const auto& setting = *row.setting;
        if (setting.number)
            row.numberLabel.SetWindowText(NativePreferences::Format(L"Value (%d to %d)", L"數值（%d 至 %d）", setting.minimum, setting.maximum));
        const Values defaults;
        const CString english = L"Default: " + CString(FormatValue(setting, defaults, false).c_str()) + L". Current source: configuration/default provenance unavailable.";
        const CString chinese = L"預設：" + CString(FormatValue(setting, defaults, true).c_str()) + L"。目前來源：未能區分設定檔同預設來源。";
        CString text = T(english, chinese);
        if (!setting.Available())
            text = T(L"Temperature monitoring is unavailable in this build. Stored preferences are retained.", L"呢個版本未提供溫度監察；已儲存嘅偏好會保留。");
        row.defaults.SetWindowText(text);
        row.description.SetWindowText(T(setting.explanation, setting.chineseExplanation));
    }
    void Editor::RefreshNote()
    {
        CString text;
        switch (m_noteKind)
        {
        case NoteKind::Loaded:
            text = T(L"Applied values loaded. Apply saves drafts. Remaining controls are in All settings (legacy).", L"已載入目前套用值；按套用先會儲存草稿。其他控制項喺所有設定（舊版）。");
            break;
        case NoteKind::Defaults:
            text = T(L"Defaults staged for this whole group, including filtered rows. Apply to save; unavailable controls stay unchanged.", L"已重設本組全部草稿，包括篩選隱藏項目。按套用先會儲存；未提供嘅控制項保持不變。");
            break;
        case NoteKind::SaveFailed:
            text = T(L"Restart preference could not be saved and read back. Draft retained; other settings were not applied.", L"未能儲存並讀回重新啟動偏好。草稿已保留，其他設定未套用。");
            break;
        case NoteKind::Submitted:
            text = T(L"Submitted to the existing settings consumer; current values reloaded. This is not configuration-file readback proof and does not send a test notification.", L"已交畀原有設定流程處理，再載入目前數值。呢個結果未證明設定檔讀回成功，亦唔會發出測試通知。");
            break;
        case NoteKind::Explanation:
            if (m_noteSetting) text = T(m_noteSetting->explanation, m_noteSetting->chineseExplanation);
            break;
        case NoteKind::InvalidNumber:
            if (m_noteSetting)
                text = NativePreferences::Format(L"%s: enter a whole value from %d to %d, step %d, with a valid unit. Nothing was applied.", L"%s：請輸入 %d 至 %d 嘅整數，步距 %d，同有效單位。尚未套用任何更改。", T(m_noteSetting->english, m_noteSetting->chinese).GetString(), m_noteSetting->minimum, m_noteSetting->maximum, m_noteSetting->step);
            break;
        }
        m_note.SetWindowText(text);
    }
    void Editor::Reload(){m_loading=true;m_values=Values::Current();for(auto& r:m_rows)SyncRow(*r);m_loading=false;m_dirty=false;Note(NoteKind::Loaded);Layout();}
    Row* Editor::Find(const std::wstring& id){for(auto& r:m_rows)if(id==r->setting->id)return r.get();return nullptr;}
    std::wstring Editor::DraftValue(const Row& row,bool chinese)const
    {
        const auto& s=*row.setting;std::wstring result;
        if(s.group==Interface){const bool appearance=std::wstring(s.id)==L"interface.appearance";const auto id=static_cast<NativePreferences::TextId>(static_cast<int>(appearance?NativePreferences::TextId::System:NativePreferences::TextId::English)+(appearance?static_cast<int>(m_interface.draft.values.appearance):static_cast<int>(m_interface.draft.values.language)));return NativePreferences::Text(id,chinese?NativePreferences::Language::Cantonese:NativePreferences::Language::English).GetString();}
        if(s.enabled)result=m_values.*s.enabled?(chinese?L"已啟用":L"Enabled"):(chinese?L"已停用":L"Disabled");
        if(s.number){CString text;row.number.GetWindowText(text);if(!result.empty())result+=L"; ";result+=text.GetString();result+=L" ";if(s.trafficUnits)result+=m_values.trafficUnit==0?L"MB":m_values.trafficUnit==1?L"GB":(chinese?L"無效單位":L"Invalid unit");else if(s.unit)result+=s.unit;}
        return result;
    }
    std::vector<std::wstring> Editor::Subjects()const
    {
        std::vector<std::wstring> values;
        for(const auto& r:m_rows)
        {
            const auto& s=*r->setting;std::wstring text=s.id;
            for(const auto* part:{GroupName(s.group,false),GroupName(s.group,true),s.english,s.chinese,s.explanation,s.chineseExplanation}){text+=L" ";text+=part;}
            text+=L" "+DraftValue(*r,false)+L" "+DraftValue(*r,true);
            if(!s.Available())text+=L" Unavailable 未提供";values.push_back(std::move(text));
        }
        return values;
    }
    void Editor::UpdateResults()
    {
        std::wstring selected;const int old=m_results.GetCurSel();if(old!=LB_ERR){auto* s=static_cast<Setting*>(m_results.GetItemDataPtr(old));if(s)selected=s->id;}
        m_results.SetRedraw(FALSE);m_results.ResetContent();
        for(size_t i=0;i<m_rows.size();++i)if(m_searches[Discovery].filter.Visible(i))
        {
            const auto& s = *m_rows[i]->setting;
            const CString english = CString(GroupName(s.group, false)) + L" > " + s.english + L" : " + DraftValue(*m_rows[i], false).c_str();
            const CString chinese = CString(GroupName(s.group, true)) + L" > " + s.chinese + L"：" + DraftValue(*m_rows[i], true).c_str();
            CString label = T(english, chinese);
            const int at=m_results.AddString(label);if(at>=0){m_results.SetItemDataPtr(at,const_cast<Setting*>(&s));if(selected==s.id)m_results.SetCurSel(at);}
        }
        if(m_results.GetCurSel()==LB_ERR&&m_results.GetCount())m_results.SetCurSel(0);m_results.SetRedraw(TRUE);m_results.Invalidate();::NotifyWinEvent(EVENT_OBJECT_REORDER,m_results.GetSafeHwnd(),OBJID_CLIENT,CHILDID_SELF);
    }
    void Editor::Layout()
    {
        if(!m_body.GetSafeHwnd()||m_layout)return;m_layout=true;
        CRect client;GetClientRect(client);const int width=MulDiv(client.Width(),96,m_dpi),height=MulDiv(client.Height(),96,m_dpi);
        CClientDC headerDC(this);auto* headerFont=headerDC.SelectObject(&m_font);
        auto extent=[&](CWnd& c,int w,int minimum){CString text;c.GetWindowText(text);CRect bounds(0,0,Px(w-32),0);headerDC.DrawText(text,bounds,DT_CALCRECT|DT_WORDBREAK|DT_NOPREFIX);return (std::max)(minimum,MulDiv(bounds.Height(),96,m_dpi)+20);};
        const int left=200,cw=(std::min)(860,width-left-24);
        headerDC.SelectObject(&m_headingFont);const int titleHeight=extent(m_title,cw,40);headerDC.SelectObject(&m_font);const int searchY=20+titleHeight+16;
        Place(m_navigation,12,searchY,176,height-searchY-88);Place(m_legacy,12,height-80,176,64);m_theme.ShowWindow(SW_HIDE);
        Place(m_title,left,20,cw,titleHeight);
        const int searchHeight=(std::max)(extent(m_regex,88,44),extent(m_clear,76,44));
        Place(m_search,left,searchY,cw-180,searchHeight);Place(m_regex,left+cw-172,searchY,88,searchHeight);Place(m_clear,left+cw-76,searchY,76,searchHeight);
        const int statusY=searchY+searchHeight+8,statusHeight=(std::max)(extent(m_status,cw-184,32),extent(m_open,176,40));
        Place(m_status,left,statusY,cw-184,statusHeight);Place(m_open,left+cw-176,statusY,176,statusHeight);Place(m_return,left+cw-176,statusY,176,statusHeight);
        m_open.ShowWindow(m_page==Discovery?SW_SHOW:SW_HIDE);m_return.ShowWindow(!m_reveal.empty()?SW_SHOW:SW_HIDE);
        int footerHeight=0;
        if(m_page==Interface)footerHeight=m_interface.LayoutActions(left,0,cw,m_dpi,true)+12;
        else footerHeight=extent(m_note,cw,32)+(std::max)({extent(m_applyButton,96,40),extent(m_revert,96,40),extent(m_reset,cw-216,40)})+32;
        const int footerY=height-footerHeight,bodyY=statusY+statusHeight+12;m_viewHeight=(std::max)(40,footerY-bodyY-12);
        Place(m_body,left,bodyY,cw,m_viewHeight);m_note.ShowWindow(m_page==Interface?SW_HIDE:SW_SHOW);
        if(m_page!=Interface){const int nh=extent(m_note,cw,32);Place(m_note,left,footerY,cw,nh);const int by=footerY+nh+8,bh=footerHeight-nh-20;Place(m_applyButton,left,by,96,bh);Place(m_revert,left+104,by,96,bh);Place(m_reset,left+208,by,cw-208,bh);}
        for(auto* c:{&m_applyButton,&m_revert,&m_reset})c->ShowWindow(m_page==Interface?SW_HIDE:SW_SHOW);m_reset.EnableWindow(m_page!=Discovery);
        headerDC.SelectObject(headerFont);
        auto& state=m_searches[m_page];state.filter.Request(Subjects());int y=0,count=0;bool showLanguage=false,showAppearance=false;CRect body;m_body.GetClientRect(body);const int bw=MulDiv(body.Width(),96,m_dpi);
        CClientDC dc(&m_body);auto* old=dc.SelectObject(&m_font);
        auto textHeight=[&](CWnd& c,int w){CString text;c.GetWindowText(text);CRect r(0,0,Px(w),0);dc.DrawText(text,&r,DT_CALCRECT|DT_WORDBREAK|DT_NOPREFIX);return (std::max)(24,(r.Height()*96+static_cast<int>(m_dpi)-1)/static_cast<int>(m_dpi));};
        for(size_t i=0;i<m_rows.size();++i)
        {
            auto& r=*m_rows[i];const auto& s=*r.setting;if(m_page==Interface&&s.group==Interface&&(state.filter.Visible(i)||m_reveal==s.id)){++count;if(std::wstring(s.id)==L"interface.appearance")showAppearance=true;else showLanguage=true;}r.visible=m_page!=Discovery&&m_page!=Interface&&s.group==m_page&&(state.filter.Visible(i)||m_reveal==s.id);
            if(r.visible){r.top=y;r.labelHeight=(std::max)(52,textHeight(s.enabled?static_cast<CWnd&>(r.check):static_cast<CWnd&>(r.label),bw-170)+24);r.defaultHeight=textHeight(r.defaults,bw-24);r.descriptionHeight=r.expanded?textHeight(r.description,bw-24):0;r.height=r.labelHeight+8+(s.number?48:0)+r.defaultHeight+12+(r.expanded?r.descriptionHeight+12:0);y+=r.height+12;++count;}
        }
        const int updateTop=y;const int updateHeight=m_page==0?textHeight(m_updateInfo,bw-24)+16:0;y+=updateHeight;
        dc.SelectObject(old);m_interface.SetFilter(showLanguage,showAppearance);if(m_page==Interface)y=m_interface.Layout(0,0,bw-12,m_dpi,true);else m_interface.Layout(0,0,bw-12,m_dpi,false);m_contentHeight=y;state.scroll=(std::max)(0,(std::min)(state.scroll,(std::max)(0,y-m_viewHeight)));
        if(m_page==Interface){m_interface.Layout(0,-state.scroll,bw-12,m_dpi,true);m_interface.LayoutActions(left,footerY,cw,m_dpi,true);}
        m_body.SetMetrics(y,m_viewHeight,state.scroll);
        for(auto& pointer:m_rows)
        {
            auto& r=*pointer;const auto& s=*r.setting;for(auto* c:r.Controls())if(c->GetSafeHwnd())c->ShowWindow(r.visible?SW_SHOW:SW_HIDE);if(!r.visible)continue;
            const int top=r.top-state.scroll;Place(s.enabled?static_cast<CWnd&>(r.check):static_cast<CWnd&>(r.label),0,top,bw-116,r.labelHeight);Place(r.details,bw-108,top+4,96,40);int at=top+r.labelHeight+8;
            if(s.number){Place(r.numberLabel,12,at+4,bw-280,36);Place(r.number,bw-264,at,136,36);Place(r.unit,bw-116,at-2,104,40);at+=48;}
            if(r.expanded){Place(r.description,12,at,bw-24,r.descriptionHeight);at+=r.descriptionHeight+12;}else r.description.ShowWindow(SW_HIDE);Place(r.defaults,12,at,bw-24,r.defaultHeight);
        }
        m_results.ShowWindow(m_page==Discovery?SW_SHOW:SW_HIDE);
        m_updateInfo.ShowWindow(m_page==0?SW_SHOW:SW_HIDE);if(m_page==0)Place(m_updateInfo,12,updateTop-state.scroll,bw-24,updateHeight);
        if(m_page==Discovery){Place(m_results,0,0,bw,m_viewHeight);UpdateResults();count=m_results.GetCount();m_body.SetMetrics(0,m_viewHeight,0);m_results.SyncScroll();}
        CString status;status=NativePreferences::Format(L"%s: %d settings shown",L"%s：顯示 %d 項設定",GroupName(m_page,m_chinese),count);
        if(NativePreferences::Current().values.language==NativePreferences::Language::Bilingual){CString en,zh;en.Format(L"%s: %d settings shown",GroupName(m_page,false),count);zh.Format(L"%s：顯示 %d 項設定",GroupName(m_page,true),count);status=en+L"\r\n"+zh;}
        if(!count)status=T(L"No matches. Clear this group's search or choose All groups.",L"冇符合項目。清除本組搜尋，或者選所有組別。");if(!m_reveal.empty())status+=T(L"; selected setting revealed",L"；已顯示所選設定");
        if(!state.filter.status.empty())status+=L" | "+CString(state.filter.status.c_str());m_status.SetWindowText(status);m_layout=false;m_body.Invalidate();Invalidate(FALSE);
    }
    void Editor::Scroll(UINT code,int position)
    {
        if(m_page==Discovery)return;auto& offset=m_searches[m_page].scroll;
        switch(code){case SB_LINEUP:offset-=48;break;case SB_LINEDOWN:offset+=48;break;case SB_PAGEUP:offset-=m_viewHeight;break;case SB_PAGEDOWN:offset+=m_viewHeight;break;case SB_THUMBTRACK:case SB_THUMBPOSITION:offset=position;break;case SB_TOP:offset=0;break;case SB_BOTTOM:offset=m_contentHeight;break;default:return;}Layout();
    }
    void Editor::UpdateInfo()
    {
        const auto status=NativeUpdate::GetStatus();CString phase;
        switch(status.phase)
        {
        case NativeUpdate::Phase::Disabled:phase=T(L"Disabled",L"已停用");break;
        case NativeUpdate::Phase::Idle:phase=T(L"Waiting for next check",L"等候下次檢查");break;
        case NativeUpdate::Phase::Checking:phase=T(L"Checking",L"檢查中");break;
        case NativeUpdate::Phase::Downloading:phase=T(L"Downloading",L"下載中");break;
        case NativeUpdate::Phase::Ready:phase=T(L"Ready; restart deferred while Settings is open",L"已就緒；設定開啟期間延後重新啟動");break;
        case NativeUpdate::Phase::Applying:phase=T(L"Applying",L"套用中");break;
        case NativeUpdate::Phase::Failed:phase=NativePreferences::Format(L"Update unavailable (code %lu)",L"更新暫時不可用（代碼 %lu）",status.error);break;
        default:phase=T(L"Stopped",L"已停止");break;
        }
        CString text=T(L"Automatic checks run every five minutes. Disabling automatic restart still permits staging for next launch. Current service: ",L"自動檢查每五分鐘執行一次。關閉自動重新啟動仍可準備下次啟動嘅更新。目前狀態：");text+=phase;
        CString old;m_updateInfo.GetWindowText(old);if(old!=text){m_updateInfo.SetWindowText(text);Layout();}
    }
    void Editor::EnsureVisible(Row& r)
    {if(!r.visible)return;auto& offset=m_searches[m_page].scroll;int next=offset;if(r.top<offset)next=r.top;else if(r.top+r.height>offset+m_viewHeight)next=r.height>m_viewHeight?r.top:r.top+r.height-m_viewHeight;if(next!=offset){offset=next;Layout();}}
    void Editor::SwitchGroup(int group)
    {if(group<0||group>Discovery)return;m_regexEditor.Close();m_binding.Invalidate();m_reveal.clear();m_page=group;m_navigation.SetCurSel(group);m_loading=true;m_search.SetWindowText(m_searches[group].filter.options.query.c_str());m_loading=false;Layout();m_search.SetFocus();}
    void Editor::Reveal(const std::wstring& id)
    {auto* r=Find(id);if(!r)return;SwitchGroup(r->setting->group);if(r->setting->group==Interface){m_reveal=id;Layout();m_interface.Focus(id==L"interface.appearance");return;}m_reveal=id;Layout();EnsureVisible(*r);if(r->setting->number&&r->number.IsWindowEnabled())r->number.SetFocus();else if(r->check.GetSafeHwnd()&&r->check.IsWindowEnabled())r->check.SetFocus();else r->details.SetFocus();}
    void Editor::OpenSelected(){const int at=m_results.GetCurSel();if(at==LB_ERR)return;auto* s=static_cast<Setting*>(m_results.GetItemDataPtr(at));if(s)Reveal(s->id);}
    void Editor::SetRowFrom(Row& row,const Values& source){row.setting->Copy(m_values,source);m_loading=true;SyncRow(row);m_loading=false;m_dirty=true;Layout();EnsureVisible(row);}
    void Editor::ResetPage()
    {if(m_page==Discovery)return;const Values defaults;m_loading=true;for(auto& r:m_rows)if(r->setting->group==m_page){r->setting->Copy(m_values,defaults);SyncRow(*r);}m_loading=false;m_dirty=true;Layout();Note(NoteKind::Defaults);}
    void Editor::ApplyDraft()
    {
        if(m_page==Interface){m_interface.Command(NativePreferences::Editor::ApplyChoice);Layout();return;}
        Values candidate=m_values;
        for(const auto& r:m_rows)
        {
            const auto& s=*r->setting;if(!s.number)continue;CString text;r->number.GetWindowText(text);int value=candidate.*s.number;
            if(!ReadNumberDraft(text.GetString(),s.minimum,s.maximum,s.step,s.Available(),value)||(s.trafficUnits&&!ValidTrafficUnit(candidate.trafficUnit,s.Available())))
            {Reveal(s.id);Note(NoteKind::InvalidNumber, &s);return;}candidate.*s.number=value;
        }
        if(candidate.automaticRestart!=NativeUpdate::AutomaticRestartEnabled()&&!NativeUpdate::SaveAutomaticRestart(candidate.automaticRestart))
        {m_dirty=true;Note(NoteKind::SaveFailed);return;}
        m_apply(candidate);Reload();Note(NoteKind::Submitted);
    }
    bool Editor::CanDiscard(){return (!m_dirty&&!m_interface.draft.dirty)||MessageBox(T(L"Discard unapplied settings changes?",L"捨棄未套用嘅設定修改？"),T(L"Settings",L"設定"),MB_YESNO|MB_ICONQUESTION|MB_DEFBUTTON2)==IDYES;}
    void Editor::OnCancel(){if(CanDiscard()){m_regexEditor.Close();m_binding.Invalidate();EndDialog(IDCANCEL);}}
    Row* Editor::Target(HWND window,CPoint point)
    {
        for(auto& r:m_rows)if(r->visible)
        {
            for(auto* c:r->Controls())if(c->GetSafeHwnd()&&window==c->GetSafeHwnd())return r.get();
            if(point!=CPoint(-1,-1)){CRect body;m_body.GetWindowRect(body);CRect bounds(body.left,body.top+Px(r->top-m_searches[m_page].scroll),body.right,body.top+Px(r->top+r->height-m_searches[m_page].scroll));CRect clipped;clipped.IntersectRect(bounds,body);if(clipped.PtInRect(point))return r.get();}
        }
        return nullptr;
    }
    void Editor::Menu(Row& row,CPoint point)
    {
        const std::wstring identity=row.setting->id;EnsureVisible(row);if(point==CPoint(-1,-1)){CRect rect;row.details.GetWindowRect(rect);point=CPoint(rect.left,rect.bottom);}
        CMenu menu;menu.CreatePopupMenu();menu.AppendMenu(MF_STRING,1,T(L"Show explanation",L"顯示解說"));const UINT flags=MF_STRING|(row.setting->Available()?0:MF_GRAYED);
        menu.AppendMenu(flags,2,T(L"Stage this setting's default",L"暫存呢項設定嘅預設值"));menu.AppendMenu(flags,3,T(L"Revert this setting to applied value",L"還原呢項設定至目前套用值"));
        const UINT selected=NativeMenus::Track(&menu,TPM_RETURNCMD|TPM_RIGHTBUTTON,point.x,point.y,this);auto* current=Find(identity);if(!current)return;
        if(selected==1){current->expanded=true;Layout();EnsureVisible(*current);Note(NoteKind::Explanation, current->setting);}
        else if(selected==2&&current->setting->Available())SetRowFrom(*current,Values{});else if(selected==3&&current->setting->Available())SetRowFrom(*current,Values::Current());
    }
    void Editor::OnContextMenu(CWnd* window,CPoint point){if(auto* row=Target(window?window->GetSafeHwnd():nullptr,point))Menu(*row,point);}
    BOOL Editor::OnCommand(WPARAM w,LPARAM l)
    {
        const UINT id=LOWORD(w),code=HIWORD(w);if(code==BN_CLICKED&&m_interface.Command(id)){Layout();return TRUE;}if(m_loading)return CDialog::OnCommand(w,l);
        if(id==Search&&code==EN_CHANGE){m_regexEditor.Close();m_binding.Invalidate();m_reveal.clear();CString text;m_search.GetWindowText(text);m_searches[m_page].filter.options.query=text.GetString();m_searches[m_page].scroll=0;Layout();return TRUE;}
        if(id==PageFirst&&code==LBN_SELCHANGE){const int group=m_navigation.GetCurSel();if(group!=LB_ERR)SwitchGroup(group);return TRUE;}
        if(id==Results&&code==LBN_DBLCLK){OpenSelected();return TRUE;}
        if(id>=FirstRow&&id<FirstRow+m_rows.size()*8)
        {
            auto& r=*m_rows[(id-FirstRow)/8];const auto& s=*r.setting;const UINT part=(id-FirstRow)%8;
            if(part==0&&s.enabled&&code==BN_CLICKED&&s.Available()){m_values.*s.enabled=r.check.GetCheck()==BST_CHECKED;m_dirty=true;r.check.Invalidate();Layout();return TRUE;}
            if(part==2&&code==EN_CHANGE){m_dirty=true;Layout();return TRUE;}
            if(part==2&&code==EN_SETFOCUS){EnsureVisible(r);return TRUE;}
            if(part==4&&code==BN_CLICKED){Menu(r,CPoint(-1,-1));return TRUE;}
            if(part==3&&code==BN_CLICKED&&s.trafficUnits&&s.Available())
            {
                const std::wstring identity=s.id;CMenu menu;menu.CreatePopupMenu();menu.AppendMenu(MF_STRING|(m_values.trafficUnit==0?MF_CHECKED:0),1,L"MB");menu.AppendMenu(MF_STRING|(m_values.trafficUnit==1?MF_CHECKED:0),2,L"GB");CRect rect;r.unit.GetWindowRect(rect);
                const UINT value=NativeMenus::Track(&menu,TPM_RETURNCMD|TPM_RIGHTBUTTON,rect.left,rect.bottom,this);if((value==1||value==2)&&Find(identity)){m_values.trafficUnit=value-1;r.unit.SetLabel(value==1?L"MB":L"GB");m_dirty=true;Layout();}return TRUE;
            }
        }
        if(code!=BN_CLICKED)return CDialog::OnCommand(w,l);if(id>=PageFirst&&id<PageFirst+6){SwitchGroup(id-PageFirst);return TRUE;}
        switch(id)
        {
        case Regex:
        {
            // Reopening focuses the existing editor and retains its original callbacks.
            const int group=m_page;const auto generation=m_binding.Open(m_regexEditor.IsOpen());
            m_regexEditor.Open(m_hWnd,m_search.GetSafeHwnd(),m_searches[group].filter.options,
                [this,group,generation]{return m_page==group&&m_binding.Accepts(generation)?Subjects():std::vector<std::wstring>{};},
                [this,group,generation](const NativeRegex::Options& options){if(m_page!=group||!m_binding.Accepts(generation))return;m_searches[group].filter.options=options;m_loading=true;m_search.SetWindowText(options.query.c_str());m_loading=false;m_reveal.clear();Layout();},m_chinese,m_appearance);return TRUE;
        }
        case Clear:m_regexEditor.Close();m_binding.Invalidate();m_searches[m_page].filter.options.regex=false;m_searches[m_page].filter.options.query.clear();m_loading=true;m_search.SetWindowText(L"");m_loading=false;m_reveal.clear();Layout();m_search.SetFocus();return TRUE;
        case Apply:ApplyDraft();return TRUE;case Revert:Reload();return TRUE;case Reset:ResetPage();return TRUE;case OpenResult:OpenSelected();return TRUE;
        case ReturnFilter:m_reveal.clear();Layout();m_search.SetFocus();return TRUE;
        case Legacy:if(CanDiscard()){m_regexEditor.Close();m_binding.Invalidate();EndDialog(IDRETRY);}return TRUE;
        case Theme:SwitchGroup(Interface);return TRUE;
        default:return CDialog::OnCommand(w,l);
        }
    }
    BOOL Editor::PreTranslateMessage(MSG* msg)
    {
        if(m_regexEditor.Translate(msg))return TRUE;
        if(msg->message==WM_KEYDOWN)
        {
            HIMC context=ImmGetContext(msg->hwnd);const bool composing=context&&ImmGetCompositionStringW(context,GCS_COMPSTR,nullptr,0)>0;if(context)ImmReleaseContext(msg->hwnd,context);if(composing)return CDialog::PreTranslateMessage(msg);
            if(msg->wParam==VK_APPS||(msg->wParam==VK_F10&&(GetKeyState(VK_SHIFT)&0x8000))){if(auto* field=DYNAMIC_DOWNCAST(NativeMaterial::TextField,CWnd::FromHandlePermanent(msg->hwnd))){field->SendMessage(WM_CONTEXTMENU,reinterpret_cast<WPARAM>(field->GetSafeHwnd()),-1);return TRUE;}if(auto* r=Target(::GetFocus(),CPoint(-1,-1))){Menu(*r,CPoint(-1,-1));return TRUE;}}
            if(msg->wParam=='L'&&(GetKeyState(VK_CONTROL)&0x8000)){m_search.SetFocus();return TRUE;}
            if(msg->wParam==VK_RETURN&&msg->hwnd==m_results.GetSafeHwnd()){OpenSelected();return TRUE;}
            if(msg->wParam==VK_ESCAPE&&msg->hwnd==m_search.GetSafeHwnd()){CString query;m_search.GetWindowText(query);if(!query.IsEmpty()){m_search.SetWindowText(L"");return TRUE;}}
        }
        return CDialog::PreTranslateMessage(msg);
    }
    void Editor::OnTimer(UINT_PTR id)
    {if(id!=0x4E92){CDialog::OnTimer(id);return;}bool changed=false;for(auto& state:m_searches)changed=state.filter.Poll()||changed;if(changed)Layout();UpdateInfo();const HWND focus=::GetFocus();if(focus!=m_lastFocus){m_lastFocus=focus;if(auto* r=Target(focus,CPoint(-1,-1)))EnsureVisible(*r);else if(m_page==Interface&&focus&&::IsChild(m_body.GetSafeHwnd(),focus)&&::GetDlgCtrlID(focus)!=53998){CRect bounds;::GetWindowRect(focus,&bounds);m_body.ScreenToClient(bounds);int delta=0;if(bounds.top<0)delta=MulDiv(bounds.top,96,m_dpi);else if(bounds.bottom>Px(m_viewHeight))delta=MulDiv(bounds.bottom-Px(m_viewHeight),96,m_dpi);if(delta){m_searches[m_page].scroll+=delta;Layout();}}}}
    void Editor::OnPaint(){CPaintDC dc(this);CRect r;GetClientRect(r);dc.FillRect(r,&m_background);}
    HBRUSH Editor::OnCtlColor(CDC* dc,CWnd* window,UINT){if(auto* field=DYNAMIC_DOWNCAST(NativeMaterial::TextField,window))return field->Color(dc);const auto c=NativeMaterial::Palette::Resolve(m_appearance);dc->SetTextColor(RGB(BYTE(c.onSurface.r*255),BYTE(c.onSurface.g*255),BYTE(c.onSurface.b*255)));dc->SetBkColor(RGB(BYTE(c.surface.r*255),BYTE(c.surface.g*255),BYTE(c.surface.b*255)));return static_cast<HBRUSH>(m_background.GetSafeHandle());}
    void Editor::OnGetMinMaxInfo(MINMAXINFO* info){CDialog::OnGetMinMaxInfo(info);RECT r{0,0,Px(760),Px(640)};AdjustWindowRectEx(&r,GetStyle(),FALSE,GetExStyle());info->ptMinTrackSize={r.right-r.left,r.bottom-r.top};}
    LRESULT Editor::OnDpi(WPARAM dpi,LPARAM value){m_dpi=HIWORD(dpi);const auto* r=reinterpret_cast<const RECT*>(value);SetWindowPos(nullptr,r->left,r->top,r->right-r->left,r->bottom-r->top,SWP_NOZORDER|SWP_NOACTIVATE);SetFonts();m_lastFocus=nullptr;Layout();return 0;}
}
void OpenInterface(CWnd* owner){if(owner&&::IsWindow(owner->GetSafeHwnd())){openInterface=true;owner->PostMessage(WM_COMMAND,ID_OPTIONS);}}
bool IsSafeToRestart() noexcept{return activeEditors==0;}
bool Show(CWnd* owner,int initialPage,const std::function<void(const Values&)>& apply)
{
    EditorLifetime lifetime;struct Template{DLGTEMPLATE dialog;WORD menu{},className{},title{};}layout{};
    layout.dialog.style=WS_POPUP|WS_CAPTION|WS_SYSMENU|WS_THICKFRAME|WS_CLIPCHILDREN|DS_MODALFRAME;layout.dialog.dwExtendedStyle=WS_EX_CONTROLPARENT;layout.dialog.cx=500;layout.dialog.cy=400;
    Editor editor(initialPage,apply);if(!editor.InitModalIndirect(&layout.dialog,owner))return true;const INT_PTR result=editor.DoModal();return result==IDRETRY||result==-1;
}
}
