#pragma once

#include "framework.h"

#include "../GameCommon/CCollision.h"
#include "../GameCommon/CErrorLog.h"
#include "../GameCommon/CFrametime.h"
#include "../GameCommon/CHeapArray.h"
#include "../GameCommon/CLinkList.h"
#include "../GameCommon/CNetwork.h"
#include "../GameCommon/CServerInfo.h"
#include "../GameCommon/CSocket.h"
#include "../GameCommon/CTimer.h"

#include "CMatchTime.h"
#include "CServerEnvironment.h"

class CServer
{
public:

	enum ServerState
	{
		E_AWAITING_CONNECTION = 0,
		E_COUNTDOWN,
		E_GAME_RUNNING,
		E_END_MATCH,
		E_MAX_STATE
	};

	CCollision* m_collision;
	CErrorLog* m_errorLog;
	CFrametime* m_frametime;
	CHeapArray* m_serverInfos;
	CLinkList<CCollisionPrimitive>* m_collisionPrimitives;
	CLinkList<CServerObject>* m_collectables;
	CLinkList<CString>* m_mapList;
	CLinkListNode<CCollisionPrimitive>* m_collisionPrimitive;
	CLinkListNode<CPlayerStart>* m_playerStart;
	CLinkListNode<CServerObject>* m_collectable;
	CLinkListNode<CString>* m_currentMap;
	CMatchTime* m_endMatchTime;
	CMatchTime* m_matchTime;
	CNetwork* m_networkReceive;
	CNetwork* m_networkSend;
	CServerEnvironment* m_serverEnvironment;
	CServerInfo* m_acceptServerInfo;
	CServerInfo* m_clientServerInfo;
	CServerInfo* m_serverInfo;
	CSocket* m_listenSocket;
	CString* m_mapName;
	CTimer* m_countdownTime;
	CVec3f m_lastDirection;
	CVec3f m_pointOnPlane;
	CVec3i m_sectorIndex;
	
	bool m_listenThreadRunning;
	bool m_playerActive;

	errno_t m_err;

	FILE* m_fMapList;

	HANDLE m_listenThreadHandle;

	int m_blueTeamCount;
	int m_countdown;
	int m_redTeamCount;
	int m_state;
	int m_totalBytes;
	int m_winsockStartupResult;

	UINT m_listenThreadId;

	WORD m_winsockVersionRequested;

	WSADATA	m_wsaData;

	typedef void (CServer::* TMethod)();

	TMethod m_method[CNetwork::ClientEvent::E_CE_MAX_EVENTS];
	TMethod m_frame[CServer::E_MAX_STATE];

	CServer();
	~CServer();

	// Base
	void AdvanceTimers();
	void AwaitingConnection();
	void CheckCollectables();
	void CreateClient(SOCKET socket);
	void DestroyClient(CServerInfo* serverInfo);
	void Frame();
	void IdleTimeout();
	void InitializeWinsock();
	void LoadEnvironment(CString* mapName);
	void MapChange(CString* mapName);
	void PollClients();
	void ProcessEvent();
	void ReadyCheck();
	void ReceiveClients();
	void RequestActivity(CServerInfo* serverInfo);
	void Reset();
	void ResetClients();
	void SendClientsLoadEnvironment(CString* mapName);
	void SendNetwork(CNetwork* network);
	void SendNetwork(CNetwork* network, CSocket* socket);
	void SendUpdates();
	void ShutdownClients();
	void ShutdownListen();
	void Start(const char* port);
	void Stop();

	// Network
	void Activity();
	void Chat();
	void Disconnect();
	void Exit();
	void Info();
	void NullActivity();
	void Ready();

	// Server State
	void Countdown();
	void EndMatch();
	void GameRunning();

	static unsigned int __stdcall ListenThread(void* obj);
};