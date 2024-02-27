#pragma once

#include "framework.h"

#include "CAccountInfo.h"
#include "CCollision.h"
#include "CErrorLog.h"
#include "CFrametime.h"
#include "CHeapArray.h"
#include "CNetwork.h"
#include "CServerInfo.h"
#include "CTimer.h"

class CServer
{
public:

	CCollision* m_collision;
	CErrorLog* m_errorLog;
	CFrametime* m_frametime;
	CHeapArray* m_serverInfo;
	CServerEnvironment* m_serverEnvironment;
	CTimer* m_timerCountdown;
	CTimer* m_timerReload[CServerInfo::E_MAX_CLIENTS];

	SOCKET m_listenSocket;
	
	bool m_listenThreadRunning;
	
	int m_blueTeamCount;
	int m_countdown;
	int m_redTeamCount;

	WSADATA	m_wsaData;

	void (*pFunc[CNetwork::ServerEvent::E_SE_MAX_EVENTS]) (CServer* server, CNetwork* network, CServerInfo* serverInfo);

	CServer();
	~CServer();

	void Accept();
	void CreateClient(CNetwork* network, SOCKET tempSocket);
	void CreateListenSocket(const char* port);
	void DestroyClient(CServerInfo* serverInfo);
	void InitializeWinsock();
	void ProcessEvent(CNetwork* network);
	void ReceiveClients();
	void ReceiveGameSocket(CServerInfo* serverInfo);
	void ReceiveLobbySocket(CServerInfo* serverInfo);
	void RequestActivity(CServerInfo* serverInfo);
	void SendNetwork(CNetwork* network);
	void SendNetwork(CNetwork* network, SOCKET tempSocket);
	void SendUpdates();
	void Shutdown();
	void ShutdownClients();
	void ShutdownListen();
	void StartListenSocket();
	void StartServer(const char* port);

private:

	CNetwork* m_network;
	
	HANDLE m_listenThreadHandle;
	
	UINT m_listenThreadId;

	struct addrinfo* m_addrResult = {};
	struct addrinfo* m_addrPtr = {};
	struct addrinfo  m_addrHints = {};
};