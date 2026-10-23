#include "../stdafx.h"
#include "Engine.h"
#include <array>
#include <chrono>
#include <stdexcept>

namespace NativeRegex
{
namespace
{
    constexpr size_t PatternLimit=256, StateLimit=512, TextLimit=8192, CorpusLimit=65536, RowLimit=512, ResultLimit=256, OutputLimit=65536;
    constexpr unsigned StepLimit=4000000;
    wchar_t Fold(wchar_t c) { return c>=L'A'&&c<=L'Z' ? c+32 : c; }
    struct Budget
    {
        const std::atomic_bool& cancel; unsigned steps{};
        std::chrono::steady_clock::time_point start{std::chrono::steady_clock::now()};
        void Tick()
        {
            if(++steps>StepLimit) throw std::wstring(L"Operation limit exceeded; accepted filter retained.");
            if((steps&255)==0 && (cancel.load() || std::chrono::steady_clock::now()-start>std::chrono::milliseconds(500)))
                throw std::wstring(L"Canceled or 500 ms limit exceeded; accepted filter retained.");
        }
    };
    enum class Kind { Character, Any, Class, Split, Begin, End, Match };
    struct State { Kind kind{}; wchar_t character{}; std::wstring ranges; bool negate{}; int a{-1},b{-1}; };
    struct Patch { int state; bool second; };
    struct Fragment { int start; std::vector<Patch> out; };
    class Program
    {
        std::wstring pattern; size_t pos{}; bool fold{}; std::vector<State> states; int entry{};
        int Add(State state) { if(states.size()>=StateLimit) Fail(L"Automaton exceeds 512 states"); states.push_back(std::move(state));return static_cast<int>(states.size()-1); }
        [[noreturn]] void Fail(const wchar_t* message) { throw std::wstring(message)+L" at UTF-16 offset "+std::to_wstring(pos)+L"."; }
        void Link(const std::vector<Patch>& out,int destination) { for(auto p:out)(p.second?states[p.state].b:states[p.state].a)=destination; }
        Fragment Atom()
        {
            if(pos==pattern.size())Fail(L"Expected expression");
            wchar_t c=pattern[pos++];
            if(c==L'(')
            {
                if(pattern.substr(pos,2)!=L"?:")Fail(L"Only noncapturing (?:...) groups are supported; captures/lookaround unavailable");
                pos+=2;auto f=Alternative();if(pos==pattern.size()||pattern[pos++]!=L')')Fail(L"Unclosed group");return f;
            }
            State s;
            if(c==L'.')s.kind=Kind::Any;
            else if(c==L'^')s.kind=Kind::Begin;
            else if(c==L'$')s.kind=Kind::End;
            else if(c==L'[')
            {
                s.kind=Kind::Class;if(pos<pattern.size()&&pattern[pos]==L'^'){s.negate=true;++pos;}
                while(pos<pattern.size()&&pattern[pos]!=L']')
                {
                    wchar_t first=ClassCharacter();wchar_t last=first;
                    if(pos+1<pattern.size()&&pattern[pos]==L'-'&&pattern[pos+1]!=L']'){++pos;last=ClassCharacter();}
                    if(first>last)Fail(L"Reversed range");s.ranges+=first;s.ranges+=last;
                }
                if(s.ranges.empty()||pos==pattern.size())Fail(L"Empty or unclosed character class");++pos;
            }
            else if(c==L'\\')
            {
                if(pos==pattern.size())Fail(L"Trailing escape");c=pattern[pos++];
                if(c==L'd'||c==L'w'||c==L's')
                {
                    s.kind=Kind::Class;
                    s.ranges=c==L'd'?L"09":c==L'w'?L"09AZaz__":L"\t\r  ";
                }
                else {s.kind=Kind::Character;s.character=Escaped(c);}
            }
            else
            {
                if(std::wstring(L"*+?{}]|)").find(c)!=std::wstring::npos)Fail(L"Unexpected or unsupported syntax; escape punctuation for literals");
                s.kind=Kind::Character;s.character=c;
            }
            int id=Add(std::move(s));return {id,{{id,false}}};
        }
        wchar_t Escaped(wchar_t c)
        {
            if(c==L'n')return L'\n';if(c==L'r')return L'\r';if(c==L't')return L'\t';
            if(std::wstring(L"\\.^$|?*+()[]{}-").find(c)==std::wstring::npos)Fail(L"Unsupported escape (no backreferences, Unicode properties or mode modifiers)");return c;
        }
        wchar_t ClassCharacter() { wchar_t c=pattern[pos++];if(c==L'\\'){if(pos==pattern.size())Fail(L"Trailing class escape");c=Escaped(pattern[pos++]);}return c; }
        Fragment Sequence()
        {
            Fragment result{-1,{}};
            while(pos<pattern.size()&&pattern[pos]!=L')'&&pattern[pos]!=L'|')
            {
                auto f=Atom();
                if(pos<pattern.size()&&std::wstring(L"?*+").find(pattern[pos])!=std::wstring::npos)
                {
                    wchar_t q=pattern[pos++];State split;split.kind=Kind::Split;split.a=f.start;int id=Add(split);
                    if(q==L'?'){f.out.push_back({id,true});f.start=id;}
                    else {Link(f.out,id);f.out={{id,true}};if(q==L'*')f.start=id;}
                    if(pos<pattern.size()&&std::wstring(L"?*+").find(pattern[pos])!=std::wstring::npos)Fail(L"Lazy, possessive and repeated quantifiers are unavailable");
                }
                if(result.start<0)result=std::move(f);else{Link(result.out,f.start);result.out=std::move(f.out);}
            }
            if(result.start<0){State s;s.kind=Kind::Split;int id=Add(s);return {id,{{id,false}}};}return result;
        }
        Fragment Alternative()
        {
            auto result=Sequence();while(pos<pattern.size()&&pattern[pos]==L'|'){++pos;auto right=Sequence();State s;s.kind=Kind::Split;s.a=result.start;s.b=right.start;result.start=Add(s);result.out.insert(result.out.end(),right.out.begin(),right.out.end());}return result;
        }
        void Closure(int id,size_t at,size_t length,std::vector<int>& list,std::array<bool,StateLimit>& seen,Budget& budget) const
        {
            budget.Tick();if(id<0||seen[id])return;seen[id]=true;const auto& s=states[id];
            if(s.kind==Kind::Split){Closure(s.a,at,length,list,seen,budget);Closure(s.b,at,length,list,seen,budget);}
            else if(s.kind==Kind::Begin){if(at==0)Closure(s.a,at,length,list,seen,budget);}
            else if(s.kind==Kind::End){if(at==length)Closure(s.a,at,length,list,seen,budget);}
            else list.push_back(id);
        }
        bool Accept(const State& s,wchar_t c) const
        {
            if(s.kind==Kind::Any)return c!=L'\r'&&c!=L'\n';
            if(s.kind==Kind::Character)return fold?Fold(c)==Fold(s.character):c==s.character;
            if(s.kind!=Kind::Class)return false;
            bool found=false;for(size_t i=0;i<s.ranges.size();i+=2)
            {auto a=s.ranges[i],b=s.ranges[i+1];if(c>=a&&c<=b)found=true;if(fold){wchar_t other=c>=L'a'&&c<=L'z'?c-32:Fold(c);if(other>=a&&other<=b)found=true;}}
            return found!=s.negate;
        }
    public:
        Program(const Options& options):pattern(options.query),fold(options.ignoreCase)
        {
            if(pattern.size()>PatternLimit)Fail(L"Pattern exceeds 256 UTF-16 units");
            if(options.regex){auto f=Alternative();if(pos!=pattern.size())Fail(L"Unexpected closing group");State s;s.kind=Kind::Match;int end=Add(s);Link(f.out,end);entry=f.start;}
            else
            {
                // Literal mode has no syntax interpretation.
                entry=0;for(wchar_t c:pattern){State s;s.kind=Kind::Character;s.character=c;s.a=static_cast<int>(states.size()+1);Add(s);}State s;s.kind=Kind::Match;Add(s);
            }
        }
        bool Find(const std::wstring& text,size_t from,Span& span,Budget& budget) const
        {
            for(size_t start=from;start<=text.size();++start)
            {
                std::vector<int> current;std::array<bool,StateLimit> seen{};Closure(entry,start,text.size(),current,seen,budget);
                bool matched=false;size_t end=start;
                for(size_t at=start;;++at)
                {
                    for(int id:current){budget.Tick();if(states[id].kind==Kind::Match){matched=true;end=at;}}
                    if(at==text.size()||current.empty())break;
                    std::vector<int> next;seen.fill(false);
                    for(int id:current){budget.Tick();if(Accept(states[id],text[at]))Closure(states[id].a,at+1,text.size(),next,seen,budget);}
                    current=std::move(next);
                }
                if(matched){span={start,end-start};return true;}
            }
            return false;
        }
    };
    Result Evaluate(const Options& options,const std::vector<std::wstring>& texts,Operation operation,const std::wstring& replacement,const std::atomic_bool& cancel)
    {
        Result result;
        try
        {
            if(texts.size()>RowLimit||replacement.size()>4096)throw std::wstring(L"Limit: 512 rows and 4096 replacement units.");
            size_t total=0;for(const auto& t:texts){total+=t.size();if(t.size()>TextLimit||total>CorpusLimit)throw std::wstring(L"Limit: 8192 units per subject, 65536 per operation.");}
            Program program(options);Budget budget{cancel};
            if(operation==Operation::Filter)
            {for(const auto& text:texts){Span span;result.rows.push_back(program.Find(text,0,span,budget));}}
            else if(!texts.empty())
            {
                const auto& text=texts.front();size_t from=0,last=0;Span span;
                while(from<=text.size()&&program.Find(text,from,span,budget))
                {
                    if(result.matches.size()>=ResultLimit)throw std::wstring(L"More than 256 matches; narrow the subject or pattern.");
                    result.matches.push_back(span);
                    if(operation==Operation::Replace)result.output+=text.substr(last,span.start-last)+replacement;
                    if(operation==Operation::Split)result.output+=text.substr(last,span.start-last)+L"\r\n";
                    last=span.start+span.length;from=last+(span.length==0?1:0);
                    if(result.output.size()>OutputLimit)throw std::wstring(L"Output exceeds 65536 UTF-16 units.");
                }
                if(operation!=Operation::Search)result.output+=text.substr(last);
            }
            if(cancel.load())throw std::wstring(L"Canceled; accepted filter retained.");
            if(result.output.size()>OutputLimit)throw std::wstring(L"Output exceeds 65536 UTF-16 units.");
            result.complete=true;
        }
        catch(const std::wstring& error){result.error=error;result.rows.clear();result.matches.clear();result.output.clear();}
        catch(...){result.error=L"Evaluation unavailable; accepted filter retained.";result.rows.clear();result.matches.clear();result.output.clear();}
        return result;
    }
}
Session::~Session(){Cancel();if(m_worker.valid())m_worker.wait();}
void Session::Cancel(){++m_generation;m_pending.reset();if(m_cancel)m_cancel->store(true);}
void Session::Request(const Options& options,std::vector<std::wstring> text,Operation operation,std::wstring replacement)
{
    Cancel();m_pending=std::make_unique<RequestData>(RequestData{options,std::move(text),operation,std::move(replacement),m_generation});if(!m_worker.valid())Start();
}
void Session::Start()
{
    auto request=std::move(*m_pending);m_pending.reset();m_running=request.generation;m_cancel=std::make_shared<std::atomic_bool>(false);auto cancel=m_cancel;
    m_worker=std::async(std::launch::async,[request=std::move(request),cancel]{return Evaluate(request.options,request.text,request.operation,request.replacement,*cancel);});
}
bool Session::Poll(Result& result)
{
    if(!m_worker.valid()||m_worker.wait_for(std::chrono::milliseconds(0))!=std::future_status::ready)return false;
    auto completed=m_worker.get();const bool current=m_running==m_generation;if(m_pending)Start();if(current){result=std::move(completed);return true;}return false;
}
void Filter::Request(const std::vector<std::wstring>& text)
{
    if(m_initialized&&m_text==text&&m_requested.query==options.query&&m_requested.regex==options.regex&&m_requested.ignoreCase==options.ignoreCase)return;
    if(!m_initialized)m_rows.assign(text.size(),true);
    m_initialized=true;m_text=text;m_requested=options;status=L"Evaluating; previous rows retained.";m_session.Request(options,text);
}
bool Filter::Visible(size_t row) const
{
    if(row>=m_text.size())return false;
    if(m_acceptedText.empty())return options.query.empty();
    for(size_t i=0;i<m_acceptedText.size();++i)if(m_acceptedText[i]==m_text[row])return i<m_rows.size()&&m_rows[i];
    return false;
}
bool Filter::Poll(){Result r;if(!m_session.Poll(r))return false;if(r.complete){m_rows=std::move(r.rows);m_acceptedText=m_text;status=L"";}else status=r.error;return true;}
}
