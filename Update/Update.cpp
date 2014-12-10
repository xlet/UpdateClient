#include <windows.h>
#include <tchar.h>

#include "UpdateWnd.h"

#include "resource.h"


// 确认是http请求
BOOL Isttp()
{
	tString tstrPostUrl;
	GetUrlFromCommandline(tstrPostUrl);
	TCHAR str[5] = _T("");

	// 
	_tcsncpy_s(str, tstrPostUrl.c_str(),  4);
	

	// 
	for(TCHAR *p = str; *p; p++)
		*p = tolower(*p);

	return !_tcsicmp(_T("http"), str);
}


//主函数，程序入口
int APIENTRY _tWinMain(HINSTANCE hInstance,
	HINSTANCE /*hPrevInstance*/,
	LPTSTR    lpCmdLine,
	int       nCmdShow)
{
	//::CoInitialize(NULL);
	//MessageBox(NULL, _T("hrllo"), _T("Update"), MB_OKCANCEL);
	//通过main函数参数来判断调用此程序
	
	if (!Isttp())
	{
		return 0;
	}
//	LPCTSTR lptstr = GetCommandLine();

 //	MessageBox(NULL, lptstr, _T("TISHI"), MB_OK);
// 	if (!Isttp(lpCmdLine))
// 
// 	{
// #ifdef _DEBUG
// 	MessageBox(NULL, lptstr, _T("TISHI"), MB_OK);
// #endif
// 		return 0;
// 	}


	//////////////////////////////////////////////////////////////////////////
	CPaintManagerUI::SetInstance(hInstance);
	CPaintManagerUI::SetResourcePath(CPaintManagerUI::GetInstancePath() /*+ _T("skin\\")*/);

	CUpdateWnd *pUpdateWnd = new CUpdateWnd;
	if (pUpdateWnd == NULL)
	{
		return 0;
	}
	pUpdateWnd->Create(NULL, _T("自动更新"), UI_WNDSTYLE_DIALOG,WS_EX_WINDOWEDGE);
	pUpdateWnd->CenterWindow();
		pUpdateWnd->SetIcon(IDI_ICON1);
	pUpdateWnd->ShowModal();

	CPaintManagerUI::MessageLoop();
	CPaintManagerUI::Term();
	//::CoUninitialize();
	return 0;
}