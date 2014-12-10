#include "UpdateWnd.h"
#include "resource.h"
#include <tlhelp32.h> //声明快照函数文件

//http://dlsw.baidu.com/sw-search-sp/soft/da/17519/BaiduYunGuanjia_4.8.3.1409021519.exe

extern HWND hWndCallBk;


CUpdateWnd::CUpdateWnd(void)
{
	m_pDownloadPara = NULL;
	m_pUnzipPara = NULL;
	m_pHttpRequestPara = NULL;
	m_pImagesDownloadPara= NULL;
	m_nChangelogIndex = 0;
	m_nDownloadNum = 0;
	m_pUpdateError = NULL;
}


CUpdateWnd::~CUpdateWnd(void)
{
	SAFE_DELETE(m_pDownloadPara);
	SAFE_DELETE(m_pUnzipPara);
	SAFE_DELETE(m_pHttpRequestPara);
	SAFE_DELETE(m_pUpdateError);
}


LPCTSTR  CUpdateWnd::GetWindowClassName() const
{
	return _T("UpdateDlg");
}

CDuiString CUpdateWnd::GetSkinFile()
{
	return _T("update.xml");
}

CDuiString CUpdateWnd::GetSkinFolder()
{
	return  _T("");
}

UINT CUpdateWnd::GetClassStyle() const 
{
	return UI_CLASSSTYLE_DIALOG;
}

UILIB_RESOURCETYPE CUpdateWnd::GetResourceType() const
{
	return UILIB_ZIPRESOURCE;
}

LPCTSTR CUpdateWnd::GetResourceID() const
{
	return MAKEINTRESOURCE(IDR_ZIPRES);
}

 CControlUI* CUpdateWnd::CreateControl(LPCTSTR pstrClass)
{
	if (_tcsicmp(pstrClass, _T("GifControl")) == 0)
	{
		return new CControlGifUI(); 

	}
	return NULL;
}

void CUpdateWnd::OnFinalMessage(HWND hWnd)
{
	WindowImplBase::OnFinalMessage(hWnd);	
	delete this;
}

//exe中传递的参数, 类似如下字符串, GetCommandLine前面包括当前exe路径
//http://10.0.40.18:8081/api/v1/version.xml?version=0.0.1&product=im
//c++中post的数据必须是UTF-8的编码
void CUpdateWnd::InitUpdateInfo()
{
	m_pHttpRequestPara = new httprequest_para;
	m_pUpdateInfo = new UpdateInfo;
	//指针传到线程参数中， 在Http请求线程中初始化
	m_pHttpRequestPara->pUpdateInfo = m_pUpdateInfo;
	//LPCTSTR lptstrCommand = GetCommandLine(); //包含了exe路径
	
	//去除前面exe路径剩下的url
	tString tstrPostUrl ;//= _tcsrchr(lptstrCommand, _T('\"'))+2;

	if (!GetUrlFromCommandline(tstrPostUrl))
	{
		//if (MessageBox(NULL, _T("打开进程参数出错"), _T("警告"), MB_OK) == IDOK)
		//{
			//exit(0);
			PostQuitMessage(0);
		//}
	}
	
	TCHAR tszFlag[5] = {0x0};

	
	int nFindPos = tstrPostUrl.find(_T(':'));
	if (nFindPos!= tString::npos && nFindPos != 4)
	{
		_tcscpy(tszFlag, _T(":"));
		m_pHttpRequestPara->port = _ttoi(_tcsrchr(tstrPostUrl.c_str(), _T(':')) + 1);
	}
	else
	{
		_tcscpy(tszFlag, _T("/"));
		m_pHttpRequestPara->port = 80;  //默认http端口80
	}
	m_pHttpRequestPara->serverName = GetBetweenString(tstrPostUrl.c_str(), _T("//"), tszFlag);

	tString tstrPostData= _tcsrchr(tstrPostUrl.c_str(), _T('?'))+1;  //POST数据
	tStringToUtf8(m_pHttpRequestPara->postdata, tstrPostData.c_str());
	m_pHttpRequestPara->filePath = GetBetweenString(tstrPostUrl.c_str(), GetBetweenString(tstrPostUrl.c_str(), _T("//"), _T("/")).c_str(), _T("?"));


#if _DEBUG  
	//取出版本号和产品版本号
	//这段代码可去掉
	tString tstrProductName, tstrVersion;  
	tstrVersion = GetBetweenString(tstrPostData.c_str(), _T("version="), _T("&"));
	if (tstrVersion == _T(""))
	{
		tstrVersion = _tcsrchr(tstrPostData.c_str(), _T('=')) + 1;
		tstrProductName = GetBetweenString(tstrPostData.c_str(), _T("product="), _T("&"));
	}
	else
		tstrProductName =  _tcsrchr(tstrPostData.c_str(), _T('=')) + 1;
#endif
	//tstrVersion = GetBetweenString(tstrPostData.c_str(), _T("product="))
	m_pUpdateError = new UpdateError(m_pHttpRequestPara->serverName,tstrPostData , m_pHttpRequestPara->port);
	m_pHttpRequestPara->pUpdateError = m_pUpdateError;
}

