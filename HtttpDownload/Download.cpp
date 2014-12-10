// A file download subsystem


#include "stdafx.h"
#include "Download.h"

/**
下载一个文件

将文件的URL传给接口函数

filename为文件下载到本地的存储路径

第四个参数为回调函数指针，记录当前下载的状况， 默认为空
*/
bool Download::download(const char *url,const char*  fileDir, const char* filename, bool reload, void (*update)(unsigned long, unsigned long))
{
    ofstream fout;              // output stream
    unsigned char buf[BUF_SIZE];// 写入 buffer
    unsigned long numrcved;     // 每次读取的的字节长度
    unsigned long filelen;      // 文件在硬盘上的长度
    HINTERNET hIurl = NULL, hInet = NULL;     // internet handles
    unsigned long contentlen;   // length of content
    unsigned long len;          // length of contentlen
    unsigned long total = 0;    // running total of bytes received
    char header[80];            // holds Range header
	char filePath[MAX_FILENAME_SIZE] = {0x0};
    try
    {
        if(!ishttp(url))
			return false;
          //  throw DLExc("Must be HTTP url");

        /*
        fileDir为文件的目录，从url中获取文件名后得到文件存储路径
        fout返回打开的文件流. 如果reload为真, 那么将打开文件并删除。
		存在文件的长度（如果文件没被截断）将被返回。
        */
		char fname[MAX_PATH] = {0x0};
		if (filename == NULL || strlen(filename) == 0)
		{
			if(!getfname(url, fname))
				return false;
				//throw DLExc("File name error");
		}
		else
		{
			strcpy(fname, filename);
		}
		
		

		if (fileDir != NULL && strlen(fileDir)!=0 )
		{
			sprintf(filePath, "%s\\%s", fileDir, fname);
		}
		else
		{
			sprintf(filePath, "%s",  fname);
		}
		
		//sprintf(szFilePath, "%s\\")
		
       // filelen = openfile(filePath, reload, fout);

        //网络连接是否可用
        if(InternetAttemptConnect(0) != ERROR_SUCCESS)
			return false;
            //throw DLExc("Can't connect");

        // 打开网络连接
        hInet = InternetOpen("downloader", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
        if(hInet == NULL)
            throw DLExc("Can't open connection");

		 filelen = openfile(filePath, reload, fout);

        // 数据范围 头请求
		//Range:bytes=500-  表示500字节以后的范围
		//使用这个请求可以继续前一次的下载

        sprintf(header, "Range:bytes=%d-", filelen);

        // 打开url
        //hIurl = InternetOpenUrl(hInet, url, header, -1, INTERNET_FLAG_NO_CACHE_WRITE, 0);
        hIurl = InternetOpenUrl(hInet, url, header, strlen(header), INTERNET_FLAG_NO_CACHE_WRITE, 0);
        if(hIurl == NULL)
            throw DLExc("Can't open url");

        // 保证支持 HTTP/1.1 或者更高版本
        if(!httpverOK(hIurl))
            throw DLExc("HTTP/1.1 not supported");

        //得到资源长度
        len = sizeof contentlen;
		//char szzz[2048] = {0x0};
		if (!HttpQueryInfo(hIurl, HTTP_QUERY_FLAG_NUMBER | HTTP_QUERY_STATUS_CODE, &contentlen, &len, NULL))
		{
			throw DLExc("Can't get status code");
		}

		if (contentlen == 404)
		{
			throw DLExc("404");

		}

        if(!HttpQueryInfo(hIurl, HTTP_QUERY_CONTENT_LENGTH | HTTP_QUERY_FLAG_NUMBER, &contentlen, &len, NULL))
            throw DLExc("File or content length not found");
		
        // 如果需下载的文件存在， 并且没下载完， 那么完成下载
        if(filelen != contentlen && contentlen)
        {
            do
            {
                // 读取字节
                if(!InternetReadFile(hIurl, &buf, BUF_SIZE, &numrcved))
                    throw DLExc("Error occurred during download");

                // 写字节
                fout.write((const char *) buf, numrcved);
                if(!fout.good())
                    throw DLExc("Error writing file");

                // 更新总进度
                total += numrcved;

                // 回调函数返回下载状态
                if(update && numrcved > 0)
                    update(contentlen + filelen, total + filelen);
            } while (numrcved > 0);
        }
        else
        {
            if(update)
                update(filelen, filelen);
        }
    }
    catch (DLExc)
    {
        fout.close();
		DeleteFile(filePath);
        InternetCloseHandle(hIurl);
        InternetCloseHandle(hInet);
		//return false;
        // 重新跑出异常 供调用者使用
        throw;
    }

    fout.close();
    InternetCloseHandle(hIurl);
    InternetCloseHandle(hInet);

    return true;
}

bool Download::download(const char *url, char* pbuf, int bufsize)
{
	//DWORD db ;
	try
	{
		if(!ishttp(url))
			throw DLExc("Must be HTTP url");

		//网络连接是否可用
		if(InternetAttemptConnect(0) != ERROR_SUCCESS)
			throw DLExc("Can't connect");


		// 打开网络连接
		 HINTERNET hInet = InternetOpen("downloader", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
		 if(hInet == NULL)
			 throw DLExc("Can't open connection");

		 DWORD options = INTERNET_FLAG_NEED_FILE|INTERNET_FLAG_HYPERLINK|INTERNET_FLAG_RESYNCHRONIZE|INTERNET_FLAG_RELOAD;
		HINTERNET  hIurl = InternetOpenUrl(hInet, url, NULL, -1, /*INTERNET_FLAG_NO_CACHE_WRITE*/options, 0);
		if(hIurl == NULL)
			throw DLExc("Can't open url");
		// 保证支持 HTTP/1.1 或者更高版本
		if(!httpverOK(hIurl))
			throw DLExc("HTTP/1.1 not supported");

		//得到资源长度
 		DWORD content;
 		DWORD len = sizeof content;
// 		if(!HttpQueryInfo(hIurl, HTTP_QUERY_CONTENT_LENGTH | HTTP_QUERY_FLAG_NUMBER, &contentlen, &len, NULL))
// 			throw DLExc("File or content length not found");

		if (!HttpQueryInfo(hIurl, HTTP_QUERY_FLAG_NUMBER | HTTP_QUERY_STATUS_CODE, &content, &len, NULL))
		{
			throw DLExc("Can't get status code");
		}

		if (content >=400)
		{
			char szError[24] = {0x0};
			itoa(content,szError,  10);
			throw DLExc(szError);

		}

		// 读取字节, 此处比较简单对于比较大的文件如果是读在内存中可以分多次读取
		DWORD dwRedSizeOut;
		if(!InternetReadFile(hIurl, pbuf, bufsize, &dwRedSizeOut))
			{
				throw DLExc("Error occurred during download");
			}
		else
		{
			pbuf[dwRedSizeOut]= 0x0;
			return true;
		}
	}
	catch(DLExc )
	{
		throw;
	}
	return false;
}

// 返回true 如果http版本高于或等于1.1
bool Download::httpverOK(HINTERNET hIurl)
{
    char str[80];
    unsigned long len = 79;

    if(!HttpQueryInfo(hIurl, HTTP_QUERY_VERSION, &str, &len, NULL))
        return false;


    char *p = strchr(str, '/');
    p++;
    if(*p == '0')
        return false;       // 不能使用 HTTP 0.x

    //  str如:HTTP/1.1
    p = strchr(str, '.');
    p++;

    // 转成int 
    int minorVerNum = atoi(p);

    if(minorVerNum > 0)
        return true;

    return false;
}


//从url中得到http服务器上文件名称
bool Download::getfname(const char *url, char *fname)
{
    // 找到最后一个 /
   const  char *p = strrchr(url, '/');

    // 复制最后一个 / 后的文件名
    if(p && (strlen(p) < MAX_FILENAME_SIZE))
    {
        p++;
        strcpy(fname, p);
        return true;
    }
    else
    {
        return false;
    }
}

/*
	打开输出文件，初始化输出流，并返回文件的长度。如果reload  为true, 那么会删除已经存在（下载完或者未下载完）的文件
*/
unsigned long Download::openfile(const char * filename, bool reload, ofstream &fout)
{
#if 0
    char fname[MAX_FILENAME_SIZE];

    if(!getfname(url, fname))
        throw DLExc("File name error");

    if(!reload)
        fout.open(fname, ios::binary | ios::out | ios::app | ios::ate);
    else
        fout.open(fname, ios::binary | ios::out | ios::trunc);

    if(!fout)
        throw DLExc("Can't open output file");
#endif

	if(!reload)
		fout.open(filename, ios::binary | ios::out | ios::app | ios::ate);
	else
		fout.open(filename, ios::binary | ios::out | ios::trunc);

	if(!fout)
		throw DLExc("Can't open output file");

 //返回长度
    return fout.tellp();
}

// 确认是http请求
bool Download::ishttp(const char *url)
{
    char str[5] = "";

    // 
    strncpy(str, url, 4);

    // 
    for(char *p = str; *p; p++)
        *p = tolower(*p);

    return !strcmp("http", str);
}


//下载接口实现
bool  http_downloadfile(const char* url,const char*  fileDir, const char* filename, bool reload, download_callback callbk)
{
	/*
		char url[] = "http://img3.wcnimg.com/M00/44/5B/wKgKZVMXChriLAoGAABBjs_j4Go664-100-100.jpg";

		bool reload = false;

		if(argc==2 && !strcmp(argv[1], "reload"))
		reload = true;

		printf("Beginning download\n");

		try
		{
		if(Download::download(url, reload, showprogress))
		printf("Download Complete\n");
		}
		catch(DLExc exc)
		{
		printf("%s\n", exc.geterr());
		printf("Download interrupted\n");
		}

		system("PAUSE");
		return EXIT_SUCCESS;
	*/
	try
	{
		if (Download::download(url,fileDir,filename, reload,  callbk))
		{
			OutputDebugString("Download Complete \n");
		    return true;
		}
		else
			return false;
	}
	catch (DLExc exc)
	{
		OutputDebugString("\n");
		OutputDebugString(exc.geterr());
		OutputDebugString("Download Interrupted \n");
		return false;
	
	}
	return true;
}

bool http_downloadtobuf(const char* url, char* pbuf, int bufSize)
{
	try
	{
		if (Download::download(url, pbuf, bufSize))
		{
			OutputDebugString("Download Complete \n");
			return true;
		}
		else
			return false;
	}
	catch(DLExc exc)
	{
		OutputDebugString("\n");
		OutputDebugString(exc.geterr());
		OutputDebugString("Download Interrupted \n");
		return false;
	}
}