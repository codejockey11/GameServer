#pragma once

#include "framework.h"

#include "CHttpRequest.h"
#include "CServerInfo.h"
#include "CString.h"

class CAccountInfo
{
public:

	CHttpRequest* m_httpRequest;
	CServerInfo* m_serverInfo;
	CString* m_url;
	
	void* m_server;
	
	CAccountInfo();
	CAccountInfo(void* server, CServerInfo* serverInfo, const char* url);
	~CAccountInfo();

	void RequestAccountInfo();
};