//在下载更新包之前需要发送post请求获取更新的xml并解析，每个版本变更日志对应的图片也需要下载
void CUpdateWnd::PreUpdate()
{
	//初始化更新信息
	InitUpdateInfo();
	unsigned httpRequestThreadAddr;
	unsigned ImagesDownloadThreadAddr;

	//创建线程并立即运行  像服务端post请求返回xml
	m_hHttpRequestThread = (HANDLE)_beginthreadex(NULL, 0, HttpRequest, (void*)m_pHttpRequestPara,0,  &httpRequestThreadAddr);
	m_pImagesDownloadPara = new imagesdownload_para;
	m_pImagesDownloadPara->hWnd = m_hWnd;
	m_pImagesDownloadPara->pUpdateInfo = m_pUpdateInfo;
	m_pImagesDownloadPara->pUpdateError = m_pUpdateError;
	m_pImagesDownloadPara->hHttpRequestThread = m_hHttpRequestThread;
	m_hImagesDownloadThread = (HANDLE)_beginthreadex(NULL, 0, DownloadUpdateImages, (void*)m_pImagesDownloadPara, CREATE_SUSPENDED, &ImagesDownloadThreadAddr );
	//等待post请求结果返回， 此处可以添加一个等待界面
	WaitForSingleObject(m_hHttpRequestThread, INFINITE);

	DWORD httpRequestThreadExitCode = -1;
	GetExitCodeThread(m_hHttpRequestThread, &httpRequestThreadExitCode);
	if (httpRequestThreadExitCode != 0)
	{
		if (MessageBox(NULL, _T("无法连接到更新服务器"), _T("警告"), MB_OK) == IDOK)
		{
			Exit();
			//PostQuitMessage(0);
		}
		
	}

	//由于窗口大小是由post返回的xml中窗口大小属性决定的， 所以上面必须等线程结束
	this->ResizeClient(m_pUpdateInfo->m_size.cx, m_pUpdateInfo->m_size.cy);
	ResumeThread(m_hImagesDownloadThread);

}


void CUpdateWnd::InitWindow()
{

	m_pLoadingView = static_cast<CVerticalLayoutUI*>(m_PaintManager.FindControl(_T("loadingview")));
	m_pGifControl = static_cast<CControlGifUI*>(m_PaintManager.FindControl(_T("loadinggif")));
	m_pBkgroudContainer = static_cast<CVerticalLayoutUI*>(m_PaintManager.FindControl(_T("bkgroud")));
	m_pUpdateView = static_cast<CVerticalLayoutUI*>(m_PaintManager.FindControl(_T("updateview")));
	m_pDownloadView = static_cast<CVerticalLayoutUI*>(m_PaintManager.FindControl(_T("downloadview")));
	m_pInstallView = static_cast<CVerticalLayoutUI*>(m_PaintManager.FindControl(_T("installview")));
	m_pCompleteView = static_cast<CVerticalLayoutUI*>(m_PaintManager.FindControl(_T("completeview")));
	m_pTitle = static_cast<CLabelUI*>(m_PaintManager.FindControl(_T("title")));
	m_pErrorView = static_cast<CVerticalLayoutUI*>(m_PaintManager.FindControl(_T("errorview")));
	m_pDownloadProgress = static_cast<CProgressUI*>(m_PaintManager.FindControl(_T("downloadprogress")));
	m_pUnzipProgress = static_cast<CProgressUI*>(m_PaintManager.FindControl(_T("installprogress")));
	m_pCloseBtn = static_cast<CButtonUI*>(m_PaintManager.FindControl(_T("closebtn")));
	m_pDownloadTip = static_cast<CLabelUI*>(m_PaintManager.FindControl(_T("downloadtip")));
	m_pArrowBtnView = static_cast<CVerticalLayoutUI*>(m_PaintManager.FindControl(_T("arrowbtncontainer")));
	PreUpdate();
}

