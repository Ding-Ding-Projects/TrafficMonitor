#pragma once
#include "Text.h"
#include "Draft.h"
#include "../NativeMaterial/Controls.h"
#include "../NativeMaterial/Panel.h"
#include "../NativeRegex/Engine.h"
#include <array>
namespace NativePreferences
{
    class Editor final
    {
    public:
        ~Editor();
        enum : UINT { LanguageChoice=56200, AppearanceChoice, ApplyChoice, RevertChoice, ResetChoice, ReapplyChoice, InterfaceDetails, SoundChoice };
        bool Create(CWnd* parent,CWnd* actionsParent=nullptr);
        void Refresh();
        int Layout(int x,int y,int width,UINT dpi,bool visible);
        int LayoutActions(int x,int y,int width,UINT dpi,bool visible);
        void SetFont(CFont* font){m_font=font;}
        void SetFilter(bool language,bool appearance,bool sounds=false){m_showLanguage=language;m_showAppearance=appearance;m_showSounds=sounds;}
        bool Command(UINT id);
        Draft draft;
        void UseSaved();
        void Focus(bool appearance){(appearance?m_appearanceChoice:m_languageChoice).SetFocus();}
        void FocusSounds(){m_soundChoice.SetFocus();}
    private:
        std::array<HWND,24> m_observed{};
        CWnd* m_parent{};CWnd* m_actionsParent{};CFont* m_font{};CFont m_fallbackFont,m_sectionFont;UINT m_fontDpi{},m_sectionDpi{};
        bool m_showLanguage{true},m_showAppearance{true},m_showSounds{false},m_detailsExpanded{};
        NativeMaterial::Panel m_languagePanel,m_appearancePanel,m_previewPanel,m_soundPanel;
        CStatic m_heading,m_language,m_languageHelp,m_appearance,m_appearanceHelp,m_sound,m_soundHelp,m_scope,m_note;NativeMaterial::ReadOnlyText m_preview;
        NativeMaterial::Button m_languageChoice,m_appearanceChoice,m_soundChoice,m_apply,m_revert,m_reset,m_reapply,m_details;
        std::array<NativeRegex::Options,3> m_searches;
        TextId m_status{TextId::Draft};
        void Save(bool reapply);
    };
}
