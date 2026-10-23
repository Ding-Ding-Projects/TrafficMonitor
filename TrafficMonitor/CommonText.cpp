#include "stdafx.h"
#include "Common.h"

wstring CCommon::StrToUnicode(const char* str, bool utf8)
{
    wstring result;
    int size;
    size = MultiByteToWideChar((utf8 ? CP_UTF8 : CP_ACP), 0, str, -1, NULL, 0);
    if (size <= 0) return wstring();
    wchar_t* str_unicode = new wchar_t[size + 1];
    MultiByteToWideChar((utf8 ? CP_UTF8 : CP_ACP), 0, str, -1, str_unicode, size);
    result.assign(str_unicode);
    delete[] str_unicode;
    return result;
}

string CCommon::UnicodeToStr(const wchar_t* wstr, bool utf8)
{
    string result;
    int size{ 0 };
    size = WideCharToMultiByte((utf8 ? CP_UTF8 : CP_ACP), 0, wstr, -1, NULL, 0, NULL, NULL);
    if (size <= 0) return string();
    char* str = new char[size + 1];
    WideCharToMultiByte((utf8 ? CP_UTF8 : CP_ACP), 0, wstr, -1, str, size, NULL, NULL);
    result.assign(str);
    delete[] str;
    return result;
}

wstring CCommon::AsciiToUnicode(const string& str)
{
    std::wstring result;
    result.resize(str.size());
    for (size_t i{}; i < str.size(); i++)
        result[i] = str[i];
    return result;
}

string CCommon::AsciiToStr(const std::wstring& wstr)
{
    std::string result;
    result.resize(wstr.size());
    for (size_t i{}; i < wstr.size(); i++)
        result[i] = static_cast<char>(wstr[i]);
    return result;
}

template<class T>
static void _StringNormalize(T& str)
{
    if (str.empty()) return;

    int size = static_cast<int>(str.size());  //字符串的长度
    if (size < 0) return;
    int index1 = 0;     //字符串中第1个不是空格或控制字符的位置
    int index2 = size - 1;  //字符串中最后一个不是空格或控制字符的位置
    while (index1 < size && str[index1] >= 0 && str[index1] <= 32)
        index1++;
    while (index2 >= 0 && str[index2] >= 0 && str[index2] <= 32)
        index2--;
    if (index1 > index2)    //如果index1 > index2，说明字符串全是空格或控制字符
        str.clear();
    else if (index1 == 0 && index2 == size - 1) //如果index1和index2的值分别为0和size - 1，说明字符串前后没有空格或控制字符，直接返回
        return;
    else
        str = str.substr(index1, index2 - index1 + 1);
}

void CCommon::StringNormalize(std::string& str)
{
    _StringNormalize(str);
}

void CCommon::StringNormalize(std::wstring& str)
{
    _StringNormalize(str);
}

template<class T>
static void _StringSplit(const T& str, wchar_t div_ch, vector<T>& results, bool skip_empty = true, bool trim = true)
{
    results.clear();
    size_t split_index = -1;
    size_t last_split_index = -1;
    while (true)
    {
        split_index = str.find(div_ch, split_index + 1);
        T split_str = str.substr(last_split_index + 1, split_index - last_split_index - 1);
        if (trim)
            _StringNormalize(split_str);
        if (!split_str.empty() || !skip_empty)
            results.push_back(split_str);
        if (split_index == wstring::npos)
            break;
        last_split_index = split_index;
    }
}

void CCommon::StringSplit(const std::string& str, char div_ch, vector<std::string>& results, bool skip_empty, bool trim)
{
    _StringSplit(str, div_ch, results, skip_empty, trim);
}

void CCommon::StringSplit(const std::wstring& str, wchar_t div_ch, vector<std::wstring>& results, bool skip_empty, bool trim)
{
    _StringSplit(str, div_ch, results, skip_empty, trim);
}

template<class T>
static void _StringSplit(const T& str, const T& div_str, vector<T>& results, bool skip_empty = true, bool trim = true)
{
    results.clear();
    size_t split_index = 0 - div_str.size();
    size_t last_split_index = 0 - div_str.size();
    while (true)
    {
        split_index = str.find(div_str, split_index + div_str.size());
        T split_str = str.substr(last_split_index + div_str.size(), split_index - last_split_index - div_str.size());
        if (trim)
            _StringNormalize(split_str);
        if (!split_str.empty() || !skip_empty)
            results.push_back(split_str);
        if (split_index == wstring::npos)
            break;
        last_split_index = split_index;
    }
}

void CCommon::StringSplit(const std::string& str, const std::string& div_str, vector<std::string>& results, bool skip_empty, bool trim)
{
    _StringSplit(str, div_str, results, skip_empty, trim);
}

void CCommon::StringSplit(const std::wstring& str, const std::wstring& div_str, vector<std::wstring>& results, bool skip_empty, bool trim)
{
    _StringSplit(str, div_str, results, skip_empty, trim);
}

CString CCommon::GetTextResource(UINT id, int code_type)
{
    CString res_str;
    HRSRC hRes = FindResource(NULL, MAKEINTRESOURCE(id), _T("TEXT"));
    if (hRes != NULL)
    {
        DWORD resSize = SizeofResource(NULL, hRes);  // 获取资源的大小
        HGLOBAL hglobal = LoadResource(NULL, hRes);
        if (hglobal != NULL)
        {
            LPVOID pResourceData = LockResource(hglobal);  // 获取资源数据的指针
            if (code_type == 2)
            {
                // 资源是宽字符字符串
                res_str = CString((const wchar_t*)pResourceData, resSize / sizeof(wchar_t));
            }
            else
            {
                // 资源是窄字符字符串
                std::string strData((const char*)pResourceData, resSize);
                res_str = CCommon::StrToUnicode(strData.c_str(), (code_type != 0)).c_str();
            }
        }
    }
    return res_str;
}

