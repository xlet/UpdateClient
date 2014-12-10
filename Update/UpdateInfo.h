#ifndef _UPDATEINFO_H
#define  _UPDATEINFO_H

#include "Global.h"

#include <vector>
 
using namespace std;
/*
<?xml version="1.0" encoding="UTF-8" standalone="true"?>
<response>
	<success>true</success>
	<version url="http://10.0.40.18:8081/download/liuliu_0.0.2.msi" version="0.0.2" create_at="2014-09-10 10:50:42">
		<change_log title="更新1" description="简约而不简单" image="http://sdfasdf.jpg"/>
		<change_log title="跟新2" description="阿斯顿" image="http://asdfasdf.png"/>
	</version>
	<version url="http://10.0.40.18:8081/download/liuliu_0.0.3.msi" version="0.0.3" create_at="2014-09-10 10:50:51">
		<change_log title="重大更新" description="阿斯顿发生地方阿斯顿发" image="http://sdfasdfasd.gif"/>
	</version>
</response>
*/

//每个版本变更是更新的详细信息
class Change_log
{
public:
	Change_log();
	~Change_log();
public:
	tString m_tstrTitle;
	tString m_tstrdescribe;
	string  m_strImageUrl;
};

//版本变更类
class Version
{
public:	
	Version();
	~Version();
public:
	vector<Change_log*>  m_vcChangelogs;
	tString             m_tstrTime;
	string              m_strUrl;
	tString             m_tstrVersion;
};

//备份文件信息
struct BackUpInfo
{
	tString tstrOldPath;
	tString tstrBackUpPath;
};

//该类只有m_vcVersion一个参数， 如果有新的更新信息此类可以进行扩展
class UpdateInfo
{
public:
	UpdateInfo();
	~UpdateInfo();
public: 
	//
	vector<Version*>	   m_vcVersion;
	vector<tString>        m_vcImageName;
	//下载的更新包的文件名
	vector<tString>        m_vcPackageName;  
	SIZE                   m_size;  // 窗体size
	tString                m_pProductName;	
	vector<BackUpInfo*>	   m_vcBackUpInfo;
	//tString				  m_tstrExeName;
};
class UpdateError
{
public:
	UpdateError(/*UpdateInfo* p, */tString& tstrHost, tString& tstrPostData, int port=80, tString tstrRequest=_T("/api/v1/feedback"));
	~UpdateError();
	BOOL  SendErrorMsg(LPCTSTR lptError);
private:
	UpdateError();
	tString			  m_tStrPostData;
	int				  m_nPort;
	HINTERNET         m_hOpen;
	HINTERNET		  m_hConnect;
	HINTERNET		  m_hRequest;
	UpdateInfo*       m_pUpdateInfo;
	tString			  m_tstrRequest; 
	tString			  m_tstrHost;
};

#endif