LRESULT CUpdateWnd::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	
	if (uMsg == WM_ONDOWNLOADPROGRESS)
	{
		return OnDownloadProgress(uMsg, wParam, lParam);
	}
	else if (uMsg == WM_ONINSTALLSTATE)
	{
		return OnInstallState(uMsg, wParam, lParam);
	}
	else if (uMsg == WM_BEGINUPDATE)
	{
		return OnBeginUpdate(uMsg, wParam, lParam);
	}
 	else if (uMsg == WM_TIMER)
 	{
 		LRESULT lRes =  OnTimer(uMsg, wParam, lParam/*, bHanded*/);
 	}

	return __super::HandleMessage(uMsg, wParam, lParam);
}

LRESULT CUpdateWnd::OnTimer(UINT uMsg, WPARAM wParam, LPARAM lParam/*, BOOL& bHanded*/)
{
	if (wParam == 1)
	{
		SwitchChangeLog(++m_nChangelogIndex);
	}
	return 0;
}

//消息函数， 更新日志加载完之后开始下载安装包进行更新, 此时的界面能够显示有更新日志的图片
LRESULT CUpdateWnd::OnBeginUpdate(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
 	if(m_pLoadingView == NULL)
 		return 0;
 	m_pLoadingView->SetVisible(false);
 	m_pArrowBtnView->SetVisible(true);
 	//关闭gif
 	m_pGifControl->Stop();
 
 	CLabelUI* pUpdateTipLabel = static_cast<CLabelUI*>(m_PaintManager.FindControl(_T("updatetip")));
 	if (pUpdateTipLabel)
 	{
 		TCHAR tszText[1024] = {0x0};
 		_stprintf_s(tszText, _T("已检测到有最新版本%s可供使用， 您需要下载吗？"),m_pUpdateInfo->m_pProductName.c_str());
 		pUpdateTipLabel->SetText(tszText);
 	}
 
	AddButtons();
	SetTimer(m_hWnd, 1, 3000, NULL);

	//更新出错， 此处应该是系统中仍有被更新的程序的进程
	//tString tstrExe = m_pUpdateInfo->m_pProductName+ _T(".exe");
	if (GetProcessInstances(/*m_pUpdateInfo->m_pProductName.c_str()*/_T("哈哈ClipSpy")) > 0)
	{
		m_pErrorView->SetVisible(true);
		CLabelUI* pErrortipLabel = static_cast<CLabelUI*>(m_PaintManager.FindControl(_T("errortip")));
		tString tstrError=_T("检测系统中仍有");
		tstrError += m_pUpdateInfo->m_pProductName + _T("进程在运行，请先关闭后重新尝试");
		pErrortipLabel->SetText(tstrError.c_str());
		CButtonUI* pBtn = static_cast<CButtonUI*>(m_PaintManager.FindControl(_T("install_error")));
		pBtn->SetText(_T("重试"));
	}
	else
	{
		m_pUpdateView->SetVisible(true);
	}
	
	return 0;
}


LRESULT CUpdateWnd::OnMouseHover(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	bHandled = FALSE; 
	//获取鼠标移动到的坐标
	POINT pt;
	::GetCursorPos(&pt);
	//pt.x = GET_X_LPARAM(lParam); pt.y = GET_Y_LPARAM(lParam) -32;
	::ScreenToClient(*this, &pt);
	CControlUI* pControl = m_PaintManager.FindControl(pt);
	int i = 0;
	for (vector<CControlUI*>::iterator it = m_vcLogBtn.begin(); it != m_vcLogBtn.end(); ++it, ++i)
	{
		if (pControl == *it)
		{
			if (m_nChangelogIndex == i)
			{
				return 0;
			}
			m_nChangelogIndex = i;
			COptionUI* pOption  = (COptionUI*)pControl;
			pOption->Selected(true);
			bHandled = true;
			break;
		}
	}
	
	return 0;
}

