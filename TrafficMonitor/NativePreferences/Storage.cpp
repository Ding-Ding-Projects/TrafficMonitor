#include "Storage.h"
#include <bcrypt.h>
#include <vector>
#pragma comment(lib,"bcrypt.lib")
namespace NativePreferences::Storage
{
    namespace
    {
        std::wstring Hash(const std::vector<unsigned char>& bytes)
        {
            BCRYPT_ALG_HANDLE algorithm{}; BCRYPT_HASH_HANDLE hash{}; DWORD size{},used{};
            if(BCryptOpenAlgorithmProvider(&algorithm,BCRYPT_SHA256_ALGORITHM,nullptr,0)<0)return {};
            if(BCryptGetProperty(algorithm,BCRYPT_OBJECT_LENGTH,reinterpret_cast<PUCHAR>(&size),sizeof(size),&used,0)<0){BCryptCloseAlgorithmProvider(algorithm,0);return {};}
            std::vector<unsigned char> object(size); unsigned char digest[32]{};
            bool ok=BCryptCreateHash(algorithm,&hash,object.data(),size,nullptr,0,0)>=0;
            if(ok)ok=BCryptHashData(hash,const_cast<PUCHAR>(bytes.data()),static_cast<ULONG>(bytes.size()),0)>=0 && BCryptFinishHash(hash,digest,sizeof(digest),0)>=0;
            if(hash)BCryptDestroyHash(hash);BCryptCloseAlgorithmProvider(algorithm,0);
            if(!ok)return {};std::wstring result;const wchar_t* hex=L"0123456789abcdef";
            for(auto b:digest){result+=hex[b>>4];result+=hex[b&15];}return result;
        }
    }
    ReadResult Read(const std::wstring& path)
    {
        ReadResult result;HANDLE file=CreateFileW(path.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
        if(file==INVALID_HANDLE_VALUE){result.error=GetLastError();result.missing=result.error==ERROR_FILE_NOT_FOUND;result.valid=result.missing;if(result.missing)result.error=0;result.protectedFile=!result.missing;result.fingerprint=result.missing?L"missing":L"";return result;}
        LARGE_INTEGER length{};if(!GetFileSizeEx(file,&length)||length.QuadPart<0||length.QuadPart>16384){result.error=ERROR_INVALID_DATA;result.protectedFile=true;CloseHandle(file);return result;}
        std::vector<unsigned char> bytes(static_cast<size_t>(length.QuadPart));DWORD read{};
        bool ok=ReadFile(file,bytes.data(),static_cast<DWORD>(bytes.size()),&read,nullptr)!=FALSE&&read==bytes.size();CloseHandle(file);
        if(!ok){result.error=ERROR_READ_FAULT;result.protectedFile=true;return result;}result.fingerprint=Hash(bytes);
        if(result.fingerprint.empty()){result.error=ERROR_INVALID_DATA;result.protectedFile=true;return result;}
        // Exact bounded schema; neither partial documents nor unknown fields are downgraded.
        std::string text(bytes.begin(),bytes.end());
        for(int language=0;language<3;++language)for(int appearance=0;appearance<3;++appearance)
        {
            const std::string expected="[interface]\r\nschemaVersion=1\r\nlanguage="+std::to_string(language)+"\r\nappearance="+std::to_string(appearance)+"\r\n";
            if(text==expected){result.values={static_cast<Language>(language),static_cast<Appearance>(appearance)};result.valid=true;result.error=0;return result;}
        }
        result.error=ERROR_INVALID_DATA;result.protectedFile=true;return result;
    }
    Result Write(const std::wstring& path,Values value,const std::wstring& expected,ReadResult& stored)
    {
        auto before=Read(path);if(!before.valid)return Result::Protected;if(before.fingerprint!=expected)return Result::Conflict;
        // A separate, exclusively created temporary file belongs only to this operation.
        std::wstring temporary;HANDLE file=INVALID_HANDLE_VALUE;
        for(unsigned attempt=0;attempt<16 && file==INVALID_HANDLE_VALUE;++attempt)
        {temporary=path+L"."+std::to_wstring(GetCurrentProcessId())+L"."+std::to_wstring(GetTickCount64())+L"."+std::to_wstring(attempt)+L".tmp";file=CreateFileW(temporary.c_str(),GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);if(file==INVALID_HANDLE_VALUE&&GetLastError()!=ERROR_FILE_EXISTS)return Result::WriteFailed;}
        if(file==INVALID_HANDLE_VALUE)return Result::WriteFailed;
        const std::string data="[interface]\r\nschemaVersion=1\r\nlanguage="+std::to_string(static_cast<int>(value.language))+"\r\nappearance="+std::to_string(static_cast<int>(value.appearance))+"\r\n";DWORD written{};
        bool ok=WriteFile(file,data.data(),static_cast<DWORD>(data.size()),&written,nullptr)!=FALSE&&written==data.size()&&FlushFileBuffers(file)!=FALSE;CloseHandle(file);
        const auto latest=Read(path);if(!ok||!latest.valid||latest.fingerprint!=expected){DeleteFileW(temporary.c_str());return ok?Result::Conflict:Result::WriteFailed;}
        if(!MoveFileExW(temporary.c_str(),path.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)){DeleteFileW(temporary.c_str());return Result::WriteFailed;}
        stored=Read(path);return stored.valid&&!stored.missing&&stored.values==value?Result::Saved:Result::Uncertain;
    }
}
