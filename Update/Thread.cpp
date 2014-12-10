#include "Thread.h"
#include  "UpdateInfo.h"
#include "XmlDocument.h"

///////////////////////////////////////////////////////////////////////////
//
//  更新日志（图片）目录在
//   update.exe所在目录\update\change_log
//
//
//  备份的文件在：
//      update.exe所在目录\update\backup  
//
///
///	注意的是更新完毕后删除update文件夹及其文件夹下
//	的所有文件
//
//
// 注意更新发生错误的时候发送消息时wParam参数为102 
//
//



HWND  hWndCallBk ;

#define  BUFFER_SZIE    1024
//unzip库中文件路径都是使用'/'
E_ZipContent CheckZipItem(LPCTSTR lptName)
{
	LPCTSTR pp = _tcsrchr(lptName, _T('\\'));
	if (_tcsrchr(lptName, _T('\\'))[1] == _T('\0'))
	{
		return E_Dir;
	}
	return E_File;
}

//回调函数， 下载进度
void  DownloadProgress(unsigned long total, unsigned long part)
{
	int val = (int)((double)part /total * 100);
	PostMessage(hWndCallBk, WM_ONDOWNLOADPROGRESS, (WPARAM)val, 0 );
	//printf("progress: %i%%\n", val);
}

//解析Post返回的Xml字符串
BOOL ParseVersionXml(UpdateInfo* pUpdateInfo, const tString& tstrXml)
{
	::CoInitialize(NULL);
	//MessageBox(NULL, tstrXml.c_str(), _T("xml解析"), MB_OK);
	CXmlDocument XmlDoc;
	CXmlNodeList  VerNodeList, ChangelogNodeList; 
	CXmlNode RootNode,  xmlNode, xmlSubNode;
	BOOL bRet = XmlDoc.LoadXml(tstrXml.c_str());
	if (!bRet)
		//MessageBox(NULL, _T("加载失败"), _T("xml解析"), MB_OK);
		return  bRet;
	bRet = XmlDoc.SelectSingleNode(_T("response"), RootNode);
	if (!bRet)
		return bRet;

	bRet = RootNode.SelectSingleNode(_T("window"), xmlNode);
	if (bRet)
	{
		pUpdateInfo->m_size.cy = xmlNode.GetAttributeInt(_T("height"));
		pUpdateInfo->m_size.cx = xmlNode.GetAttributeInt(_T("width"));
	}
	else
		return  FALSE;
	bRet = RootNode.SelectSingleNode(_T("product"), xmlNode);
	if(bRet)
	{
		UnicodeToTString(pUpdateInfo->m_pProductName, xmlNode.GetAttribute(_T("name")).c_str());
	}
	else
		return FALSE;

	bRet = RootNode.SelectNodes(_T("version"),  VerNodeList);
	if (!bRet)
		return bRet;


		
	int x = VerNodeList.GetLength();
	for (int i = 0 ; i < VerNodeList.GetLength(); ++i)
	{
		if (VerNodeList.GetItem(i, xmlNode))
		{
			Version* pVer = new Version;
			W2C(pVer->m_strUrl,  (xmlNode.GetAttribute(_T("url"))).c_str());
			//pVer->m_strUrl = xmlNode.GetAttribute(_T("url"));
			UnicodeToTString(pVer->m_tstrVersion, (xmlNode.GetAttribute(_T("version"))).c_str());
			//pVer->m_tstrVersion = xmlNode.GetAttribute(_T("version"));
			UnicodeToTString(pVer->m_tstrTime, (xmlNode.GetAttribute(_T("create_at"))).c_str());
			//pVer->m_tstrTime = xmlNode.GetAttribute(_T("create_at"));
			bRet = xmlNode.SelectNodes(_T("change_log"), ChangelogNodeList);
			if (!bRet)
				return bRet;
			for (int j = 0; j < ChangelogNodeList.GetLength(); ++j)
			{
				if (ChangelogNodeList.GetItem(j, xmlSubNode))
				{
					Change_log* pChangeLog = new Change_log;
					UnicodeToTString(pChangeLog->m_tstrTitle, (xmlSubNode.GetAttribute(_T("title"))).c_str());
					//pChangeLog->m_tstrTitle = xmlSubNode.GetAttribute(_T("title"));
					UnicodeToTString(pChangeLog->m_tstrdescribe, (xmlSubNode.GetAttribute(_T("description")).c_str()));
					//pChangeLog->m_tstrdescribe = xmlSubNode.GetAttribute(_T("description"));
					W2C(pChangeLog->m_strImageUrl, (xmlSubNode.GetAttribute(_T("image")).c_str()));
					//pChangeLog->m_strImageUrl = xmlSubNode.GetAttribute(_T("image"));
					pVer->m_vcChangelogs.push_back(pChangeLog);
				}
			}
			pUpdateInfo->m_vcVersion.push_back(pVer);
		}
	}
::CoUninitialize();
	return bRet;
}


