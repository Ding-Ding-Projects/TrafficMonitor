#include "DraftValidation.h"
#include <iostream>
#include <vector>
#include <climits>
using namespace NativeSettings;
int main()
{
    int checks=0,failures=0;
    auto check=[&](bool ok,const char* name){++checks;if(!ok){++failures;std::cerr<<"FAIL "<<name<<'\n';}};
    for(const auto* text:{L"",L"-1",L"+1",L" 1",L"1 ",L"1ms",L"1.0",L"0",L"101",L"2147483648",L"999999999999999999999999999999999999999999999999999999999999999999",L"１２"})
    {int v=73;check(!ParseNumber(text,1,100,1,v)&&v==73,"invalid memory draft preserves destination");}
    for(const auto* text:{L"1",L"80",L"100"}){int v=0;check(ParseNumber(text,1,100,1,v),"memory limits accepted");}
    for(const auto* text:{L"1",L"200",L"32767"}){int v=0;check(ParseNumber(text,1,32767,1,v),"traffic bounds accepted");}
    for(const auto* text:{L"0",L"32768"}){int v=91;check(!ParseNumber(text,1,32767,1,v)&&v==91,"traffic bounds rejected");}
    for(const auto* text:{L"1",L"80",L"120"}){int v=0;check(ParseNumber(text,1,120,1,v),"temperature bounds accepted");}
    for(const auto* text:{L"0",L"121"}){int v=0;check(!ParseNumber(text,1,120,1,v),"temperature bounds rejected");}
    for(const auto* text:{L"200",L"1000",L"30000"}){int v=0;check(ParseNumber(text,200,30000,100,v),"interval bounds accepted");}
    for(const auto* text:{L"199",L"250",L"30001"}){int v=0;check(!ParseNumber(text,200,30000,100,v),"interval bounds and step rejected");}
    int large=0;check(ParseNumber(L"2147483647",0,INT_MAX,1,large)&&large==INT_MAX,"integer maximum accepted without overflow");
    check(!ParseNumber(L"2147483648",0,INT_MAX,1,large)&&large==INT_MAX,"integer overflow rejected before arithmetic");
    int retained=95;check(ReadNumberDraft(L"unavailable",1,120,1,false,retained)&&retained==95,"unavailable capability retains stored threshold");
    check(!ReadNumberDraft(L"unavailable",1,120,1,true,retained)&&retained==95,"available capability rejects invalid threshold");
    check(ValidTrafficUnit(0)&&ValidTrafficUnit(1)&&!ValidTrafficUnit(-1)&&!ValidTrafficUnit(2),"exact traffic units");
    check(ValidTrafficUnit(7,false),"unavailable capability retains unit without mutation");
    EditorBinding binding;const auto first=binding.Open(false);
    check(binding.Accepts(first),"new editor accepts its callback");
    for(int i=0;i<5;++i)check(binding.Open(true)==first&&binding.Accepts(first),"repeated focus preserves original callback binding");
    binding.Invalidate();check(!binding.Accepts(first),"group or query change invalidates old callbacks");
    const auto next=binding.Open(false);check(binding.Accepts(next)&&!binding.Accepts(first),"new owner accepts only new callback");
    binding.Invalidate();check(!binding.Accepts(next),"owner close invalidates pending callback");
    std::cout<<"Native settings checks: "<<checks-failures<<" passed, "<<failures<<" failed\n";return failures?1:0;
}
