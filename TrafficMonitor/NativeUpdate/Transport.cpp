#define NOMINMAX
#include "Transport.h"
#include <winhttp.h>
#include <bcrypt.h>
#include <fstream>
#include <stdexcept>
#include <memory>
#pragma comment(lib,"winhttp.lib")
#pragma comment(lib,"bcrypt.lib")

namespace NativeUpdate::Detail
{
    struct Internet { HINTERNET handle{}; ~Internet(){if(handle)WinHttpCloseHandle(handle);} };
    std::wstring Wide(const std::string& text)
    {
        int n=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,text.data(),static_cast<int>(text.size()),nullptr,0);
        if(n<=0)throw std::runtime_error("encoding");std::wstring result(n,0);
        MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,text.data(),static_cast<int>(text.size()),result.data(),n);return result;
    }
    std::vector<unsigned char> Fetch(const std::wstring& initial,size_t limit,const std::atomic<bool>& cancel,ULONGLONG deadline)
    {
        std::wstring url=initial;
        Internet session{WinHttpOpen(L"TrafficMonitor/NativeUpdater",WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,WINHTTP_NO_PROXY_NAME,WINHTTP_NO_PROXY_BYPASS,0)};
        if(!session.handle)throw TransportError("session",GetLastError());
        WinHttpSetTimeouts(session.handle,5000,5000,5000,5000);
        for(int redirect=0;redirect<5;++redirect)
        {
            if(cancel||GetTickCount64()>deadline)throw std::runtime_error("cancel");
            URL_COMPONENTS parts{};parts.dwStructSize=sizeof(parts);parts.dwHostNameLength=parts.dwUrlPathLength=parts.dwExtraInfoLength=parts.dwUserNameLength=parts.dwPasswordLength=static_cast<DWORD>(-1);
            if(!WinHttpCrackUrl(url.c_str(),0,0,&parts)||parts.nScheme!=INTERNET_SCHEME_HTTPS||parts.nPort!=443||parts.dwUserNameLength||parts.dwPasswordLength)throw std::runtime_error("url");
            std::wstring host(parts.lpszHostName,parts.dwHostNameLength);
            if(host!=L"api.github.com"&&host!=L"github.com"&&host!=L"release-assets.githubusercontent.com"&&host!=L"objects.githubusercontent.com")throw std::runtime_error("host");
            std::wstring path(parts.lpszUrlPath,parts.dwUrlPathLength);if(parts.dwExtraInfoLength)path.append(parts.lpszExtraInfo,parts.dwExtraInfoLength);
            Internet connection{WinHttpConnect(session.handle,host.c_str(),443,0)};
            Internet request{connection.handle?WinHttpOpenRequest(connection.handle,L"GET",path.c_str(),nullptr,WINHTTP_NO_REFERER,WINHTTP_DEFAULT_ACCEPT_TYPES,WINHTTP_FLAG_SECURE):nullptr};
            if(!request.handle)throw TransportError("request",GetLastError());
            DWORD redirects=WINHTTP_OPTION_REDIRECT_POLICY_NEVER;WinHttpSetOption(request.handle,WINHTTP_OPTION_REDIRECT_POLICY,&redirects,sizeof(redirects));
            DWORD auth=WINHTTP_AUTOLOGON_SECURITY_LEVEL_HIGH;WinHttpSetOption(request.handle,WINHTTP_OPTION_AUTOLOGON_POLICY,&auth,sizeof(auth));
            if(!WinHttpSendRequest(request.handle,L"Accept: application/vnd.github+json\r\nX-GitHub-Api-Version: 2022-11-28\r\n",static_cast<DWORD>(-1),nullptr,0,0,0)||!WinHttpReceiveResponse(request.handle,nullptr))throw TransportError("response",GetLastError());
            DWORD status{},length=sizeof(status);if(!WinHttpQueryHeaders(request.handle,WINHTTP_QUERY_STATUS_CODE|WINHTTP_QUERY_FLAG_NUMBER,nullptr,&status,&length,nullptr))throw TransportError("status",GetLastError());
            if(status>=300&&status<=399)
            {
                wchar_t location[8192]{};length=sizeof(location);
                if(!WinHttpQueryHeaders(request.handle,WINHTTP_QUERY_LOCATION,nullptr,location,&length,nullptr))throw TransportError("redirect",GetLastError());
                url=location;continue;
            }
            if(status!=200)throw TransportError("http",ERROR_WINHTTP_INVALID_SERVER_RESPONSE,status);
            std::vector<unsigned char> bytes;unsigned char buffer[65536];DWORD read{};
            do
            {
                if(cancel||GetTickCount64()>deadline)throw std::runtime_error("cancel");
                if(!WinHttpReadData(request.handle,buffer,sizeof(buffer),&read))throw TransportError("read",GetLastError());
                if(read>limit-bytes.size())throw std::runtime_error("size");bytes.insert(bytes.end(),buffer,buffer+read);
            }while(read);
            return bytes;
        }
        throw std::runtime_error("redirect limit");
    }
    std::string Digest(const std::vector<unsigned char>& bytes,bool sha1)
    {
        BCRYPT_ALG_HANDLE algorithm{};BCRYPT_HASH_HANDLE hash{};
        if(BCryptOpenAlgorithmProvider(&algorithm,sha1?BCRYPT_SHA1_ALGORITHM:BCRYPT_SHA256_ALGORITHM,nullptr,0)<0)throw std::runtime_error("hash provider");
        unsigned char digest[32]{};DWORD length=sha1?20:32;
        NTSTATUS result=BCryptCreateHash(algorithm,&hash,nullptr,0,nullptr,0,0);
        if(result>=0)result=BCryptHashData(hash,const_cast<PUCHAR>(bytes.data()),static_cast<ULONG>(bytes.size()),0);
        if(result>=0)result=BCryptFinishHash(hash,digest,length,0);
        if(hash)BCryptDestroyHash(hash);BCryptCloseAlgorithmProvider(algorithm,0);
        if(result<0)throw std::runtime_error("hash");
        const char* hex="0123456789abcdef";std::string out;for(DWORD i=0;i<length;++i){out+=hex[digest[i]>>4];out+=hex[digest[i]&15];}return out;
    }
    bool PlainDirectory(const std::filesystem::path& path)
    {
        auto current=path;
        while(!current.empty())
        {
            DWORD attributes=GetFileAttributesW(current.c_str());
            if(attributes==INVALID_FILE_ATTRIBUTES||(attributes&FILE_ATTRIBUTE_REPARSE_POINT)||!(attributes&FILE_ATTRIBUTE_DIRECTORY))return false;
            auto parent=current.parent_path();if(parent==current)break;current=parent;
        }
        return true;
    }
    std::vector<unsigned char> Read(const std::filesystem::path& path,size_t limit)
    {
        DWORD attributes=GetFileAttributesW(path.c_str());
        if(attributes==INVALID_FILE_ATTRIBUTES||(attributes&(FILE_ATTRIBUTE_REPARSE_POINT|FILE_ATTRIBUTE_DIRECTORY))||!PlainDirectory(path.parent_path()))throw std::runtime_error("file path");
        std::ifstream in(path,std::ios::binary|std::ios::ate);if(!in)throw std::runtime_error("file");auto size=in.tellg();
        if(size<0||static_cast<unsigned long long>(size)>limit)throw std::runtime_error("file size");
        std::vector<unsigned char> bytes(static_cast<size_t>(size));in.seekg(0);if(!bytes.empty()&&!in.read(reinterpret_cast<char*>(bytes.data()),bytes.size()))throw std::runtime_error("file read");return bytes;
    }
    void Write(const std::filesystem::path& path,const std::vector<unsigned char>& bytes)
    {
        if(!PlainDirectory(path.parent_path()))throw std::runtime_error("directory");
        HANDLE file=CreateFileW(path.c_str(),GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);
        if(file==INVALID_HANDLE_VALUE)throw std::runtime_error("create");DWORD written{};
        bool ok=WriteFile(file,bytes.data(),static_cast<DWORD>(bytes.size()),&written,nullptr)&&written==bytes.size()&&FlushFileBuffers(file);CloseHandle(file);
        if(!ok)throw std::runtime_error("write");
    }
}
