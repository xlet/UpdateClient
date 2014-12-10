#include "BaseWnd.h"


namespace DuiLib
{
	CControlGifUI::CControlGifUI():m_pGif(NULL)
		,m_nPreUpdateDelay(0)
		,m_isUpdateTime(false)
	{

	}

	CControlGifUI::~CControlGifUI()
	{
		m_pManager->KillTimer(this, GIF_TIMER_ID);

		if (m_pGif)
		{
			delete m_pGif;
			m_pGif = NULL;
		}
	}

	LPCTSTR CControlGifUI::GetClass() const
	{
		return _T("ControlGifUI");
	}

	LPVOID CControlGifUI::GetInterface( LPCTSTR pstrName )
	{
		if( _tcscmp(pstrName, _T("ControlGifUI")) == 0 ) 
			return static_cast<CControlGifUI*>(this);

		return CControlUI::GetInterface(pstrName);
	}
	void CControlGifUI::SetVisible( bool bVisible /*= true*/ )
	{
		if(bVisible == false)
			m_pManager->KillTimer(this, GIF_TIMER_ID);

		CControlUI::SetVisible(bVisible);
	}

	void CControlGifUI::PaintStatusImage( HDC hDC )
	{
		if(m_pGif)
		{
			GifTFontInfo* pImageInfo = NULL;
			if (m_isUpdateTime)
			{
				m_isUpdateTime = false;
				pImageInfo = m_pGif->GetNextFrameInfo();
			}
			else
			{
				pImageInfo = m_pGif->GetCurrentFrameInfo();
			}
			if (pImageInfo)
			{
				RECT rcBmpPart = {0};
				RECT rcCorners = {0};
				rcBmpPart.right = pImageInfo->nX;
				rcBmpPart.bottom = pImageInfo->nY;
				CRenderEngine::DrawImage(hDC,pImageInfo->hBitmap,m_rcItem, m_rcPaint,rcBmpPart,rcCorners,pImageInfo->alphaChannel,255);
				if (m_nPreUpdateDelay != pImageInfo->delay)
				{
					m_pManager->KillTimer(this, GIF_TIMER_ID);
					m_pManager->SetTimer(this, GIF_TIMER_ID, pImageInfo->delay);
					m_nPreUpdateDelay = pImageInfo->delay;
				}
			}
		}
		//没有gif图片,则与普通按钮一样
		else
		{
			CControlUI::PaintStatusImage(hDC);
		}
	}

	void CControlGifUI::Stop()
	{
		m_pManager->KillTimer(this, GIF_TIMER_ID);
	}

	void CControlGifUI::DoEvent( TEventUI& event )
	{
		if( event.Type == UIEVENT_TIMER && event.wParam == GIF_TIMER_ID )
		{
			m_isUpdateTime = true;
			Invalidate();
			return;
		}

		CControlUI::DoEvent(event);
	}

	void CControlGifUI::SetAttribute( LPCTSTR pstrName, LPCTSTR pstrValue )
	{
		if( _tcscmp(pstrName, _T("NormalGifFile")) == 0 )
		{
			SetNormalGifFile(pstrValue);
		}

		CControlUI::SetAttribute(pstrName,pstrValue);
	}

	void CControlGifUI::SetNormalGifFile( LPCTSTR pstrName )
	{
		if(pstrName == NULL) return;

		if (m_pGif)
		{
			m_pManager->KillTimer(this, GIF_TIMER_ID);
			m_nPreUpdateDelay = 0;
			delete m_pGif;
			m_pGif = NULL;
		}

		m_pGif = CRenderEngine::LoadGif(STRINGorID(pstrName),0, 0);

		Invalidate();
	}
}




CBaseWnd::CBaseWnd(void)
{
}


CBaseWnd::~CBaseWnd(void)
{
}



LRESULT CBaseWnd::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LRESULT lRes = 0;
	BOOL bHandled = TRUE;
	switch (uMsg)
	{    
	case WM_NCACTIVATE:    lRes = OnNcActivate(uMsg, wParam, lParam, bHandled); break;
	case WM_NCCALCSIZE:    lRes = OnNcCalcSize(uMsg, wParam, lParam, bHandled); break;
	case WM_NCPAINT:       lRes = OnNcPaint(uMsg, wParam, lParam, bHandled); break;
	case WM_NCHITTEST:     lRes = OnNcHitTest(uMsg, wParam, lParam, bHandled); break;
	case WM_SIZE:          lRes = OnSize(uMsg, wParam, lParam, bHandled); break;

	default:
		bHandled = FALSE;
	}
	if( bHandled ) return lRes;
	//if( m_PaintManager.MessageHandler(uMsg, wParam, lParam, lRes) ) return lRes;
	return __super::HandleMessage(uMsg, wParam, lParam);
}