void CUpdateWnd::Notify(TNotifyUI& msg)
{
	if (_tcsicmp(msg.sType, _T("click")) == 0)
	{
		if (_tcsicmp(msg.pSender->GetName(), _T("closebtn")) == 0)
		{
			
			//Clear();
			//::PostQuitMessage(0);
			Exit();
		}
		else if (_tcsicmp(msg.pSender->GetName(), _T("updateOK_btn")) == 0)
		{
			OnBtn_UpdateOK(msg);
		}
		else if (_tcsicmp(msg.pSender->GetName(), _T("updateCancel_btn")) == 0)
		{
			OnBtn_UpdateCancel(msg);
		}
		else if (_tcsicmp(msg.pSender->GetName(), _T("downloadcancel_btn")) == 0)
		{
			OnBtn_DownloadCancel(msg);
		}
		else if (_tcsicmp(msg.pSender->GetName(), _T("installok_btn")) == 0)
		{
			OnBtn_InstallOK(msg);
		}
		else if (_tcsicmp(msg.pSender->GetName(), _T("install_error")) == 0)
		{
			OnBtn_InstallError(msg);
		}
		else if (_tcsicmp(msg.pSender->GetName(), _T("leftarrow_btn")) == 0)
		{
			KillTimer(m_hWnd, 1);
			SwitchChangeLog(m_nChangelogIndex-1);
			SetTimer(m_hWnd, 1, 3000, NULL);
		}
		else if (_tcsicmp(msg.pSender->GetName(), _T("rightarrow_btn")) == 0)
		{
			KillTimer(m_hWnd, 1);
			SwitchChangeLog(m_nChangelogIndex + 1);
			SetTimer(m_hWnd, 1, 3000, NULL);
		}
		else
			__super::Notify(msg);
	}
	else if (_tcsicmp(msg.sType, _T("selectchanged")) == 0)
	{
		OnSelectChanged(msg);
	}
	else
		__super::Notify(msg);
}

//OPTION选项改变出发背景图片的切换
void CUpdateWnd::OnSelectChanged(TNotifyUI& msg)
{
	int i = (int)(msg.pSender->GetTag());

	m_pBkgroudContainer->SetBkImage(m_pUpdateInfo->m_vcImageName.at(i).c_str());

}

void CUpdateWnd::SwitchChangeLog(int nIndex)
{
	if (nIndex < 0)
	{
		m_nChangelogIndex = 0;
		return;
	}

	m_nChangelogIndex = nIndex;
	if (nIndex >= m_pUpdateInfo->m_vcImageName.size())
	{
		m_nChangelogIndex =  0;//m_pUpdateInfo->m_vcImageName.size() - 1;
	}

	((COptionUI*)m_vcLogBtn[m_nChangelogIndex])->Selected(true);
	
}

//动态添加按钮控件
void CUpdateWnd::AddButtons()
{
	int nBtnNum = m_pUpdateInfo->m_vcImageName.size();
	int cx = m_pUpdateInfo->m_size.cx; //545
	int nBtnWidth = 7;
	int nBtnHeight = 8;
	int interval = 10;
	int nBeginPos = (cx - nBtnWidth*nBtnNum - (nBtnNum - 1))/2;
	CHorizontalLayoutUI* pContainer = static_cast<CHorizontalLayoutUI*>(m_PaintManager.FindControl(_T("btncontainer")));

	TCHAR tszAttrlist[500];
	for (int i = 0; i < nBtnNum; ++i)
	{
		_stprintf_s(tszAttrlist, _T("name=\"change_log_btn%d\" float=\"true\" pos=\"%d,1,0,0\" width=\"%d\" height=\"%d\" normalimage=\"dian2.png\" selectedimage=\"dian1.png\" hotimage=\"dian1.png\" group=\"change_logtab\" "),i,  nBeginPos+i*(nBtnWidth + interval), nBtnWidth, nBtnHeight);


		COptionUI* pOption = new COptionUI;
		pOption->ApplyAttributeList(tszAttrlist);
		pOption->SetTag(UINT_PTR(i));
		pContainer->Add(pOption);
		m_vcLogBtn.push_back(pOption);
	}
	((COptionUI*)m_vcLogBtn[0])->Selected(true);
	//添加日志左右切换按钮
	memset(tszAttrlist, 0x0, sizeof(tszAttrlist));
	//CVerticalLayoutUI* pVerticalLayout = static_cast<CVerticalLayoutUI*>(m_PaintManager.FindControl(_T("arrowbtncontainer")));
	CButtonUI* pLeftBtn = new CButtonUI;
	_tcscpy(tszAttrlist, _T("name=\"leftarrow_btn\" float=\"true\" pos=\"10,0,0,0\" width=\"35\" height=\"35\" normalimage=\"leftarrow1.png\" pushedimage=\"leftarrow3.png\" hotimage=\"leftarrow2.png\" "));
	pLeftBtn->ApplyAttributeList(tszAttrlist);
	m_pArrowBtnView->Add(pLeftBtn);
	CButtonUI* pRightBtn = new CButtonUI;
	_stprintf_s(tszAttrlist,  _T("name=\"rightarrow_btn\" float=\"true\" pos=\"%d,0,0,0\" width=\"35\" height=\"35\" normalimage=\"rightarrow1.png\" pushedimage=\"rightarrow3.png\" hotimage=\"rightarrow2.png\" "), cx-45);
	pRightBtn->ApplyAttributeList(tszAttrlist);
	m_pArrowBtnView->Add(pRightBtn);
}

