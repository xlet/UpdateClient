#ifndef _THREAD_H
#define _THREAD_H
#include "Global.h"
#include <process.h>
#include  <string>
using namespace std;
#include "unzip.h"
#include "UpdateInfo.h"
#include "..\\HtttpDownload\\Download.h"
#ifdef _DEBUG
#pragma  comment(lib, "HtttpDownload_d.lib")
#else
#pragma  comment(lib, "HtttpDownload.lib")
#endif

/*
	由于diulib中使用了xunzip解压压缩文件， 会导致，此处使用了一个Unzip的命名空间
*/
using namespace  Unzip;

#define ZIP_BUF  16384   //解压zip每次解压的大小

#define WM_BEGINUPDATE    WM_USER + 1

typedef void  (*download_callback) (unsigned long , unsigned long);


//更新包下载线程参数
struct download_para
{
	HWND   hWnd;
	//std::string url ;
	UpdateInfo*  pUpdateInfo;
	download_callback pcalbk;
	UpdateError* pUpdateError;
};

//解压更新包线程参数
struct unzipthread_para
{
	HWND    hWnd;
	TCHAR   tszDir[MAX_FILENAME_SIZE+1];
	HANDLE hThread;
	//TCHAR	tszZipName[MAX_FILENAME_SIZE+1];
	UpdateInfo*  pUpdateInfo;
	UpdateError* pUpdateError;
};

//http请求线程参数
struct httprequest_para
{
	tString  serverName;
	tString  filePath;
	string   postdata;// 注意utf8编码
	int      port;	  //加入带url中带端口的话，没带端口为默认80
	//tString  tstrReponse;   //Post 或者get的返回值
	UpdateInfo*  pUpdateInfo;
	UpdateError* pUpdateError;
};

 struct imagesdownload_para
 {
	 HWND        hWnd;
	HANDLE       hHttpRequestThread;
 	UpdateInfo*  pUpdateInfo;
	UpdateError* pUpdateError;
 };

enum E_ZipContent
{
	E_Dir = 0,
	E_File
};

E_ZipContent CheckZipItem(LPCTSTR lptName);

//回调函数， 下载进度
void  DownloadProgress(unsigned long total, unsigned long part);

//解析post返回的Xml
BOOL ParseVersionXml(UpdateInfo* pUpdateInfo, const tString& tstrXml);

//文件复制函数
BOOL BackUpFile(LPCTSTR lptExistingFileName, UpdateInfo* pUpdateInfo, LPCTSTR lptBackUpDir = NULL);

//安装出错还原到本次更新之前的版本
BOOL RestoreToOldVersion();

//更新完成或者失败， 清理程序创建的文件夹以及下载的一些文件
BOOL Clear();

//线程函数
//下载更新包之前下载与更新信息相关的一些图片
unsigned _stdcall DownloadUpdateImages(void* para);

//线程函数
//下载补丁包可以， 在callbk 回调函数中监控当前下载的进度
unsigned _stdcall DownLoadUpdatePackage(void* para);

//线程函数
//下载的安装包 ， 进行解压
//直接解压到当前目录覆盖即可
unsigned _stdcall UnzipUpdatePackage(void* para);

//解压时候可以显示进度
unsigned _stdcall UnzipUpdatePackageWithProgress(void* para);

//压缩文件中存在多级目录时或者空目录时， 解压时若本地无该目录需要获取压缩文件中目录后再创建
void GetDirFromZipFileItem(LPCTSTR lptPath, LPTSTR lptDir, int nSize);
//BOOL CheckUpdate(const char* url);

//
unsigned _stdcall HttpRequest(void* para);

#endif