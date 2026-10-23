#pragma once
#include <map>
#include <string>
#include <vector>
#include <stdexcept>

namespace NativeUpdate::Detail
{
    // Bounded JSON reader for release metadata. Duplicate keys are rejected.
    struct Json
    {
        enum Type { Null, String, Number, Boolean, Object, Array } type{Null};
        std::string value;
        std::map<std::string, Json> object;
        std::vector<Json> array;
        const Json& at(const std::string& key) const
        { if (type != Object) throw std::runtime_error("object"); return object.at(key); }
        std::string text() const
        { if (type != String) throw std::runtime_error("string"); return value; }
        bool flag() const
        { if (type != Boolean) throw std::runtime_error("boolean"); return value == "true"; }
    };
    class Reader
    {
        const std::string& input; size_t pos{};
        char take() { if (pos >= input.size()) throw std::runtime_error("end"); return input[pos++]; }
        void space() { while (pos < input.size() && (input[pos]==' ' || input[pos]=='\r' || input[pos]=='\n' || input[pos]=='\t')) ++pos; }
        std::string string()
        {
            if (take() != '"') throw std::runtime_error("quote");
            std::string out;
            for (;;)
            {
                unsigned char c=take(); if(c=='"') return out;
                if(c<32) throw std::runtime_error("control");
                if(c!='\\') { out+=static_cast<char>(c); continue; }
                c=take();
                switch(c)
                {
                case '"': case '\\': case '/': out+=static_cast<char>(c); break;
                case 'b': out+='\b'; break; case 'f':out+='\f';break;
                case 'n':out+='\n';break;case 'r':out+='\r';break;case 't':out+='\t';break;
                case 'u':
                {
                    unsigned code=0;
                    for(int i=0;i<4;++i) { char h=take(); code*=16; if(h>='0'&&h<='9')code+=h-'0';else if(h>='a'&&h<='f')code+=h-'a'+10;else if(h>='A'&&h<='F')code+=h-'A'+10;else throw std::runtime_error("hex"); }
                    // Protocol fields are ASCII; other JSON strings can retain escaped Unicode.
                    if(code<128)out+=static_cast<char>(code);else out+='?';
                    break;
                }
                default:throw std::runtime_error("escape");
                }
            }
        }
        Json read(unsigned depth)
        {
            if(depth>24)throw std::runtime_error("depth");space();Json result;
            if(pos>=input.size())throw std::runtime_error("empty");char c=input[pos];
            if(c=='"') {result.type=Json::String;result.value=string();return result;}
            if(c=='{'||c=='[')
            {
                ++pos;bool object=c=='{';result.type=object?Json::Object:Json::Array;space();
                if(pos<input.size()&&input[pos]==(object?'}':']')) {++pos;return result;}
                for(size_t count=0;;++count)
                {
                    if(count>4096)throw std::runtime_error("items");space();std::string key;
                    if(object){key=string();space();if(take()!=':')throw std::runtime_error("colon");}
                    auto item=read(depth+1);
                    if(object){if(!result.object.emplace(key,std::move(item)).second)throw std::runtime_error("duplicate");}
                    else result.array.push_back(std::move(item));
                    space();char sep=take();if(sep==(object?'}':']'))return result;if(sep!=',')throw std::runtime_error("comma");
                }
            }
            size_t start=pos;while(pos<input.size()&&input[pos]!=','&&input[pos]!=']'&&input[pos]!='}'&&input[pos]!=' '&&input[pos]!='\r'&&input[pos]!='\n'&&input[pos]!='\t')++pos;
            result.value=input.substr(start,pos-start);
            if(result.value=="true"||result.value=="false")result.type=Json::Boolean;
            else if(result.value=="null")result.type=Json::Null;
            else
            {
                result.type=Json::Number;const auto& value=result.value;size_t i=0;
                if(i<value.size()&&value[i]=='-')++i;
                if(i==value.size())throw std::runtime_error("number");
                if(value[i]=='0')++i;
                else {if(value[i]<'1'||value[i]>'9')throw std::runtime_error("number");while(i<value.size()&&value[i]>='0'&&value[i]<='9')++i;}
                if(i<value.size()&&value[i]=='.'){++i;size_t start=i;while(i<value.size()&&value[i]>='0'&&value[i]<='9')++i;if(i==start)throw std::runtime_error("fraction");}
                if(i<value.size()&&(value[i]=='e'||value[i]=='E')){++i;if(i<value.size()&&(value[i]=='+'||value[i]=='-'))++i;size_t start=i;while(i<value.size()&&value[i]>='0'&&value[i]<='9')++i;if(i==start)throw std::runtime_error("exponent");}
                if(i!=value.size())throw std::runtime_error("number trailing");
            }
            return result;
        }
    public:
        explicit Reader(const std::string& text):input(text){}
        Json parse(){auto result=read(0);space();if(pos!=input.size())throw std::runtime_error("trailing");return result;}
    };
}