void CUpdateWnd::SetUpdateTips()
{
	CLabelUI*  pDownloadTip = static_cast<CLabelUI*>(m_PaintManager.FindControl(_T("updatetip")));
	if (pDownloadTip != NULL)
	{
		TCHAR tszTip[MAX_PATH + 1] = {0x0};
		_stprintf_s(tszTip, _T("%s%s"), /*m_tszVerName*/_T(""), _T("下载进度"));
		pDownloadTip->SetText(tszTip);
	}
	CLabelUI* pInstallTip = static_cast<CLabelUI*>(m_PaintManager.FindControl(_T("installtip")));
	if (pInstallTip != NULL)
	{
		TCHAR tszTip[MAX_PATH + 1] = {0x0};
		_stprintf_s(tszTip, _T("%s%s"), _T("正在安装"), /*m_tszVerName*/_T(""));
		pInstallTip->SetText(tszTip);
	}
	CLabelUI* pCompletetip = static_cast<CLabelUI*>(m_PaintManager.FindControl(_T("completetip")));
	if (pCompletetip != NULL)
	{
		TCHAR tszTip[MAX_PATH + 1] = {0x0};
		_stprintf_s(tszTip, _T("%s%s"),  /*m_tszVerName*/_T(""), _T("已经成功安装"));
		pCompletetip->SetText(tszTip);
	}

	
}

void CUpdateWnd::OnBtn_UpdateOK(TNotifyUI& msg)
{
	//显示下载界面
	m_pUpdateView->SetVisible(false);
	m_pDownloadView->SetVisible(true);
	m_pTitle->SetText(_T("软件下载"));

	TCHAR  tszText[128] = {0x0};
	_stprintf_s(tszText, _T("总共%d个文件， 正在下载第1个"), m_pUpdateInfo->m_vcVersion.size());
	m_pDownloadTip->SetText(tszText);

	unsigned   downloadThreadAddr;

	int nSize = m_pUpdateInfo->m_vcVersion.size();
	
	hWndCallBk = m_hWnd ;//设置回调函数消息传递句柄
	m_pDownloadPara = new download_para;
	m_pDownloadPara->pUpdateInfo = m_pUpdateInfo;
	m_pDownloadPara->pUpdateError = m_pUpdateError;
	m_pDownloadPara->pcalbk = DownloadProgress;
	m_pDownloadPara->hWnd = m_hWnd; 
	//开启线程进行下载
	m_hDownloadThread = (HANDLE)_beginthreadex(NULL,
		0 ,
		DownLoadUpdatePackage, 
		m_pDownloadPara, 
		CREATE_SUSPENDED,
		&downloadThreadAddr);

	//创建解压线程,解压覆盖
	m_pUnzipPara = new unzipthread_para;
	
	//获取去掉后缀的文件名
	//_tcscpy_s(m_tszVerName,_tcslen(m_pUnzipPara->tszZipName)+1,  m_pUnzipPara->tszZipName);
	//(_tcsrchr(m_tszVerName, _T('.')))[0] = 0;//删除文件名，只获得路径

	//用于测试
	//_tcscpy(m_pUnzipPara->tszZipName, _T("BaiduYunGuanjia_4.8.3.1409021519.zip"));
	m_pUnzipPara->pUpdateInfo = m_pUpdateInfo;
	m_pUnzipPara->pUpdateError = m_pUpdateError;
	m_pUnzipPara->hThread = m_hDownloadThread;
	m_pUnzipPara->hWnd = m_hWnd;
	TCHAR tszFileDir[MAX_FILENAME_SIZE +1] = {0x0};
	GetCurDir(tszFileDir, MAX_FILENAME_SIZE + 1);
	_tcscpy_s(m_pUnzipPara->tszDir, _tcslen(tszFileDir)+ 1,tszFileDir);
	unsigned unzipThreadAddr;

	//开启线进行解压
	 m_hUnzipThread = (HANDLE)_beginthreadex(NULL,
		0, 
		/*UnzipUpdatePackage*/ UnzipUpdatePackageWithProgress,
		m_pUnzipPara, 
		CREATE_SUSPENDED,
		&unzipThreadAddr);

	SetUpdateTips();
	//唤醒线程
	ResumeThread(m_hUnzipThread);

	ResumeThread(m_hDownloadThread);

}

