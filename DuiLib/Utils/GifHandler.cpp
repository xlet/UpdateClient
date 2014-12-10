#include "StdAfx.h"

#include "GifHandler.h"

namespace DuiLib 
{
	CGifHandler::CGifHandler():
    nCurrentFrame(0),
	isDeleting(false)
	{
	}

	CGifHandler::~CGifHandler()
	{
		isDeleting = true;
		for(int i = 0; i < ImageInfos.GetSize(); i++)
		{
			CRenderEngine::FreeImage((GifTFontInfo *)ImageInfos.GetAt(i));
		}
	}

	int CGifHandler::GetFrameCount()
	{
		return ImageInfos.GetSize();
	}

	void CGifHandler::AddFrameInfo(GifTFontInfo* pFrameInfo)
	{
		if (pFrameInfo)
		{
			ImageInfos.Add(pFrameInfo);
		}	
	}

	GifTFontInfo* CGifHandler::GetNextFrameInfo()
	{
		if(isDeleting == false)
		{	
			int n = nCurrentFrame++;
			if (nCurrentFrame >= ImageInfos.GetSize())
			{
				nCurrentFrame = 0;
			}

			return (GifTFontInfo *)ImageInfos.GetAt(n);
		}

		return NULL;
	}

	GifTFontInfo* CGifHandler::GetCurrentFrameInfo()
	{
		if(isDeleting == false)
		{	
			return (GifTFontInfo *)ImageInfos.GetAt(nCurrentFrame);
		}

		return NULL;
	}

	GifTFontInfo* CGifHandler::GetFrameInfoAt(int index)
	{
		if(isDeleting == false)
		{	
			return (GifTFontInfo *)ImageInfos.GetAt(index);
		}

		return NULL;
	}
}