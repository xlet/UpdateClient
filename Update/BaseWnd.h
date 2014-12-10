#ifndef _BASEWND_H
#define _BASEWND_H
#pragma once
#include <windows.h>
#include <commctrl.h>
#include <tchar.h>
#include <objbase.h>
#include <UIlib.h>
#include<algorithm>

using namespace DuiLib;
#ifdef _DEBUG
#   ifdef _UNICODE
#       pragma comment(lib, "DuiLib_sud.lib")
#   else
#       pragma comment(lib, "DuiLib_sd.lib")
#   endif
#else
#   ifdef _UNICODE
#       pragma comment(lib, "DuiLib_su.lib")
#   else
#       pragma comment(lib, "DuiLib_s.lib")
#   endif
#endif

class CBaseWnd:public WindowImplBase
{
public:
	CBaseWnd(void);
	virtual ~CBaseWnd(void);
	LRESULT MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam, bool& bHandled);
	virtual void       InitWindow(){}
	LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnNcActivate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnNcCalcSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnNcPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnNcHitTest(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	BOOL IsInStaticControl(CControlUI *pControl);
	LRESULT OnNcButtonDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
protected:
	//void Notify(TNotifyUI& msg);
	void OnExit(TNotifyUI& msg){Close();};
};

namespace DuiLib
{
	class  CControlGifUI:public CControlUI
	{
	public:	
		CControlGifUI();
		~CControlGifUI();
		LPCTSTR GetClass() const;
		LPVOID GetInterface(LPCTSTR pstrName);
		void SetNormalGifFile(LPCTSTR pstrName);
		void SetAttribute(LPCTSTR pstrName, LPCTSTR pstrValue);
		virtual void DoEvent(TEventUI& event);
		virtual void SetVisible(bool bVisible = true);//设置控件可视性
		virtual void PaintStatusImage(HDC hDC);
		void Stop();
	protected:
		enum
		{
			GIF_TIMER_ID = 15
		};
		bool m_isUpdateTime; //控制gif只能根据定时器播放
		CGifHandler* m_pGif;
		int m_nPreUpdateDelay;
	};
}
#endif