void CUpdateWnd::StartProcess()
{
	PROCESS_INFORMATION piProcInfo;
	STARTUPINFO siStartupInfo;
	memset(&siStartupInfo,0,sizeof(STARTUPINFO));
	siStartupInfo.cb = sizeof(STARTUPINFO);
	TCHAR tszExePath[MAX_FILENAME_SIZE + 1] = {0x0};
	GetCurDir(tszExePath, MAX_FILENAME_SIZE);
	_tcscat(tszExePath, _T("\\"));
	_tcscat(tszExePath, m_pUpdateInfo->m_pProductName.c_str());
	_tcscat(tszExePath, _T(".exe"));
	if (CreateProcess(
		tszExePath,//_T("D:\\imClient\\ImClient\\Debug\\ImClient\\liuliu.exe"),
		NULL,
		NULL, NULL, FALSE, 
		0,NULL, 0,
		&siStartupInfo, 
		&piProcInfo))
	{
		CloseHandle(piProcInfo.hThread);
		CloseHandle(piProcInfo.hProcess);
	}
}

void CUpdateWnd::OnBtn_UpdateCancel(TNotifyUI& msg)
{
	//::PostQuitMessage(0);
	StartProcess();
	Exit();
}

void CUpdateWnd::Close()
{
//	DWORD dwRet;
// 	GetExitCodeThread(m_hDownloadThread, &dwRet);
// 	if (dwRet == STILL_ACTIVE)
// 	{
// 		DWORD dw =0;
// 		TerminateThread(m_hDownloadThread, dw);
// 	}
	KillTimer(m_hWnd, 1);
	this->WindowImplBase::Close();
}

void CUpdateWnd::Exit()
{
	DWORD dw = 0;
	//TerminateThread(m_hDownloadThread, dw);
	Close();
	Clear();
	::PostQuitMessage(0);
}
//下载的时候点击取消按钮
void CUpdateWnd::OnBtn_DownloadCancel(TNotifyUI& msg)
{
	Exit();
	//::PostQuitMessage(0);
}

void CUpdateWnd::OnBtn_InstallOK(TNotifyUI& msg)
{
	StartProcess();
	//Clear();
	//::PostQuitMessage(0);
	Exit();
}

void CUpdateWnd::OnBtn_InstallError(TNotifyUI& msg)
{
	if (_tcscmp(msg.pSender->GetText(), _T("退出")) == 0)
	{
		Exit();
	}
	else if (_tcscmp(msg.pSender->GetText(), _T("重试")) == 0)
	{
		CButtonUI* pBtn = static_cast<CButtonUI*>(m_PaintManager.FindControl(_T("install_error")));
		if (GetProcessInstances(m_pUpdateInfo->m_pProductName.c_str()) <=0 )
		{
			pBtn->SetText(_T("退出"));
		}
		else
		{
			tString tstrMsg = _T("系统中仍有");
			tstrMsg += m_pUpdateInfo->m_pProductName;tstrMsg+= _T("未关闭需要强制关闭吗？");
			if (MessageBox(m_hWnd, tstrMsg.c_str(),_T("警告"), MB_OKCANCEL ) == IDOK)
			{
				KillProcess();
				pBtn->SetText(_T("退出"));
				m_pErrorView->SetVisible(false);
				m_pUpdateView->SetVisible(true);
			}
		}
	}
}

