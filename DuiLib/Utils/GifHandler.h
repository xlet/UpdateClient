#ifndef __GIFHANDLER_H__
#define __GIFHANDLER_H__

#pragma once



namespace DuiLib 
{
	

	class UILIB_API CGifHandler
	{
	public:
		CGifHandler();

		virtual ~CGifHandler();

		int GetFrameCount();

		void AddFrameInfo(GifTFontInfo* pFrameInfo);

		GifTFontInfo* GetNextFrameInfo();

		GifTFontInfo* GetCurrentFrameInfo();

		GifTFontInfo* GetFrameInfoAt(int index);

	private:
		CStdPtrArray  ImageInfos;
		int nCurrentFrame;
		int nFrameCount;
		bool isDeleting;

	};
}

#endif