//线程函数
//下载补丁包可以， 在callbk 回调函数中监控当前下载的进度
unsigned _stdcall DownLoadUpdatePackage(void* para)
{
	TCHAR tszFileDir[MAX_FILENAME_SIZE+1] = {0x0};
	GetCurDir(tszFileDir, MAX_FILENAME_SIZE);
	_tcscat(tszFileDir, _T("\\update"));
	char* pszFileDir = NULL;
	T2C(&pszFileDir, tszFileDir);
	download_para* p = (download_para*)para;
	BOOL bRet = FALSE;
	tString tStrZipName;

	//下载
	for (vector<Version*>::iterator it = p->pUpdateInfo->m_vcVersion.begin(); it != p->pUpdateInfo->m_vcVersion.end(); ++it)
	{
		bRet = http_downloadfile((*it)->m_strUrl.c_str(), pszFileDir, NULL,false, p->pcalbk)?TRUE:FALSE;
		if (bRet)
		{
			TCHAR tszPackagePath[MAX_FILENAME_SIZE + 1] = {0x0};

			GetFileNameFromURL((*it)->m_strUrl.c_str(), tStrZipName);
			_stprintf_s(tszPackagePath,  _T("%s\\%s"), tszFileDir,tStrZipName.c_str());
			p->pUpdateInfo->m_vcPackageName.push_back(tszPackagePath);
		}
		else
		{
			//更新安装包下载失败
			tString tstrError = _T("更新安装包下载失败,URL:");
			tString tstrUrl;
			StringTotString(tstrUrl,(*it)->m_strUrl.c_str() );
			tstrError += tstrUrl;
			p->pUpdateError->SendErrorMsg(tstrError.c_str());
			PostMessage(hWndCallBk, WM_ONDOWNLOADPROGRESS, 102, 0 );
			return 1;
		}
	}
	
	SAFE_ARRYDELETE(pszFileDir);
	return bRet?0:1;
}