int CUpdateWnd::GetProcessInstances(LPCTSTR lptName)
{
	tString tstrExe= lptName;
	tstrExe += _T(".exe");
	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (INVALID_HANDLE_VALUE == hSnapshot)
	{
		return  -1;
	}
	PROCESSENTRY32 pe ;
	pe.dwSize = sizeof(PROCESSENTRY32);
	BOOL bOk;
	for (bOk = Process32First(hSnapshot, &pe); bOk; bOk = Process32Next(hSnapshot, &pe))
	{
		if (_tcscmp(pe.szExeFile, tstrExe.c_str()) == 0)
		{
			DWORD dwProcessID = pe.th32ProcessID;
			
			m_vcHProcess.push_back(pe.th32ProcessID);
		}
	}

	CloseHandle(hSnapshot);
	return m_vcHProcess.size();
}

void CUpdateWnd::KillProcess()
{
	for (int i=0; i < m_vcHProcess.size(); ++i)
	{
		HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, m_vcHProcess[i]);
		TerminateProcess(hProcess, 0);
		CloseHandle(hProcess);
	}
	m_vcHProcess.clear();
}

// LRESULT CUpdateWnd::OnBeginInstallation(UINT uMsg, WPARAM wParam, LPARAM lParam)
// {
// 	
// 		m_pDownloadView->SetVisible(false);
// 		m_pInstallView->SetVisible(true);
// 	
// 	return 0;
// }

LRESULT CUpdateWnd::OnDownloadProgress(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	int nProgress = (int)wParam;
	TCHAR tszProgress[10] = {0x0};
	_stprintf_s(tszProgress, _T("%d%s"), nProgress, _T("%"));
	m_pDownloadProgress->SetValue(nProgress);
	m_pDownloadProgress->SetText(tszProgress);
	
	int nNum = m_nDownloadNum;
	if (nProgress == 100 )
	{
		m_nDownloadNum++;
		if (m_nDownloadNum == m_pUpdateInfo->m_vcVersion.size())
		{
			m_pDownloadView->SetVisible(false);
			m_pInstallView->SetVisible(true);
			m_pTitle->SetText(_T("软件安装"));
			m_pCloseBtn->SetEnabled(false);
		}
		else
		{
			TCHAR  tszText[128] = {0x0};
			_stprintf_s(tszText, _T("总共%d个文件， 正在下载第%d个"), m_pUpdateInfo->m_vcVersion.size(), m_nDownloadNum+1);
				m_pDownloadTip->SetText(tszText);
		}
	}
	else if (nProgress == 102)
	{
		//此处应该是解压失败
		m_pDownloadView->SetVisible(false);
		m_pErrorView->SetVisible(true);
		CLabelUI* pErrortipLabel = static_cast<CLabelUI*>(m_PaintManager.FindControl(_T("errortip")));
		pErrortipLabel->SetText(_T("下载失败，请重新尝试"));
		return 0;
	}
	
	return 0 ;
}

//显示安装进度
LRESULT CUpdateWnd::OnInstallState(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	int nProgress = (int)wParam;

	if (nProgress == 101)
	{
		//解压成功过
		m_pInstallView->SetVisible(false);
		m_pCompleteView->SetVisible(true);
		m_pTitle->SetText(_T("更新完成"));
		m_pCloseBtn->SetEnabled(true);
		return 0;
	}
	if (nProgress == 102)
	{
		//此处应该是解压失败
		m_pInstallView->SetVisible(false);
		m_pErrorView->SetVisible(true);
		CLabelUI* pErrortipLabel = static_cast<CLabelUI*>(m_PaintManager.FindControl(_T("errortip")));
		pErrortipLabel->SetText(_T("更新失败，请重新安装"));
		m_pCloseBtn->SetEnabled(true);
		return 0;
	}
	TCHAR tszProgress[10] = {0x0};
	_stprintf_s(tszProgress, _T("%d%s"), nProgress, _T("%"));
	m_pUnzipProgress->SetValue(nProgress);
	m_pUnzipProgress->SetText(tszProgress);
	if (nProgress == 100)
	{
		//MessageBox(m_hWnd, _T("DD"), _T("DD"), MB_OK);
	}
	
	return 0 ;
}