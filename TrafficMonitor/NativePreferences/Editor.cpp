#include "../stdafx.h"
#include "Editor.h"
#include "../NativeMenus/SearchableMenu.h"
namespace NativePreferences
{
    bool Editor::Create(CWnd* parent,CWnd* actionsParent)
    {
        m_parent=parent;m_actionsParent=actionsParent?actionsParent:parent;const CRect box(0,0,100,40);const DWORD child=WS_CHILD|WS_VISIBLE;
        if(!m_languagePanel.Create(parent,56220)||!m_appearancePanel.Create(parent,56221)||!m_previewPanel.Create(parent,56222))return false;
        if(!m_languageHelp.Create(L"",child,box,&m_languagePanel)||!m_appearanceHelp.Create(L"",child,box,&m_appearancePanel))return false;
        if(!m_heading.Create(L"",child,box,parent)||!m_language.Create(L"",child,box,&m_languagePanel)||!m_appearance.Create(L"",child,box,&m_appearancePanel)||!m_scope.Create(L"",child,box,parent)||!m_preview.Create(box,&m_previewPanel,56210)||!m_note.Create(L"",child,box,m_actionsParent))return false;
        if(!m_languageChoice.Create(L"",box,&m_languagePanel,LanguageChoice)||!m_appearanceChoice.Create(L"",box,&m_appearancePanel,AppearanceChoice)||!m_apply.Create(L"",box,m_actionsParent,ApplyChoice)||!m_revert.Create(L"",box,m_actionsParent,RevertChoice)||!m_reset.Create(L"",box,m_actionsParent,ResetChoice)||!m_reapply.Create(L"",box,m_actionsParent,ReapplyChoice))return false;
        m_languageChoice.SetVariant(NativeMaterial::ButtonVariant::Selector);m_appearanceChoice.SetVariant(NativeMaterial::ButtonVariant::Selector);m_revert.SetVariant(NativeMaterial::ButtonVariant::Tonal);m_reset.SetVariant(NativeMaterial::ButtonVariant::Text);m_reapply.SetVariant(NativeMaterial::ButtonVariant::Outlined);m_preview.ModifyStyle(WS_VSCROLL,0,SWP_FRAMECHANGED);UseSaved();return true;
    }
    void Editor::UseSaved(){const auto current=Current();draft={current.values,current.base,false,false};m_status=current.protectedFile?TextId::Protected:current.persisted?TextId::Loaded:current.origin==Origin::Migrated?TextId::Migrated:TextId::Defaults;Refresh();}
    void Editor::Refresh()
    {
        if(!m_parent)return;
        if(!draft.dirty&&!Matches(draft.base,Current().base)){const auto current=Current();draft={current.values,current.base,false,false};m_status=current.protectedFile?TextId::Protected:current.persisted?TextId::Loaded:current.origin==Origin::Migrated?TextId::Migrated:TextId::Defaults;}
        m_languagePanel.SetAppearance(EffectiveAppearance());m_appearancePanel.SetAppearance(EffectiveAppearance());m_previewPanel.SetAppearance(EffectiveAppearance());
        m_languageHelp.SetWindowText(Pair(L"Choose the language used throughout the native workspace.",L"選擇原生工作區使用嘅語言。"));m_appearanceHelp.SetWindowText(Pair(L"Follow your system, or keep a light or dark interface.",L"跟隨系統，或者固定使用淺色或深色介面。"));
        m_heading.SetWindowText(Text(TextId::Interface));m_language.SetWindowText(Text(TextId::LanguageLabel));m_appearance.SetWindowText(Text(TextId::AppearanceLabel));
        m_languageChoice.SetLabel(Text(static_cast<TextId>(static_cast<int>(TextId::English)+static_cast<int>(draft.values.language))));
        m_appearanceChoice.SetLabel(Text(static_cast<TextId>(static_cast<int>(TextId::System)+static_cast<int>(draft.values.appearance))));
        m_scope.SetWindowText(Text(TextId::Scope));m_preview.SetWindowText(Text(TextId::Preview,draft.values.language));m_preview.SetAppearance(draft.values.appearance==Appearance::System?EffectiveAppearance():draft.values.appearance==Appearance::Dark?NativeMaterial::Appearance::Dark:NativeMaterial::Appearance::Light);
        m_apply.SetLabel(m_actionsParent==m_parent?Text(TextId::Apply):Pair(L"Apply",L"套用"));m_revert.SetLabel(Pair(L"Use saved",L"用已儲存值"));m_reset.SetLabel(Pair(L"Reset",L"重設"));m_reapply.SetLabel(Text(TextId::Reapply));
        if(draft.dirty&&!Matches(draft.base,Current().base)){draft.conflict=true;m_status=TextId::Conflict;}
        m_note.SetWindowText(Text(m_status));
        for(auto* b:{&m_languageChoice,&m_appearanceChoice,&m_apply,&m_revert,&m_reset,&m_reapply})b->SetAppearance(EffectiveAppearance());
        m_apply.EnableWindow(draft.dirty&&!Current().protectedFile);m_reapply.EnableWindow(draft.conflict&&!Current().protectedFile);
    }
    int Editor::Layout(int x,int y,int width,UINT dpi,bool visible)
    {
        std::array<CWnd*,12> controls{&m_heading,&m_language,&m_languageHelp,&m_languageChoice,&m_appearance,&m_appearanceHelp,&m_appearanceChoice,&m_scope,&m_preview,&m_languagePanel,&m_appearancePanel,&m_previewPanel};
        for(auto* c:controls)c->ShowWindow(visible?SW_SHOW:SW_HIDE);
        if(!visible){LayoutActions(0,0,width,dpi,false);return 0;}
        if(!m_font&&m_fontDpi!=dpi){m_fallbackFont.DeleteObject();m_fallbackFont.CreateFont(-MulDiv(14,dpi,96),0,0,0,FW_NORMAL,FALSE,FALSE,0,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,L"Segoe UI");m_fontDpi=dpi;}
        CFont* font=m_font?m_font:&m_fallbackFont;for(auto* c:controls)c->SetFont(font);
        const int initial=y;auto px=[&](int n){return MulDiv(n,dpi,96);};CClientDC dc(m_parent);auto* old=dc.SelectObject(font);
        auto height=[&](CWnd& c,int available,int minimum){CString t;c.GetWindowText(t);CRect r(0,0,px((std::max)(32,available)),0);dc.DrawText(t,r,DT_CALCRECT|DT_WORDBREAK|DT_NOPREFIX);return (std::max)(minimum,MulDiv(r.Height(),96,dpi)+8);};
        auto place=[&](CWnd& c,int left,int top,int w,int h){c.MoveWindow(px(left),px(top),px(w),px(h));};
        if(m_sectionDpi!=dpi){m_sectionFont.DeleteObject();m_sectionFont.CreateFont(-px(18),0,0,0,FW_SEMIBOLD,FALSE,FALSE,0,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,L"Segoe UI");m_sectionDpi=dpi;}m_heading.SetFont(&m_sectionFont);dc.SelectObject(&m_sectionFont);const int heading=height(m_heading,width,32);dc.SelectObject(font);place(m_heading,x,y,width,heading);y+=heading+20;
        auto row=[&](NativeMaterial::Panel& panel,CStatic& label,CStatic& help,NativeMaterial::Button& choice,bool show){panel.ShowWindow(show?SW_SHOW:SW_HIDE);if(!show)return;const bool wide=width>=600;const int inner=width-40,lw=wide?inner*46/100:inner,cw=wide?inner-lw-24:inner;const int lh=height(label,lw,24),hh=height(help,lw,24),ch=height(choice,cw-32,36)+8;int ph=0;place(label,20,16,lw,lh);place(help,20,16+lh+4,lw,hh);if(wide){ph=(std::max)(lh+hh+4,ch)+32;place(choice,20+lw+24,(ph-ch)/2,cw,ch);}else{const int cy=16+lh+4+hh+12;place(choice,20,cy,cw,ch);ph=cy+ch+16;}place(panel,x,y,width,ph);y+=ph+12;};
        row(m_languagePanel,m_language,m_languageHelp,m_languageChoice,m_showLanguage);row(m_appearancePanel,m_appearance,m_appearanceHelp,m_appearanceChoice,m_showAppearance);
        const bool any=m_showLanguage||m_showAppearance;m_scope.ShowWindow(any?SW_SHOW:SW_HIDE);m_previewPanel.ShowWindow(any?SW_SHOW:SW_HIDE);
        if(any){const int sh=height(m_scope,width,32);place(m_scope,x,y,width,sh);y+=sh+12;const int ph=height(m_preview,width-40,48);place(m_previewPanel,x,y,width,ph+32);place(m_preview,20,16,width-40,ph);y+=ph+48;}
        dc.SelectObject(old);if(m_actionsParent==m_parent)y+=LayoutActions(x,y,width,dpi,true);return y-initial;
    }
    int Editor::LayoutActions(int x,int y,int width,UINT dpi,bool visible)
    {
        for(auto* c:std::array<CWnd*,5>{&m_note,&m_apply,&m_revert,&m_reset,&m_reapply})c->ShowWindow(visible?SW_SHOW:SW_HIDE);if(!visible)return 0;
        CFont* font=m_font?m_font:&m_fallbackFont;CClientDC dc(m_actionsParent);auto* old=dc.SelectObject(font);auto px=[&](int n){return MulDiv(n,dpi,96);};const int initial=y;
        auto measure=[&](CWnd& c,int available){CString t;c.GetWindowText(t);CRect r(0,0,px(available),0);dc.DrawText(t,r,DT_CALCRECT|DT_WORDBREAK|DT_NOPREFIX);return MulDiv(r.Height(),96,dpi);};
        m_note.SetFont(font);const int noteHeight=(std::max)(24,measure(m_note,width)+8);m_note.MoveWindow(px(x),px(y),px(width),px(noteHeight));y+=noteHeight+8;
        int left=x,rowHeight=0;for(auto* b:{&m_apply,&m_revert,&m_reset,&m_reapply}){const bool show=b!=&m_reapply||draft.conflict;b->ShowWindow(show?SW_SHOW:SW_HIDE);if(!show)continue;b->SetFont(font);CString label;b->GetWindowText(label);CRect r(0,0,px((std::min)(220,width)-32),0);dc.DrawText(label,r,DT_CALCRECT|DT_WORDBREAK|DT_NOPREFIX);const int bw=(std::min)(width,(std::max)(96,MulDiv(r.Width(),96,dpi)+32));const int bh=(std::max)(40,MulDiv(r.Height(),96,dpi)+20);if(left>x&&left+bw>x+width){y+=rowHeight+8;left=x;rowHeight=0;}b->MoveWindow(px(left),px(y),px(bw),px(bh));left+=bw+8;rowHeight=(std::max)(rowHeight,bh);}dc.SelectObject(old);return y+rowHeight+12-initial;
    }
    void Editor::Save(bool reapply)
    {
        if(reapply){if(!Reload()){m_status=TextId::Protected;Refresh();return;}draft.base=Current().base;}
        const auto result=TryApply(draft.values,draft.base);
        if(result==Result::Saved){UseSaved();m_status=TextId::Saved;Refresh();return;}
        if(result==Result::Conflict){draft.conflict=true;m_status=TextId::Conflict;}
        else m_status=result==Result::Protected?TextId::Protected:result==Result::Uncertain?TextId::Uncertain:TextId::WriteFailed;
        Refresh();
    }
    bool Editor::Command(UINT id)
    {
        if(id<LanguageChoice||id>ReapplyChoice)return false;
        if(id==RevertChoice){Reload();UseSaved();return true;}
        if(id==ResetChoice){draft.values=Defaults();draft.dirty=draft.values!=draft.base.values;m_status=TextId::Draft;Refresh();return true;}
        if(id==ApplyChoice||id==ReapplyChoice){Save(id==ReapplyChoice);return true;}
        const bool language=id==LanguageChoice;auto& button=language?m_languageChoice:m_appearanceChoice;CMenu menu;menu.CreatePopupMenu();
        const int selected=language?static_cast<int>(draft.values.language):static_cast<int>(draft.values.appearance);
        for(int i=0;i<3;++i){const auto label=Text(static_cast<TextId>(static_cast<int>(language?TextId::English:TextId::System)+i));menu.AppendMenu(MF_STRING|(i==selected?MF_CHECKED:0),56300+i,label);MENUITEMINFOW info{sizeof(info)};info.fMask=MIIM_FTYPE;info.fType=MFT_RADIOCHECK;SetMenuItemInfoW(menu.GetSafeHmenu(),56300+i,FALSE,&info);}
        CRect bounds;button.GetWindowRect(bounds);
        const auto choice=NativeMenus::Track(&menu,TPM_RETURNCMD|TPM_RIGHTBUTTON,bounds.left,bounds.bottom,m_parent,&m_searches[language?0:1]);
        if(choice>=56300&&choice<56303){if(language)draft.values.language=static_cast<Language>(choice-56300);else draft.values.appearance=static_cast<Appearance>(choice-56300);draft.dirty=draft.values!=draft.base.values;m_status=TextId::Draft;Refresh();}
        if(button.GetSafeHwnd())button.SetFocus();return true;
    }
}
