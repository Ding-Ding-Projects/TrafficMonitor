#pragma once
#include <string>
namespace NativeSettings
{
    inline bool ParseNumber(const std::wstring& text,int minimum,int maximum,int step,int& value)
    {
        if(text.empty()||maximum<0||step<1)return false;
        int parsed=0;
        for(const auto c:text)
        {
            if(c<L'0'||c>L'9')return false;
            const int digit=c-L'0';
            if(digit>maximum||parsed>(maximum-digit)/10)return false;
            parsed=parsed*10+digit;
        }
        if(parsed<minimum||parsed%step)return false;
        value=parsed;return true;
    }
    inline bool ReadNumberDraft(const std::wstring& text,int minimum,int maximum,int step,bool available,int& value)
    {return !available||ParseNumber(text,minimum,maximum,step,value);}
    inline bool ValidTrafficUnit(int unit,bool available=true){return !available||unit==0||unit==1;}
    class EditorBinding
    {
    public:
        unsigned long long Open(bool alreadyOpen){return alreadyOpen?m_generation:++m_generation;}
        void Invalidate(){++m_generation;}
        bool Accepts(unsigned long long value)const{return value==m_generation;}
    private:
        unsigned long long m_generation{};
    };
}
