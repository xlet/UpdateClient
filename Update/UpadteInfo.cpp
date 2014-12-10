
#include "UpdateInfo.h"

Change_log::Change_log()
{

}

Change_log::~Change_log()
{

}
Version::Version()
{

}

Version::~Version()
{
	for(vector<Change_log*>::iterator it = m_vcChangelogs.begin(); it != m_vcChangelogs.end(); ++it)
	{
		Change_log* p = *it;
		SAFE_DELETE(p);
	}
}

UpdateInfo::UpdateInfo()
{

}

UpdateInfo::~UpdateInfo()
{
	for (vector<Version*>::iterator it = m_vcVersion.begin(); it != m_vcVersion.end(); ++it)
	{
		Version* p  = *it;
		SAFE_DELETE(p);
	}
	for (vector<BackUpInfo*>::iterator it = m_vcBackUpInfo.begin(); it != m_vcBackUpInfo.end(); ++it)
	{
		BackUpInfo* p = *it;
		SAFE_DELETE(p);
	}
}

//////////////////////////////////////////////////////////////////////////
UpdateError::UpdateError(/*UpdateInfo* p,*/ tString& tstrHost, tString& tstrPostData, int port/*=80*/, tString tstrRequest/*=_T("/api/v1/feedback")*/)
{
	m_hOpen = NULL;
	m_hConnect = NULL;
	m_hRequest = NULL;
	//m_pUpdateInfo = p;
	m_tstrHost = tstrHost;
	m_tStrPostData = tstrPostData;
	m_tstrRequest = tstrRequest;
	m_nPort = port;
}

UpdateError::UpdateError()
{

}
UpdateError::~UpdateError()
{

}

BOOL UpdateError::SendErrorMsg(LPCTSTR lptError)
{
	if (m_hOpen == NULL)
	{
		m_hOpen = InternetOpen(_T("UpdateError"), INTERNET_OPEN_TYPE_DIRECT, NULL, INTERNET_INVALID_PORT_NUMBER, 0);
		if (m_hOpen== NULL)
		   return FALSE;
	}
	if (m_hConnect == NULL)
	{
		m_hConnect = InternetConnect(m_hOpen, m_tstrHost.c_str(), m_nPort, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 1);
		if (m_hConnect == NULL)
		{
			InternetCloseHandle(m_hOpen);
			return FALSE;
		}
	}

	if (m_hRequest == NULL)
	{
		LPCTSTR lpszAccept[] =
		{
			// 响应头
			_T("*/*"), //接收任何东西
			NULL
		};
		
		m_hRequest = HttpOpenRequest(m_hConnect, _T("POST"), m_tstrRequest.c_str(),HTTP_VERSION, NULL, (LPCTSTR*)lpszAccept,INTERNET_FLAG_DONT_CACHE, 1 );
		if (m_hRequest == NULL)
		{
			InternetCloseHandle(m_hOpen);
			InternetCloseHandle(m_hConnect);
			return FALSE;
		}
	}

	TCHAR szHeader[] =
	{
		// 如果提交的是表单,那么这个 MIME 一定要带!
		_T("Content-Type: application/x-www-form-urlencoded\r\n")
	};

	m_tStrPostData += _T("&remark=");
	m_tStrPostData += lptError;

	char* utf8PostData = NULL;
	UnicodeToUtf8(&utf8PostData, m_tStrPostData.c_str());
	BOOL bRequest = HttpSendRequest(m_hRequest, szHeader, _tcslen(szHeader), utf8PostData, strlen(utf8PostData));
	SAFE_ARRYDELETE (utf8PostData);
#if _DEBUG
	
	char szBuffer[1024];
	memset(szBuffer, 0x0, 1024);
	DWORD dwByteRead = 0;
	// 防止乱码的方法就是建立完变量立即清空
	ZeroMemory(szBuffer, sizeof(szBuffer));
	// 循环读取缓冲区内容直到结束
	TCHAR* tszXml = NULL;
	if (InternetReadFile(m_hRequest, szBuffer, 1034, &dwByteRead) && dwByteRead > 0)

		Utf8ToUnicode(&tszXml, szBuffer);

	// 加入结束标记
	// 应该用变长字符串的 比如 AnsiString

	// 清空缓冲区以备下一次读取
	//ZeroMemory(szBuffer, sizeof(szBuffer));

	// 清理现场
#endif
	InternetCloseHandle(m_hOpen);
	InternetCloseHandle(m_hConnect);
	InternetCloseHandle(m_hRequest);
	return bRequest;
}