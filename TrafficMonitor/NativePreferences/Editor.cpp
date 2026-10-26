#include "../stdafx.h"
#include "Editor.h"
#include "../NativeMaterial/ActionSounds.h"
#include "../NativeMaterial/LayoutObservations.h"
namespace NativePreferences
{
    namespace
    {
        // Segment captions stay on one line; the full wording of each level is in the section help text.
        CString SoundLabel(ActionSounds level)
        {
            return level==ActionSounds::Off?InlinePair(L"Off",L"關閉"):level==ActionSounds::Important?InlinePair(L"Important",L"重要"):InlinePair(L"All",L"全部");
        }
        // Language names are written in their own language, whatever the interface language is.
        CString LanguageLabel(Language language)
        {
            return Text(static_cast<TextId>(static_cast<int>(TextId::English)+static_cast<int>(language)),Language::Cantonese);
        }
        std::vector<CString> Segments(CString (*label)(int)){return {label(0),label(1),label(2)};}
        CString AppearanceLabel(int index){return Text(static_cast<TextId>(static_cast<int>(TextId::System)+index));}
        CString LanguageSegment(int index){return LanguageLabel(static_cast<Language>(index));}
        CString SoundSegment(int index){return SoundLabel(static_cast<ActionSounds>(index));}
    }
    Editor::~Editor(){for(HWND window:m_observed)NativeLayoutObservation::Unregister(window);}
    bool Editor::Create(CWnd* parent,CWnd* actionsParent)
    {
        m_parent=parent;m_actionsParent=actionsParent?actionsParent:parent;const CRect box(0,0,100,40);const DWORD child=WS_CHILD|WS_VISIBLE;
        if(!m_languagePanel.Create(parent,56220)||!m_appearancePanel.Create(parent,56221)||!m_previewPanel.Create(parent,56222)||!m_soundPanel.Create(parent,56223))return false;
        if(!m_languageHelp.Create(L"",child,box,&m_languagePanel)||!m_appearanceHelp.Create(L"",child,box,&m_appearancePanel)||!m_soundHelp.Create(L"",child,box,&m_soundPanel)||!m_sound.Create(L"",child,box,&m_soundPanel))return false;
        if(!m_heading.Create(L"",child,box,parent)||!m_language.Create(L"",child,box,&m_languagePanel)||!m_appearance.Create(L"",child,box,&m_appearancePanel)||!m_scope.Create(L"",child,box,parent)||!m_preview.Create(L"",child,box,&m_previewPanel,56210)||!m_note.Create(L"",child,box,m_actionsParent))return false;
        if(!m_languageChoice.Create(box,&m_languagePanel,LanguageChoice)||!m_appearanceChoice.Create(box,&m_appearancePanel,AppearanceChoice)||!m_soundChoice.Create(box,&m_soundPanel,SoundChoice)||!m_apply.Create(L"",box,m_actionsParent,ApplyChoice)||!m_revert.Create(L"",box,m_actionsParent,RevertChoice)||!m_reset.Create(L"",box,m_actionsParent,ResetChoice)||!m_reapply.Create(L"",box,m_actionsParent,ReapplyChoice))return false;
        if(!m_details.Create(L"",box,parent,InterfaceDetails))return false;
        // Keep the disclosure before the nested form controls in native tab order.
        m_heading.SetWindowPos(&CWnd::wndTop,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);
        m_details.SetWindowPos(&m_heading,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);
        m_languagePanel.SetFlat(true);m_appearancePanel.SetFlat(true);m_soundPanel.SetFlat(true);
        m_details.SetVariant(NativeMaterial::ButtonVariant::Text);
        m_revert.SetVariant(NativeMaterial::ButtonVariant::Text);m_reset.SetVariant(NativeMaterial::ButtonVariant::Text);m_reapply.SetVariant(NativeMaterial::ButtonVariant::Outlined);
        if(NativeLayoutObservation::Enabled())
        {
            const std::array<CWnd*,24> observed{m_parent,m_actionsParent,&m_heading,&m_language,&m_languageHelp,&m_languageChoice,&m_appearance,&m_appearanceHelp,&m_appearanceChoice,&m_scope,&m_preview,&m_languagePanel,&m_appearancePanel,&m_previewPanel,&m_details,&m_note,&m_apply,&m_revert,&m_reset,&m_reapply,&m_sound,&m_soundHelp,&m_soundChoice,&m_soundPanel};
            for(size_t i=0;i<observed.size();++i){m_observed[i]=observed[i]->GetSafeHwnd();NativeLayoutObservation::Register(m_observed[i]);}
        }
        UseSaved();return true;
    }
    void Editor::UseSaved(){const auto current=Current();draft={current.values,current.base,false,false};m_status=current.protectedFile?TextId::Protected:current.persisted?TextId::Loaded:current.origin==Origin::Migrated?TextId::Migrated:TextId::Defaults;Refresh();}
    void Editor::Refresh()
    {
        if(!m_parent)return;
        for(HWND window:m_observed)NativeLayoutObservation::Invalidate(window);
        if(!draft.dirty&&!Matches(draft.base,Current().base)){const auto current=Current();draft={current.values,current.base,false,false};m_status=current.protectedFile?TextId::Protected:current.persisted?TextId::Loaded:current.origin==Origin::Migrated?TextId::Migrated:TextId::Defaults;}
        m_languagePanel.SetAppearance(EffectiveAppearance());m_appearancePanel.SetAppearance(EffectiveAppearance());m_soundPanel.SetAppearance(EffectiveAppearance());
        m_languageHelp.SetWindowText(Pair(L"Choose the language used throughout the native workspace.",L"選擇原生工作區使用嘅語言。"));m_appearanceHelp.SetWindowText(Pair(L"Follow your system, or keep a light or dark interface.",L"跟隨系統，或者固定使用淺色或深色介面。"));
        m_soundHelp.SetWindowText(Pair(L"Short Windows system sounds for meaningful actions. Windows volume and mute settings are respected and never changed.",L"為重要操作播放簡短嘅 Windows 系統音效。會跟從 Windows 嘅音量同靜音設定，亦不會更改。"));
        m_sound.SetWindowText(Pair(L"Action sounds",L"操作音效"));
        m_soundChoice.SetSegments(Segments(SoundSegment));m_soundChoice.SetSelected(static_cast<int>(draft.values.actionSounds));m_soundChoice.SetAccessibleName(Pair(L"Action sounds",L"操作音效"));
        m_heading.SetWindowText(Text(TextId::Interface));m_language.SetWindowText(Text(TextId::LanguageLabel));m_appearance.SetWindowText(Text(TextId::AppearanceLabel));
        m_languageChoice.SetSegments(Segments(LanguageSegment));m_languageChoice.SetSelected(static_cast<int>(draft.values.language));m_languageChoice.SetAccessibleName(Text(TextId::LanguageLabel));
        m_appearanceChoice.SetSegments(Segments(AppearanceLabel));m_appearanceChoice.SetSelected(static_cast<int>(draft.values.appearance));m_appearanceChoice.SetAccessibleName(Text(TextId::AppearanceLabel));
        const auto snapshot=Current();
        CString provenance;
        if(snapshot.protectedFile)provenance=Pair(L"Preference source is protected or invalid.",L"偏好來源受保護或無效。");
        else if(snapshot.origin==Origin::Uncertain)provenance=Pair(L"Saved preference verification is uncertain.",L"未能確定已儲存偏好嘅驗證結果。");
        else if(snapshot.persisted)provenance=Pair(L"Source: saved interface preferences.",L"來源：已儲存嘅介面偏好。");
        else if(snapshot.origin==Origin::Migrated)provenance=Pair(L"Source: migrated setup choices.",L"來源：已轉移嘅精靈選項。");
        else provenance=Pair(L"Source: startup defaults.",L"來源：啟動預設值。");
        CString details=Text(TextId::Scope)+L"\r\n\r\n"+provenance;
        if(m_showSounds){CString soundHelp;m_soundHelp.GetWindowText(soundHelp);details+=L"\r\n\r\n"+soundHelp;}
        m_scope.SetWindowText(details);
        m_details.SetLabel(Pair(L"Details",L"詳細資料"));
        m_details.SetDisclosure(m_detailsExpanded,Pair(L"Interface details",L"介面詳細資料"),details);
        m_preview.SetWindowText(Pair(L"Preview: interface text",L"預覽：介面文字",draft.values.language));// The preview surface itself takes the drafted appearance, so its text shows how the choice will look.
        m_previewPanel.SetAppearance(draft.values.appearance==Appearance::System?EffectiveAppearance():draft.values.appearance==Appearance::Dark?NativeMaterial::Appearance::Dark:NativeMaterial::Appearance::Light);
        m_apply.SetLabel(m_actionsParent==m_parent?Text(TextId::Apply):Pair(L"Apply",L"套用"));m_revert.SetLabel(Pair(L"Use saved",L"用已儲存值"));m_reset.SetLabel(Pair(L"Reset",L"重設"));m_reapply.SetLabel(Text(TextId::Reapply));
        if(draft.dirty&&!Matches(draft.base,Current().base)){draft.conflict=true;m_status=TextId::Conflict;}
        m_note.SetWindowText(m_status==TextId::Defaults?Pair(L"Using defaults",L"使用預設值"):m_status==TextId::Migrated?Pair(L"Using migrated choices",L"使用已轉移選項"):Text(m_status));
        for(auto* b:{&m_languageChoice,&m_appearanceChoice,&m_soundChoice})b->SetAppearance(EffectiveAppearance());for(auto* b:{&m_apply,&m_revert,&m_reset,&m_reapply,&m_details})b->SetAppearance(EffectiveAppearance());
        m_apply.EnableWindow(draft.dirty&&!Current().protectedFile);m_reapply.EnableWindow(draft.conflict&&!Current().protectedFile);
    }
    int Editor::Layout(int x,int y,int width,UINT dpi,bool visible)
    {
        std::array<CWnd*,17> controls{&m_heading,&m_language,&m_languageHelp,&m_languageChoice,&m_appearance,&m_appearanceHelp,&m_appearanceChoice,&m_scope,&m_preview,&m_languagePanel,&m_appearancePanel,&m_previewPanel,&m_details,&m_sound,&m_soundHelp,&m_soundChoice,&m_soundPanel};
        for(auto* c:controls)c->ShowWindow(visible?SW_SHOW:SW_HIDE);
        if(!visible){LayoutActions(0,0,width,dpi,false);return 0;}
        if(!m_font&&m_fontDpi!=dpi){m_fallbackFont.DeleteObject();m_fallbackFont.CreateFont(-MulDiv(14,dpi,96),0,0,0,FW_NORMAL,FALSE,FALSE,0,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,L"Segoe UI");m_fontDpi=dpi;}
        CFont* font=m_font?m_font:&m_fallbackFont;for(auto* c:controls)c->SetFont(font);
        const int initial=y;auto px=[&](int n){return MulDiv(n,dpi,96);};CClientDC dc(m_parent);auto* old=dc.SelectObject(font);NativeLayoutObservation::MeasurementBatch measurements;
        auto height=[&](CWnd& c,int available,int minimum){CString t;c.GetWindowText(t);CRect r(0,0,px((std::max)(32,available)),0);const RECT area=r;dc.DrawText(t,r,DT_CALCRECT|DT_WORDBREAK|DT_NOPREFIX);measurements.Measure(c.GetSafeHwnd(),dc.GetSafeHdc(),area,r);return (std::max)(minimum,(r.Height()*96+static_cast<int>(dpi)-1)/static_cast<int>(dpi)+4);};
        auto place=[&](CWnd& c,int left,int top,int w,int h){c.MoveWindow(px(left),px(top),px(w),px(h));};
        width=(std::min)(1120,(std::max)(64,width));
        CString detailLabel;m_details.GetWindowText(detailLabel);CRect detailBounds(0,0,px(width-24),0);
        dc.DrawText(detailLabel,detailBounds,DT_CALCRECT|DT_WORDBREAK|DT_NOPREFIX);
        const int detailWidth=(std::min)(width,(std::max)(88,(detailBounds.Width()*96+static_cast<int>(dpi)-1)/static_cast<int>(dpi)+24));
        const int detailHeight=height(m_details,detailWidth-24,40)+8;
        if(m_sectionDpi!=dpi){m_sectionFont.DeleteObject();m_sectionFont.CreateFont(-px(22),0,0,0,FW_NORMAL,FALSE,FALSE,0,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,L"Segoe UI");m_sectionDpi=dpi;}
        m_heading.SetFont(&m_sectionFont);dc.SelectObject(&m_sectionFont);
        CString headingText;m_heading.GetWindowText(headingText);CRect natural(0,0,px(2000),0);dc.DrawText(headingText,natural,DT_CALCRECT|DT_NOPREFIX);
        const bool adjacent=natural.Width()+px(detailWidth+16)<=px(width);
        const int headingWidth=adjacent?width-detailWidth-16:width;
        const int headingHeight=height(m_heading,headingWidth,32);dc.SelectObject(font);
        place(m_heading,x,y,headingWidth,headingHeight);
        if(adjacent){place(m_details,x+width-detailWidth,y,detailWidth,detailHeight);y+=(std::max)(headingHeight,detailHeight)+12;}
        else{y+=headingHeight+4;place(m_details,x,y,detailWidth,detailHeight);y+=detailHeight+12;}
        m_scope.ShowWindow(m_detailsExpanded?SW_SHOW:SW_HIDE);
        if(m_detailsExpanded){const int sh=height(m_scope,width,20);place(m_scope,x,y,width,sh);y+=sh+12;}
        m_languageHelp.ShowWindow(SW_HIDE);m_appearanceHelp.ShowWindow(SW_HIDE);m_soundHelp.ShowWindow(SW_HIDE);
        const bool bilingual=Current().values.language==Language::Bilingual;
        auto row=[&](NativeMaterial::Panel& panel,CStatic& label,CStatic&,CWnd& choice,bool show)
        {
            panel.ShowWindow(show?SW_SHOW:SW_HIDE);if(!show)return;
            // Label above, segmented choice below at the available width up to a comfortable line length.
            const int lh=height(label,width,bilingual?40:24),ch=48,cw=(std::min)(width,520);
            const int ph=lh+8+ch+16;place(label,0,8,width,lh);place(choice,0,8+lh+8,cw,ch);
            place(panel,x,y,width,ph);y+=ph+8;
        };
        row(m_languagePanel,m_language,m_languageHelp,m_languageChoice,m_showLanguage);row(m_appearancePanel,m_appearance,m_appearanceHelp,m_appearanceChoice,m_showAppearance);row(m_soundPanel,m_sound,m_soundHelp,m_soundChoice,m_showSounds);
        const bool any=m_showLanguage||m_showAppearance;m_previewPanel.ShowWindow(any?SW_SHOW:SW_HIDE);
        if(any){const int textHeight=height(m_preview,width-32,20),ph=(std::max)(draft.values.language==Language::Bilingual?72:48,textHeight+32);place(m_previewPanel,x,y,width,ph);place(m_preview,16,16,width-32,ph-32);y+=ph+12;}
        for(auto* control:std::array<CWnd*,9>{&m_heading,&m_language,&m_languageHelp,&m_appearance,&m_appearanceHelp,&m_scope,&m_preview,&m_note,&m_sound})NativeLayoutObservation::ObserveNativeText(control->GetSafeHwnd());
        measurements.Publish();dc.SelectObject(old);if(m_actionsParent==m_parent)y+=LayoutActions(x,y,width,dpi,true);return y-initial;
    }
    int Editor::LayoutActions(int x,int y,int width,UINT dpi,bool visible)
    {
        for(auto* c:std::array<CWnd*,5>{&m_note,&m_apply,&m_revert,&m_reset,&m_reapply})c->ShowWindow(visible?SW_SHOW:SW_HIDE);if(!visible)return 0;
        CFont* font=m_font?m_font:&m_fallbackFont;CClientDC dc(m_actionsParent);auto* old=dc.SelectObject(font);auto px=[&](int n){return MulDiv(n,dpi,96);};const int initial=y;NativeLayoutObservation::MeasurementBatch measurements;
        auto measure=[&](CWnd& c,int available){CString t;c.GetWindowText(t);CRect r(0,0,px(available),0);const RECT area=r;dc.DrawText(t,r,DT_CALCRECT|DT_WORDBREAK|DT_NOPREFIX);measurements.Measure(c.GetSafeHwnd(),dc.GetSafeHdc(),area,r);return MulDiv(r.Height(),96,dpi);};
        m_note.SetFont(font);const int noteHeight=(std::max)(24,measure(m_note,width)+8);m_note.MoveWindow(px(x),px(y),px(width),px(noteHeight));NativeLayoutObservation::ObserveNativeText(m_note.GetSafeHwnd());y+=noteHeight+8;
        int left=x,rowHeight=0;for(auto* b:{&m_apply,&m_revert,&m_reset,&m_reapply}){const bool show=b!=&m_reapply||draft.conflict;b->ShowWindow(show?SW_SHOW:SW_HIDE);if(!show)continue;b->SetFont(font);CString label;b->GetWindowText(label);/* The pill draws its label in Medium weight with a 24 dp inset, so size it to the label with slack and only wrap when the whole row is too narrow. */CRect r(0,0,px((std::max)(96,width-64)),0);dc.DrawText(label,r,DT_CALCRECT|DT_WORDBREAK|DT_NOPREFIX);const int bw=(std::min)(width,(std::max)(96,MulDiv(r.Width(),96,dpi)*108/100+64));const int bh=(std::max)(40,MulDiv(r.Height(),96,dpi)+20);if(left>x&&left+bw>x+width){y+=rowHeight+8;left=x;rowHeight=0;}b->MoveWindow(px(left),px(y),px(bw),px(bh));left+=bw+8;rowHeight=(std::max)(rowHeight,bh);}measurements.Publish();dc.SelectObject(old);return y+rowHeight+12-initial;
    }
    void Editor::Save(bool reapply)
    {
        if(reapply){if(!Reload()){m_status=TextId::Protected;Refresh();return;}draft.base=Current().base;}
        const auto result=TryApply(draft.values,draft.base);
        if(result==Result::Saved){UseSaved();m_status=TextId::Saved;Refresh();NativeSounds::Notify(NativeSounds::Action::SettingsApplied);return;}
        if(result==Result::Conflict){draft.conflict=true;m_status=TextId::Conflict;}
        else m_status=result==Result::Protected?TextId::Protected:result==Result::Uncertain?TextId::Uncertain:TextId::WriteFailed;
        Refresh();NativeSounds::Notify(NativeSounds::Action::ActionRefused);
    }
    bool Editor::Command(UINT id,UINT code)
    {
        if(id<LanguageChoice||id>SoundChoice)return false;
        if(id==InterfaceDetails){m_detailsExpanded=!m_detailsExpanded;CString details;m_scope.GetWindowText(details);m_details.SetDisclosure(m_detailsExpanded,Pair(L"Interface details",L"介面詳細資料"),details);m_details.SetFocus();return true;}
        if(id==RevertChoice){Reload();UseSaved();return true;}
        if(id==ResetChoice){const auto keptSounds=draft.values.actionSounds;draft.values=Defaults();if(!m_showSounds)draft.values.actionSounds=keptSounds;draft.dirty=draft.values!=draft.base.values;m_status=TextId::Draft;Refresh();return true;}
        if(id==ApplyChoice||id==ReapplyChoice){Save(id==ReapplyChoice);return true;}
        // The three choices are segmented buttons; only a changed selection matters.
        if(code!=CBN_SELCHANGE)return true;
        const bool language=id==LanguageChoice,sounds=id==SoundChoice;auto& choice=language?m_languageChoice:sounds?m_soundChoice:m_appearanceChoice;
        const int index=choice.GetSelected();if(index<0||index>2)return true;
        if(language)draft.values.language=static_cast<Language>(index);else if(sounds)draft.values.actionSounds=static_cast<ActionSounds>(index);else draft.values.appearance=static_cast<Appearance>(index);
        draft.dirty=draft.values!=draft.base.values;m_status=TextId::Draft;Refresh();return true;
    }
}