//屏蔽Enter和Esc键
LRESULT CBaseWnd::MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam, bool& bHandled)
{

	if( uMsg == WM_KEYDOWN ) {
		if( wParam == VK_RETURN ) {

			return true;
		}
		else if( wParam == VK_ESCAPE ) {
			return true;
		}

	}
	return __super::MessageHandler(uMsg, wParam, lParam,bHandled);
}


LRESULT CBaseWnd::OnNcActivate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	if( ::IsIconic(*this) ) bHandled = FALSE;
	return (wParam == 0) ? TRUE : FALSE;
}

LRESULT CBaseWnd::OnNcCalcSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	return 0;
}

LRESULT CBaseWnd::OnNcPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	return 0;
}

//类似MFC中的OnNcHitTest， 重写该函数，使得在整个窗口可以拖动的时候，控件不至于无效
LRESULT CBaseWnd::OnNcHitTest(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	POINT pt; pt.x = GET_X_LPARAM(lParam); pt.y = GET_Y_LPARAM(lParam);
	::ScreenToClient(*this, &pt);

	RECT rcClient;
	::GetClientRect(*this, &rcClient);


	RECT rcCaption = m_PaintManager.GetCaptionRect();
	if (-1 == rcCaption.bottom)
	{
		rcCaption.bottom = rcClient.bottom;
	}

	//一些静态的“控件” 像 ControlUI  ContainerUI Label等点击在上面是可以拖动的
	if( pt.x >= rcClient.left + rcCaption.left && pt.x < rcClient.right - rcCaption.right
		&& pt.y >= rcCaption.top && pt.y < rcCaption.bottom ) 
	{
		CControlUI* pControl = m_PaintManager.FindControl(pt);
		if (IsInStaticControl(pControl))
		{
			return HTCAPTION;
		}
	}


	return HTCLIENT;

}

//类似MFC中的OnSize
LRESULT CBaseWnd::OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	SIZE szRoundCorner = m_PaintManager.GetRoundCorner();
	if( !::IsIconic(*this) && (szRoundCorner.cx != 0 || szRoundCorner.cy != 0) ) {
		CDuiRect rcWnd;
		::GetWindowRect(*this, &rcWnd);
		rcWnd.Offset(-rcWnd.left, -rcWnd.top);
		rcWnd.right++; rcWnd.bottom++;
		HRGN hRgn = ::CreateRoundRectRgn(rcWnd.left, rcWnd.top, rcWnd.right, rcWnd.bottom, szRoundCorner.cx, szRoundCorner.cy);
		::SetWindowRgn(*this, hRgn, TRUE);
		::DeleteObject(hRgn);
	}

	bHandled = FALSE;
	return 0;
}

//
BOOL CBaseWnd::IsInStaticControl(CControlUI *pControl)
{
	BOOL bRet = FALSE;
	if (! pControl)
	{
		return bRet;
	}

	CDuiString strClassName;
	std::vector<CDuiString> vctStaticName;

	strClassName = pControl->GetClass();
	strClassName.MakeLower();
	vctStaticName.push_back(_T("controlui"));
	vctStaticName.push_back(_T("textui"));
	vctStaticName.push_back(_T("labelui"));
	vctStaticName.push_back(_T("containerui"));
	vctStaticName.push_back(_T("horizontallayoutui"));
	vctStaticName.push_back(_T("verticallayoutui"));
	vctStaticName.push_back(_T("tablayoutui"));
	vctStaticName.push_back(_T("childlayoutui"));
	vctStaticName.push_back(_T("dialoglayoutui"));

	std::vector<CDuiString>::iterator it = std::find(vctStaticName.begin(), vctStaticName.end(), strClassName);
	if (vctStaticName.end() != it)
	{
		CControlUI* pParent = pControl->GetParent();
		while (pParent)
		{
			strClassName = pParent->GetClass();
			strClassName.MakeLower();
			it = std::find(vctStaticName.begin(), vctStaticName.end(), strClassName);
			if (vctStaticName.end() == it)
			{
				return bRet;
			}

			pParent = pParent->GetParent();
		}

		bRet = TRUE;
	}

	return bRet;
}

LRESULT CBaseWnd::OnNcButtonDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	CControlUI* pLogoControl = static_cast<CControlUI*>(m_PaintManager.FindControl(_T("imlogo")));
	//点击下转移焦点，否则由于除了几个按钮盒输入框之外， 其他地方都是非客户区
	//点击空白的地方原来获得焦点的控件不会KillFocus
	if (pLogoControl != NULL && !pLogoControl->IsFocused())
	{
		pLogoControl->SetFocus();
	}
	bHandled = FALSE; //此处要置为false
	return 0;
}

