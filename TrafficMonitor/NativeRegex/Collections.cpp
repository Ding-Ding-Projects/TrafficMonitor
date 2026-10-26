#include "../stdafx.h"
#include "Collections.h"
#include <cstdint>

namespace NativeRegex
{
namespace
{
    constexpr size_t FileLimit = 1024 * 1024, RecordLimit = 64;
    struct Handle
    {
        HANDLE value{INVALID_HANDLE_VALUE};
        explicit Handle(HANDLE h):value(h){}
        ~Handle(){if(value!=INVALID_HANDLE_VALUE)CloseHandle(value);}
        Handle(const Handle&)=delete;
    };
    void Require(bool condition,const wchar_t* message){if(!condition)throw std::wstring(message);}
    bool Unicode(const std::wstring& s)
    {
        for(size_t i=0;i<s.size();++i)
        {
            unsigned c=s[i];if(!c)return false;
            if(c>=0xd800&&c<=0xdbff){if(++i==s.size()||s[i]<0xdc00||s[i]>0xdfff)return false;}
            else if(c>=0xdc00&&c<=0xdfff)return false;
        }
        return true;
    }
    bool SameName(const std::wstring& a,const std::wstring& b)
    {return CompareStringOrdinal(a.c_str(),static_cast<int>(a.size()),b.c_str(),static_cast<int>(b.size()),TRUE)==CSTR_EQUAL;}
    void Validate(const SavedExpression& r)
    {
        Require(!r.name.empty()&&r.name.size()<=96&&Unicode(r.name)&&r.name.find_first_not_of(L" \t\r\n")!=std::wstring::npos,L"Use a nonblank name of at most 96 UTF-16 units.");
        Require(r.options.query.size()<=256&&r.replacement.size()<=4096&&Unicode(r.options.query)&&Unicode(r.replacement),L"Invalid Unicode or expression size. Pattern limit: 256; replacement limit: 4096 UTF-16 units.");
        Require(r.operation==Operation::Search||r.operation==Operation::Replace||r.operation==Operation::Split,L"Unsupported saved operation.");
    }
    using Bytes=std::vector<unsigned char>;
    void Put(Bytes& b,uint32_t n){for(int i=0;i<4;++i)b.push_back(static_cast<unsigned char>(n>>(i*8)));}
    void PutText(Bytes& b,const std::wstring& s){Put(b,static_cast<uint32_t>(s.size()));for(wchar_t c:s){b.push_back(static_cast<unsigned char>(c));b.push_back(static_cast<unsigned char>(c>>8));}}
    struct Reader
    {
        const Bytes& bytes;size_t at{};
        uint32_t Number(){Require(at+4<=bytes.size(),L"Collection is truncated.");uint32_t n=0;for(int i=0;i<4;++i)n|=uint32_t(bytes[at++])<<(i*8);return n;}
        std::wstring Text(size_t limit){size_t n=Number();Require(n<=limit&&n*2<=bytes.size()-at,L"Collection field exceeds its bounds.");std::wstring s; s.reserve(n);while(n--){unsigned c=bytes[at]|unsigned(bytes[at+1])<<8;at+=2;s.push_back(static_cast<wchar_t>(c));}return s;}
    };
    Bytes ReadBytes(const std::wstring& path,bool missingAllowed)
    {
        Handle file(CreateFileW(path.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr));
        if(file.value==INVALID_HANDLE_VALUE){Require(missingAllowed&&GetLastError()==ERROR_FILE_NOT_FOUND,L"Cannot read collection. Check data-directory access or recover a valid backup.");return {};}
        LARGE_INTEGER size{};Require(GetFileSizeEx(file.value,&size)&&size.QuadPart>0&&size.QuadPart<=FileLimit,L"Collection file is empty or exceeds 1 MiB.");
        Bytes bytes(static_cast<size_t>(size.QuadPart));DWORD read{};Require(ReadFile(file.value,bytes.data(),static_cast<DWORD>(bytes.size()),&read,nullptr)&&read==bytes.size(),L"Collection read was incomplete.");return bytes;
    }
    std::vector<SavedExpression> Decode(const Bytes& bytes)
    {
        if(bytes.empty())return {};
        Reader r{bytes};Require(r.Number()==0x58524d54&&r.Number()==0x314c4f43,L"Not a native expression collection.");
        Require(r.Number()==1,L"Unsupported collection schema version.");
        Require(r.Number()==1,L"Unsupported engine/version: this editor requires TrafficMonitor bounded NFA v1.");
        const auto count=r.Number();Require(count<=RecordLimit,L"Collection exceeds 64 records.");std::vector<SavedExpression> records;
        for(uint32_t i=0;i<count;++i)
        {
            SavedExpression item;item.name=r.Text(96);item.options.query=r.Text(256);item.replacement=r.Text(4096);
            auto flags=r.Number();Require(flags<=3,L"Unsupported saved flags.");item.options.regex=(flags&1)!=0;item.options.ignoreCase=(flags&2)!=0;
            item.operation=static_cast<Operation>(r.Number());Validate(item);
            for(const auto& prior:records)Require(!SameName(prior.name,item.name),L"Collection contains duplicate names.");records.push_back(std::move(item));
        }
        Require(r.at==bytes.size(),L"Unexpected trailing collection data.");return records;
    }
    Bytes Encode(const std::vector<SavedExpression>& records)
    {
        Bytes b;Put(b,0x58524d54);Put(b,0x314c4f43);Put(b,1);Put(b,1);Put(b,static_cast<uint32_t>(records.size()));
        for(const auto& r:records){Validate(r);PutText(b,r.name);PutText(b,r.options.query);PutText(b,r.replacement);Put(b,(r.options.regex?1:0)|(r.options.ignoreCase?2:0));Put(b,static_cast<uint32_t>(r.operation));}
        Require(b.size()<=FileLimit,L"Collection exceeds 1 MiB.");return b;
    }
    void WriteAtomic(const std::wstring& path,const Bytes& bytes,bool recovery)
    {
        const auto temporary=path+L".tmp-"+std::to_wstring(GetCurrentProcessId())+L"-"+std::to_wstring(GetTickCount64());
        {
            Handle file(CreateFileW(temporary.c_str(),GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr));
            Require(file.value!=INVALID_HANDLE_VALUE,L"Cannot create collection transaction.");DWORD written{};
            if(!WriteFile(file.value,bytes.data(),static_cast<DWORD>(bytes.size()),&written,nullptr)||written!=bytes.size()||!FlushFileBuffers(file.value))
            {CloseHandle(file.value);file.value=INVALID_HANDLE_VALUE;DeleteFileW(temporary.c_str());throw std::wstring(L"Collection write failed; saved records are unchanged.");}
        }
        bool exists=GetFileAttributesW(path.c_str())!=INVALID_FILE_ATTRIBUTES;
        // Recovery preserves the valid backup instead of replacing it with the damaged primary.
        BOOL saved=exists&&!recovery?ReplaceFileW(path.c_str(),temporary.c_str(),(path+L".bak").c_str(),0,nullptr,nullptr)
            :MoveFileExW(temporary.c_str(),path.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH);
        if(!saved){DeleteFileW(temporary.c_str());throw std::wstring(L"Atomic collection replacement failed. Retry after checking file access; recover backup if needed.");}
    }
    std::wstring CollectionPath(const std::wstring& directory)
    {
        Require(!directory.empty(),L"Data directory is unavailable.");
        return directory+(directory.back()==L'\\'||directory.back()==L'/'?L"":L"\\")+L"regex-collections.dat";
    }
}
bool SavedExpression::operator==(const SavedExpression& r) const
{return name==r.name&&options.query==r.options.query&&options.regex==r.options.regex&&options.ignoreCase==r.options.ignoreCase&&operation==r.operation&&replacement==r.replacement;}
CollectionResult UpdateCollection(const std::wstring& directory,CollectionAction action,const SavedExpression& record,const std::wstring& newName)
{
    CollectionResult result;
    try
    {
        Require(!directory.empty(),L"Data directory is unavailable.");const auto path=directory+(directory.back()==L'\\'||directory.back()==L'/'?L"":L"\\")+L"regex-collections.dat";
        Handle lock(CreateFileW((path+L".lock").c_str(),GENERIC_READ|GENERIC_WRITE,0,nullptr,OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr));
        Require(lock.value!=INVALID_HANDLE_VALUE,L"Collection is busy or inaccessible. Retry without changing the draft.");
        if(action==CollectionAction::Recover)
        {auto bytes=ReadBytes(path+L".bak",false);result.records=Decode(bytes);WriteAtomic(path,bytes,true);}
        else
        {
            result.records=Decode(ReadBytes(path,true));auto& records=result.records;
            if(action!=CollectionAction::Read)
            {
                Validate(record);auto it=std::find_if(records.begin(),records.end(),[&](const SavedExpression& r){return SameName(r.name,record.name);});
                if(action==CollectionAction::Save){Require(it==records.end(),L"Name already exists. Choose a new name; Save never overwrites.");Require(records.size()<RecordLimit,L"Collection is full (64 records).");records.push_back(record);}
                else
                {
                    Require(it!=records.end()&&*it==record,L"Selected record changed in another editor. Refresh the collection before retrying.");
                    if(action==CollectionAction::Delete)records.erase(it);
                    else if(action==CollectionAction::Rename){auto renamed=*it;renamed.name=newName;Validate(renamed);for(const auto& other:records)Require(&other==&*it||!SameName(other.name,newName),L"Name already exists.");*it=std::move(renamed);}
                    else throw std::wstring(L"Unsupported collection action.");
                }
                WriteAtomic(path,Encode(records),false);
            }
        }
        result.complete=true;
    }
    catch(const std::wstring& error){result.error=error;result.records.clear();}
    catch(...){result.error=L"Collection operation could not complete. The editor draft is unchanged.";result.records.clear();}
    return result;
}
CollectionResult ExportCollection(const std::wstring& directory,const std::wstring& destinationPath)
{
    CollectionResult result;
    try
    {
        Require(!destinationPath.empty(),L"Export destination is unavailable.");const auto path=CollectionPath(directory);
        Handle lock(CreateFileW((path+L".lock").c_str(),GENERIC_READ|GENERIC_WRITE,0,nullptr,OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr));
        Require(lock.value!=INVALID_HANDLE_VALUE,L"Collection is busy or inaccessible. Retry without changing the draft.");
        result.records=Decode(ReadBytes(path,true));WriteAtomic(destinationPath,Encode(result.records),true);result.complete=true;
    }
    catch(const std::wstring& error){result.error=error;result.records.clear();}
    catch(...){result.error=L"Collection export could not complete. The saved collection is unchanged.";result.records.clear();}
    return result;
}
CollectionResult ImportCollection(const std::wstring& directory,const std::wstring& sourcePath,ImportMode mode)
{
    CollectionResult result;
    try
    {
        Require(!sourcePath.empty(),L"Import source is unavailable.");const auto path=CollectionPath(directory);
        Handle lock(CreateFileW((path+L".lock").c_str(),GENERIC_READ|GENERIC_WRITE,0,nullptr,OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr));
        Require(lock.value!=INVALID_HANDLE_VALUE,L"Collection is busy or inaccessible. Retry without changing the draft.");
        const auto imported=Decode(ReadBytes(sourcePath,false));auto records=Decode(ReadBytes(path,true));bool changed=false;
        if(mode==ImportMode::Replace){changed=!(records==imported);records=imported;}
        else for(const auto& item:imported)
        {
            auto it=std::find_if(records.begin(),records.end(),[&](const SavedExpression& r){return SameName(r.name,item.name);});
            if(it!=records.end()){if(*it==item)continue;throw std::wstring(L"Import conflict: \"")+item.name+L"\" already exists with different content. Nothing was imported.";}
            records.push_back(item);changed=true;
        }
        Require(records.size()<=RecordLimit,L"Import would exceed 64 records. Nothing was imported.");
        if(changed)WriteAtomic(path,Encode(records),false);
        result.records=std::move(records);result.complete=true;
    }
    catch(const std::wstring& error){result.error=error;result.records.clear();}
    catch(...){result.error=L"Collection import could not complete. The saved collection is unchanged.";result.records.clear();}
    return result;
}
}
