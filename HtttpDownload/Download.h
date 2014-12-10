// Header file for downloader.


#ifndef DOWNLOAD_H
#define DOWNLOAD_H

//#include <iostream>

const int MAX_FILENAME_SIZE = 512;

#ifdef HTTTPDOWNLOAD_EXPORTS

//#define  HTTPDOWNLOAD_API __declspec(dllexport)  

#include <string>
#include <windows.h>
#include <wininet.h>
#include <fstream>

using namespace std;

const int MAX_ERRMSG_SIZE = 80;

const int BUF_SIZE = 10240;             // 10 KB

#pragma comment(lib, "wininet.lib")
// 自定义异常类
class DLExc
{
private:
	char err[MAX_ERRMSG_SIZE];
public:
	DLExc(char *exc)
	{
		if(strlen(exc) < MAX_ERRMSG_SIZE)
			strcpy(err, exc);
	}

	// 返回指向错误信息字符串的指针
	const char *geterr()
	{
		return err;
	}
};


// http协议文件下载类
class Download
{
private:
	static bool ishttp(const char *url);
	static bool httpverOK(HINTERNET hIurl);
	static bool getfname(const char *url, char *fname);
	static unsigned long openfile(const char *filename, bool reload, ofstream &fout);
public:
	static bool download(const char *url, const char*  filedir, const char* filename , bool reload=false, void (*update)(unsigned long, unsigned long)=NULL);
	static bool download(const char *url, char* pbuf, int bufsize);
};
#else 

//#define  HTTPDOWNLOAD_API  __declspec(dllimport) 

#endif


typedef void  (*download_callback) (unsigned long , unsigned long);

#ifdef __cplusplus

extern "C"  {

#endif	

/*
	fileDir 为文件存放的目录， 该参数一般为空， 当filename为空是可以设置fileDir目录名
	将文件以默认名称存在filedir目录下

	filename 为文件的名称， 如果该参数为空， 那么使用文件原有的名称

	reload 是否重新下载 FALSE的话 会在之前的进度上进行下载 

	callbk  函数指针， 设置回调函数可以监控当前的下载进度
*/
bool http_downloadfile(const char* url,const char*  fileDir, const char* filename,  bool reload, download_callback callbk);

/*
	如果下载的是普通的文字文件如txt， 这样的， 只需要
*/
bool http_downloadtobuf(const char* url, char* pbuf, int bufSize);

#ifdef __cplusplus

}
#endif

#endif

