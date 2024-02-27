#include "CAccountInfo.h"

/*
*/
CAccountInfo::CAccountInfo()
{
	memset(this, 0x00, sizeof(CAccountInfo));
}

/*
*/
CAccountInfo::CAccountInfo(void* server, CServerInfo* serverInfo, const char* url)
{
	memset(this, 0x00, sizeof(CAccountInfo));

	m_server = server;

	m_serverInfo = serverInfo;

	m_httpRequest = new CHttpRequest();

	m_url = new CString("https://aviationweather.gov/cgi-bin/data/dataserver.php?requestType=retrieve&dataSource=metars&hoursBeforeNow=1&format=xml&stationString=KPKB");
}

/*
*/
CAccountInfo::~CAccountInfo()
{
	delete m_url;

	delete m_httpRequest;
}

/*
*/
void CAccountInfo::RequestAccountInfo()
{
	m_httpRequest->UrlRequest(m_url->GetText());
}