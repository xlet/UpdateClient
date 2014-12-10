#ifndef _GLOBAL_H
#define _GLOBAL_H

#include <windows.h>
#include <tchar.h>
#include <string>
#include  <WinInet.h>
#pragma comment(lib, "wininet.lib")
//内存释放宏
using namespace std;
#if defined(UNICODE)
typedef std::basic_string<wchar_t,std::char_traits<wchar_t>,std::allocator<wchar_t> > tString;
#else
typedef std::basic_string<char,std::char_traits<char>,std::allocator<char> >		  tString;
#endif
#define SAFE_ARRYDELETE(x) if(NULL!=x){delete[] x;x = NULL;}

#define SAFE_DELETE(x) if(NULL!=x){delete x; x =NULL;}

#define  WM_ONDOWNLOADPROGRESS  WM_USER + 100

#define  WM_ONINSTALLSTATE     WM_USER + 101

//#define  WM_ONINSTALL           WM_USER + 102 

//删除字符串前后的空格
char * Trim( char * str );

//获取当前目录
void  GetCurDir(LPTSTR lpStr, int nSize);

//从Url中下载的文件名
BOOL GetDownLoadZipFileName(const char* url, TCHAR* tstrFname);

//多字节转化为宽字节
void C2W(WCHAR** dest, const char* src);

//宽字节转化为多字节
void W2C(char** dest, const WCHAR *src);

void W2C(string& dest,const WCHAR *src);

//char 字符串转化为tchar字符串
void C2T(TCHAR** dest, const char* src);

void T2C(char** dest, const TCHAR* src);

BOOL T2C(char*dest, int nSize, const TCHAR* src );

//查找文件或者目录是否存在
BOOL FindFile(LPCTSTR lptPath) ; 

BOOL CreateDir(LPCTSTR lptPath); //创建目录

void tStringToString(std::string& strDest, const tString& tstrSrc);

void tStringToString(std::string& strDest, LPCTSTR lpctSrc);

void StringTotString(tString& tstrDest, const std::string& strSrc);

void StringTotString(tString& tstrDest, LPCSTR lpcSrc);

void UnicodeToTString(tString& tstrDest, const WCHAR* lpwSrc);

void Utf8ToUnicode(WCHAR** dest,const char* src);

void Utf8TotString(tString &strDest, const char* src);

void UnicodeToUtf8(char** dest , const WCHAR* src);

void AnsiToUtf8(char** dest, const char* src);

// void TcharToUtf8(char** dest, const TCHAR* src)
// {
// #ifdef _UNICODE
// 	UnicodeToUtf8(dest, src);
// #else
// 	AnsiToUtf8(dest, src);
// #endif
// }

void tStringToUtf8(string &tstrDest,  LPCTSTR src);

tString GetBetweenString(const TCHAR * pStr, const TCHAR * pStart, const TCHAR * pEnd);
//字符串中字符替换
void _tstrreplace(LPTSTR lptStr, TCHAR tSrc, TCHAR tDest);

#ifdef _DEBUG
void OutputLastError(DWORD dwError);
#endif

//从url中得到http服务器上文件名称
BOOL GetFileNameFromURL(const char *url, tString& tstrName);

//删除整个文件夹，包括其中的子文件、文件夹等
BOOL DeleteFolder(LPCTSTR lpstrFolder);

BOOL GetUrlFromCommandline(tString& tstrPostUrl);

#endif