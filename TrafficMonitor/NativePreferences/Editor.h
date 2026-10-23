#pragma once
#include "Text.h"
#include "Draft.h"
#include "../NativeMaterial/Controls.h"
#include "../NativeRegex/Engine.h"
#include <array>
namespace NativePreferences
{
    class Editor final
    {
    public:
        enum : UINT { LanguageChoice=56200, AppearanceChoice, ApplyChoice, RevertChoice, ResetChoice, ReapplyChoice };
        bool Create(CWnd* parent);
        void Refresh();
        int Layout(int x,int y,int width,UINT dpi,bool visible);
        bool Command(UINT id);
        Draft draft;
        void UseSaved();
        void Focus(bool appearance){(appearance?m_appearanceChoice:m_languageChoice).SetFocus();}
    private:
        CWnd* m_parent{};
        CStatic m_language,m_appearance,m_scope,m_preview,m_note;
        NativeMaterial::Button m_languageChoice,m_appearanceChoice,m_apply,m_revert,m_reset,m_reapply;
        std::array<NativeRegex::Options,2> m_searches;
        TextId m_status{TextId::Draft};
        void Save(bool reapply);
    };
}
