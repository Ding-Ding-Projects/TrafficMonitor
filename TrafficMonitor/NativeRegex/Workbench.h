#pragma once
#include "Engine.h"
#include "../NativeMaterial/Theme.h"
#include <functional>
namespace NativeRegex
{
    struct EditorState;
    // The caller owns this lifetime. Closing an owner cancels its editor before callbacks can run.
    class Editor final
    {
    public:
        Editor();
        ~Editor();
        Editor(const Editor&)=delete;
        Editor& operator=(const Editor&)=delete;
        bool Open(HWND owner, HWND anchor, const Options&, std::function<std::vector<std::wstring>()> subjects,
            std::function<void(const Options&)> apply, bool chinese,
            NativeMaterial::Appearance appearance=NativeMaterial::Appearance::Light);
        void Close();
        bool IsOpen() const;
        bool Contains(HWND) const;
        bool Translate(MSG*);
    private:
        std::shared_ptr<EditorState> m_state;
    };
}
