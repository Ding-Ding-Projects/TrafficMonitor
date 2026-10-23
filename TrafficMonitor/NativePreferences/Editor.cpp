#include "../stdafx.h"
#include "Editor.h"
#include "../NativeMenus/SearchableMenu.h"
namespace NativePreferences
{
    bool Editor::Create(CWnd* parent)
    {
        m_parent=parent;const CRect box(0,0,100,40);const DWORD child=WS_CHILD|WS_VISIBLE;
        if(!m_language.Create(L"",child,box,parent)||!m_appearance.Create(L"",child,box,parent)||!m_scope.Create(L"",child,box,parent)||!m_preview.Create(L"",child,box,parent)||!m_note.Create(L"",child,box,parent))return false;
        if(!m_languageChoice.Create(L"",box,parent,LanguageChoice)||!m_appearanceChoice.Create(L"",box,parent,AppearanceChoice)||!m_apply.Create(L"",box,parent,ApplyChoice)||!m_revert.Create(L"",box,parent,RevertChoice)||!m_reset.Create(L"",box,parent,ResetChoice)||!m_reapply.Create(L"",box,parent,ReapplyChoice))return false;
        UseSaved();return true;
    }
    void Editor::UseSaved(){const auto current=Current();draft={current.values,current.base,false,false};m_status=current.protectedFile?TextId::Protected:current.persisted?TextId::Loaded:TextId::Defaults;Refresh();}
    void Editor::Refresh()
    {
        if(!m_parent)return;
        if(!draft.dirty&&!Matches(draft.base,Current().base)){const auto current=Current();draft={current.values,current.base,false,false};m_status=current.protectedFile?TextId::Protected:current.persisted?TextId::Loaded:TextId::Defaults;}
        m_language.SetWindowText(Text(TextId::LanguageLabel));m_appearance.SetWindowText(Text(TextId::AppearanceLabel));
        m_languageChoice.SetLabel(Text(static_cast<TextId>(static_cast<int>(TextId::English)+static_cast<int>(draft.values.language))));
        m_appearanceChoice.SetLabel(Text(static_cast<TextId>(static_cast<int>(TextId::System)+static_cast<int>(draft.values.appearance))));
        m_scope.SetWindowText(Text(TextId::Scope));m_preview.SetWindowText(Text(TextId::Preview,draft.values.language));
        m_apply.SetLabel(Text(TextId::Apply));m_revert.SetLabel(Text(TextId::UseSaved));m_reset.SetLabel(Text(TextId::Reset));m_reapply.SetLabel(Text(TextId::Reapply));
        if(draft.dirty&&!Matches(draft.base,Current().base)){draft.conflict=true;m_status=TextId::Conflict;}
        m_note.SetWindowText(Text(m_status));
        for(auto* b:{&m_languageChoice,&m_appearanceChoice,&m_apply,&m_revert,&m_reset,&m_reapply})b->SetAppearance(EffectiveAppearance());
        m_apply.EnableWindow(draft.dirty&&!Current().protectedFile);m_reapply.EnableWindow(draft.conflict&&!Current().protectedFile);
    }
    int Editor::Layout(int x,int y,int width,UINT dpi,bool visible)
    {
        std::array<CWnd*,11> controls{&m_language,&m_languageChoice,&m_appearance,&m_appearanceChoice,&m_scope,&m_preview,&m_note,&m_apply,&m_revert,&m_reset,&m_reapply};
        for(auto* c:controls)c->ShowWindow(visible?SW_SHOW:SW_HIDE);if(!visible)return 0;
        const int initial=y;auto px=[&](int n){return MulDiv(n,dpi,96);};CClientDC dc(m_parent);CFont* font=m_parent->GetFont();auto* old=font?dc.SelectObject(font):nullptr;
        for(auto* c:controls)
        {
            CString text;c->GetWindowText(text);CRect measured(0,0,px((std::max)(32,width-32)),0);dc.DrawText(text,measured,DT_CALCRECT|DT_WORDBREAK|DT_NOPREFIX);
            const bool action=c==&m_languageChoice||c==&m_appearanceChoice||c==&m_apply||c==&m_revert||c==&m_reset||c==&m_reapply;
            const int height=(std::max)(action?44:24,MulDiv(measured.Height(),96,dpi)+(action?20:4));
            c->MoveWindow(px(x),px(y),px(width),px(height));if(font)c->SetFont(font);y+=height+8;
        }
        if(old)dc.SelectObject(old);return y-initial;
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