//更新之前将需要更新的文件进行备份
BOOL BackUpFile(LPCTSTR lptExistingFilePath, UpdateInfo* pUpdateInfo,  LPCTSTR lptBackUpDir)
{
	LPCTSTR lptFileName = _tcsrchr(lptExistingFilePath, _T('\\'));
	TCHAR tszBackUpPath[MAX_FILENAME_SIZE + 1] = {0x0};
	GetCurDir(tszBackUpPath, MAX_FILENAME_SIZE);
	_tcscat(tszBackUpPath, _T("\\update\\backup"));
	//判断目录存在不
	if (!FindFile(tszBackUpPath))
	{
		CreateDir(tszBackUpPath);
	}
	
	_tcscat(tszBackUpPath, lptFileName);
#if 0 
	//此种方法也可复制文件
	HANDLE hIn, hOut;
	DWORD nIn, nOut;
	hIn = CreateFile(lptExistingFilePath, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);

	if (hIn == INVALID_HANDLE_VALUE)
	{
#ifdef _DEBUG
		OutputLastError(GetLastError());
#endif
		return  FALSE;
	}
	hOut = CreateFile(tszNewFileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
	if (hOut == INVALID_HANDLE_VALUE)
	{
#ifdef _DEBUG
		OutputLastError(GetLastError());
#endif
		return  FALSE;
	}
	CHAR buffer[2048];
	while (ReadFile(hIn, buffer, 2048,&nIn, NULL) && nIn > 0)
	{
		WriteFile(hOut, buffer, nIn, &nOut, NULL);
		if (nIn != nOut)
		{
#ifdef _DEBUG
			OutputLastError(GetLastError());
#endif
			DeleteFile(tszNewFileName);
			return  FALSE;
		}
	}
	return TRUE;
#endif
	
	if  (CopyFile(lptExistingFilePath, tszBackUpPath, FALSE))
	{
		BackUpInfo* pBackUpInfo = new BackUpInfo;
		pBackUpInfo->tstrBackUpPath = tszBackUpPath;
		pBackUpInfo->tstrOldPath = lptExistingFilePath;
		pUpdateInfo->m_vcBackUpInfo.push_back(pBackUpInfo);
		return TRUE;
	}
	return FALSE;
}

//下面的函数， 当更新失败时候还原到旧的版本
BOOL RestoreToOldVersion(UpdateInfo* pUpdateInfo)
{
	for (vector<BackUpInfo*>::iterator it = pUpdateInfo->m_vcBackUpInfo.begin(); it != pUpdateInfo->m_vcBackUpInfo.end(); ++it)
	{
		if (!CopyFile((*it)->tstrBackUpPath.c_str(), (*it)->tstrOldPath.c_str(), FALSE))
		{
			//复制失败，这个时候客户端可能会被损坏
			return FALSE;
		}
	}
	return TRUE;
}

//更新完成或者失败后删除
BOOL Clear()
{
	TCHAR tszUpdateFolderPath[MAX_FILENAME_SIZE + 1] = {0x0};
	GetCurDir(tszUpdateFolderPath, MAX_FILENAME_SIZE);
	_tcscat(tszUpdateFolderPath, _T("\\update"));
	return DeleteFolder(tszUpdateFolderPath);
}

//对应chang_log中图片的下载
unsigned _stdcall DownloadUpdateImages(void* para)
{
	//WaitForSingleObject(p->hHttpRequestThread, INFINITE);
	imagesdownload_para* p = (imagesdownload_para*)(para);
	TCHAR  tszFileDir[MAX_FILENAME_SIZE + 1] = {0x0};
	GetCurDir(tszFileDir, MAX_FILENAME_SIZE );
	//当前目录的update目录下面 
	_tcscat(tszFileDir, _T("\\update\\change_log"));
	if (!FindFile(tszFileDir))
	{
		if (!CreateDir(tszFileDir))
		{
			tString tstrError = _T("创建目录失败， 名称：");
			tstrError += tszFileDir;
			p->pUpdateError->SendErrorMsg(tstrError.c_str());
			return 1;
		}
	}

	char* pszFileDir = NULL;
	T2C(&pszFileDir, tszFileDir);
	
	
	for (vector<Version*>::iterator itVer = p->pUpdateInfo->m_vcVersion.begin(); itVer != p->pUpdateInfo->m_vcVersion.end(); ++itVer)
	{
		for (vector<Change_log*>::iterator itChangelog = (*itVer)->m_vcChangelogs.begin(); itChangelog != (*itVer)->m_vcChangelogs.end(); ++itChangelog)
		{

			BOOL bRet = http_downloadfile((*itChangelog)->m_strImageUrl.c_str(), pszFileDir, NULL,false, NULL)?TRUE:FALSE;

			//存储下载的图片名称
			if (bRet)
			{
				TCHAR tszImagePath[MAX_FILENAME_SIZE + 1] = {0x0};
				tString tstrName;
				GetFileNameFromURL((*itChangelog)->m_strImageUrl.c_str(), tstrName);
				_stprintf_s(tszImagePath, _T("%s\\%s"), tszFileDir,tstrName.c_str());
				
				p->pUpdateInfo->m_vcImageName.push_back(tszImagePath);
			}
			else
			{
				//更新日志图片下载失败
				tString tstrError = _T("更新日志图片下载失败，URL:");
				tString tstrUrl;
				StringTotString(tstrUrl, (*itChangelog)->m_strImageUrl.c_str());
				tstrError += tstrUrl;
				p->pUpdateError->SendErrorMsg(tstrUrl.c_str());
			}
		}
	}

	PostMessage(p->hWnd, WM_BEGINUPDATE, 0, 0);
	SAFE_ARRYDELETE(pszFileDir);
	return 0;
}

//线程函数
//下载的安装包 ， 进行解压
//此函数，此版本中没有用到
unsigned _stdcall UnzipUpdatePackage(void* para)
{
	unzipthread_para* p = (unzipthread_para*)para;

	WaitForSingleObject(p->hThread, INFINITE);

	for (vector<tString>::iterator it = p->pUpdateInfo->m_vcPackageName.begin(); it != p->pUpdateInfo->m_vcPackageName.end(); ++it)
	{
		HZIP hz = OpenZip(it->c_str(), NULL);
		if (hz == NULL)
		{
			OutputDebugString(_T("Open zip files failed \n"));
			//p->pUpdateInfo->
			return 0;
		}
		ZIPENTRY ze ;
		GetZipItem(hz,  -1, &ze);
		int nItems = ze.index;
		//SetUnzipBaseDir()   //默认是当前目录
		for (int i= 0; i < nItems; ++i)
		{
			ZIPENTRY zeItem;
			GetZipItem(hz, i, &zeItem);
			UnzipItem(hz, i, zeItem.name );
		}
		CloseZip(hz);
	}
	
	return 0;
}

//解压时候可以显示进度
//
//
//直接解压到当前目录覆盖，
//需要注意的是在覆盖当前文件之前需要备份旧的文件，
//这样防止在安装失败后无法还原
unsigned _stdcall UnzipUpdatePackageWithProgress(void* para)
{
	unzipthread_para* p = (unzipthread_para*)para;
	//等待下载线程的结束
	WaitForSingleObject(p->hThread, INFINITE);

	DWORD httpRequestThreadExitCode = -1;
	GetExitCodeThread(p->hThread, &httpRequestThreadExitCode);
	if (httpRequestThreadExitCode != 0)
	{
		return 0;
	}
	for (vector<tString>::iterator it = p->pUpdateInfo->m_vcPackageName.begin(); it != p->pUpdateInfo->m_vcPackageName.end(); ++it)
	{
		HZIP hz = OpenZip(it->c_str(), 0);
		if (hz == NULL)
		{
			//打开压缩文件失败
			//Clear();
			p->pUpdateError->SendErrorMsg(_T("打开压缩文件失败"));
			RestoreToOldVersion(p->pUpdateInfo);
			PostMessage(p->hWnd, WM_ONINSTALLSTATE, WPARAM(102), 0);
			OutputDebugString(_T("OpenZip Failed \n"));
			return  1;
		}
		ZIPENTRY ze;
		GetZipItem(hz, -1, &ze);

		int nItemNum = ze.index;
		DWORD tot = 0;

		//计算压缩包总大小
		for (int i = 0; i < nItemNum; ++i)
		{
			GetZipItem(hz, i, &ze);
			tot += ze.unc_size;
		}

		DWORD countall = 0;
		//TCHAR szFileDir[MAX_FILENAME_SIZE +1] = {0x0};
		//GetCurDir(szFileDir, MAX_FILENAME_SIZE + 1);

		for (int i= 0; i < nItemNum; ++i)
		{
			GetZipItem(hz, i, &ze);

			//解压到的文件路径
			TCHAR tszFileName[MAX_FILENAME_SIZE + 1] = {0x0};
			_stprintf_s(tszFileName, _T("%s\\%s"), p->tszDir, ze.name);
			_tstrreplace(tszFileName, _T('/'), _T('\\'));

			//注意的是在覆盖原有旧的文件的时候先将旧的文件备份
			if(!BackUpFile(tszFileName, p->pUpdateInfo))
			{
				tString tstrError = _T("备份文件失败，文件名:");
				tstrError += tszFileName;
				p->pUpdateError->SendErrorMsg(tstrError.c_str());
			}

			if ( CheckZipItem(tszFileName) == E_Dir)
			{
				//压缩文件中可能存在空的文件夹
				//系统找不到指定的路径，文件所属文件夹不存在
				TCHAR tszItemDir[MAX_PATH+1] = {0x0};
				GetDirFromZipFileItem(tszFileName, tszItemDir, MAX_PATH + 1);

				//创建文件目录（zip中item可能为文件和文件夹）
				CreateDir(tszItemDir);
				continue;;
			}
			else
			{
				//创建一个解压文件， 存储压缩文件内容
				HANDLE hf = CreateFile(tszFileName, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);


				if (INVALID_HANDLE_VALUE == hf)
				{
					OutputDebugString(_T("CreateFile Failed \n"));
					DWORD dwError= GetLastError();
#ifdef _DEBUG
					//OutputLastError(dwError);
#endif
					if (dwError == 3)
					{
						//系统找不到指定的路径，文件所属文件夹不存在
						TCHAR tszItemDir[MAX_PATH+1] = {0x0};
						GetDirFromZipFileItem(tszFileName, tszItemDir, MAX_PATH + 1);

						//创建文件目录（zip中item可能为文件和文件夹）
						if (CreateDir(tszItemDir))
						{
							hf = CreateFile(tszFileName, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
						}
						else
						{
#ifdef _DEBUG
							OutputLastError(GetLastError());
#endif
						}
					}	
				}

				//如果创建文件还是无效的话直接进行下一次解压
				if (hf == INVALID_HANDLE_VALUE)
				{
#ifdef _DEBUG
					OutputLastError(GetLastError());
#endif
					tString tstrError = _T("解压时创建文件失败,文件名：");
					tstrError += tszFileName;
					p->pUpdateError->SendErrorMsg(tstrError.c_str());
					PostMessage(p->hWnd, WM_ONINSTALLSTATE, WPARAM(102), 0);
					CloseZip(hz);
					return 1;
					//continue;
				}

				char buf[ZIP_BUF];
				DWORD  countfile = 0;

				for (ZRESULT zr = ZR_MORE; zr== ZR_MORE;)
				{
					zr = UnzipItem(hz, i, buf, ZIP_BUF);
					unsigned long bufsize = ZIP_BUF;
					if (zr == ZR_OK)
					{
						bufsize = ze.unc_size - countfile;
					}
					else if (zr != ZR_MORE)
					{
						bufsize=0;
					}
					DWORD writ;
					WriteFile(hf, buf,bufsize, &writ, 0);

					countfile += bufsize;
					countall += bufsize;

					//计算解压进度,是相对于每个压缩包的进度
					int nProgress = (int)(100.0*((double)countall)/((double)tot));
					PostMessage(p->hWnd, WM_ONINSTALLSTATE, (WPARAM)nProgress, 0);
					//向界面发送消息 显示进度
				}

				CloseHandle(hf);
			}
		}
		CloseZip(hz);
	}


	//解压完成， 解压完成后删除更新的临时文件夹、
	//Clear();
	PostMessage(p->hWnd, WM_ONINSTALLSTATE, WPARAM(101), 0);
	return 0;
}

// BOOL CheckUpdate(const char* url)
// {
// 	return TRUE;
// }

void GetDirFromZipFileItem(LPCTSTR lptPath, LPTSTR lptDir, int nSize)
{
// 	_tcsrchr(lptPath, _T('/'));
// 	if (lptName+1 == _T('\0'))
// 	{
// 		return E_Dir;
// 	}
	_tcscpy_s(lptDir,_tcslen(lptPath)+1, lptPath);

	_tcsrchr(lptDir, _T('\\'))[0] = _T('\0');

}

unsigned _stdcall HttpRequest(void* para)
{
	httprequest_para* p = (httprequest_para*)(para);
	//接收类型
	LPCTSTR lpszAccept[] =
	{
			// 响应头
		_T("*/*"), //接收任何东西
		NULL       //注意以NULL结尾
	};
	TCHAR szHeader[] =
	{
		// 如果提交的是表单,那么这个 MIME 一定要带!
		_T("Content-Type: application/x-www-form-urlencoded\r\n")
	};

	HINTERNET  hInet = InternetOpen(_T("UpdateClient"), INTERNET_OPEN_TYPE_DIRECT, NULL, INTERNET_INVALID_PORT_NUMBER, 0);
	if (hInet == NULL)
	{
#if _DEBUG
		OutputLastError(GetLastError());
#endif 
		p->pUpdateError->SendErrorMsg(_T("HttpRequest,InternetOpen Method Failed "));
		return 1;
	}
	HINTERNET hConn = InternetConnect(hInet, p->serverName.c_str(), p->port, NULL, NULL, INTERNET_SERVICE_HTTP, 0 ,1 );
	if (hConn == NULL)
	{
#if _DEBUG
		OutputLastError(GetLastError());
#endif 
		p->pUpdateError->SendErrorMsg(_T("HttpRequest, InternetConnect Method Failed"));
		return 1;
	}
	HINTERNET hPost = HttpOpenRequest(hConn, _T("POST"), p->filePath.c_str(), HTTP_VERSION, NULL, (LPCTSTR*)lpszAccept, INTERNET_FLAG_DONT_CACHE, 1);
	if (hPost == NULL)
	{
#if _DEBUG
		OutputLastError(GetLastError());
#endif 
		p->pUpdateError->SendErrorMsg(_T("HttpRequest, HttpOpenRequest Method Failed"));
		return 1;
	}

	if (!HttpSendRequest(hPost, szHeader, _tcslen(szHeader), (LPVOID)(p->postdata.c_str()), p->postdata.length()))
	{
#if _DEBUG
		OutputLastError(GetLastError());
#endif 
		p->pUpdateError->SendErrorMsg(_T("HttpRequest, HttpSendRequest Method Failed"));
		return 1;
	}

	unsigned long lByteRead;

	char szBuffer[BUFFER_SZIE+1] = {0x0};

	std::string strReponse;
	do 
	{
		if (!InternetReadFile(hPost,  szBuffer, BUFFER_SZIE, &lByteRead))
		{
#if _DEBUG
			OutputLastError(GetLastError());
#endif 
			p->pUpdateError->SendErrorMsg(_T("HttpRequest, InternetReadFile Failed"));
			return 1;
		}
		szBuffer[lByteRead] = 0x0;
		strReponse += szBuffer;
	} while (lByteRead > 0);

	tString tstrReponse;
	Utf8TotString(tstrReponse, strReponse.c_str());
//	MessageBox(NULL, tstrReponse.c_str(), _T("Response"), MB_OK);
	if(!ParseVersionXml(p->pUpdateInfo, tstrReponse))
	{
		p->pUpdateError->SendErrorMsg(_T("Parse XMl Failed"));
		return 2;
	}

	Clear();
	// 0 返回表示正常
	// 1 表示post 过程可能出问题
	// 2 表示返回的xml 可能不正确
	return  0;
}