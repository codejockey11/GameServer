#pragma once

#include "framework.h"

#include "../GameCommon/CCollision.h"
#include "../GameCommon/CErrorLog.h"
#include "../GameCommon/CFrametime.h"
#include "../GameCommon/CHeapArray.h"
#include "../GameCommon/CList.h"
#include "../GameCommon/CLocal.h"
#include "../GameCommon/CNetwork.h"
#include "../GameCommon/CServerInfo.h"
#include "../GameCommon/CSocket.h"
#include "../GameCommon/CTimer.h"

#include "CBot.h"
#include "CMatchTime.h"
#include "CPlayerBox.h"
#include "CServerEnvironment.h"

class CServer
{
public:

	enum ServerState
	{
		E_AWAITING_CONNECTIONS = 0,
		E_COUNTDOWN,
		E_GAME_RUNNING,
		E_END_MATCH,
		
		E_MAX_STATE
	};

	enum
	{
		E_MAX_MAP_NAME = 32
	};

	bool m_listenThreadRunning;
	bool m_playerActive;

	CCollision* m_collision;

	char m_mapListName[CServer::E_MAX_MAP_NAME];

	CBot* m_bot;
	CCollisionPrimitive* m_collisionPrimitive;
	CErrorLog* m_errorLog;
	CFrametime* m_frametime;
	CHeapArray* m_serverInfos;
	CList* m_collectables;
	CList* m_collisionPrimitives;
	CList* m_mapList;
	CListNode* m_mapListNode;
	CListNode* m_node;
	CLocal* m_local;
	CMatchTime* m_endMatchTime;
	CMatchTime* m_matchTime;
	CNetwork* m_networkReceive;
	CPlayerBox* m_playerBox;
	CPlayerStart* m_playerStart;
	CServerEnvironment* m_serverEnvironment;
	CServerInfo* m_serverInfo;
	CServerObject* m_collectable;
	CSocket* m_listenSocket;
	CString* m_currentMap;
	CString* m_mapName;
	CTimer* m_countdownTime;
	CVec3f m_lastDirection;
	CVec3f m_pointOnPlane;
	CVec3i m_sectorIndex;

	errno_t m_err;

	FILE* m_fMapList;

	HANDLE m_listenThreadHandle;

	int32_t m_connections;
	int32_t m_winsockStartupResult;

	int32_t m_blueTeamCount;
	int32_t m_botCount;
	int32_t m_countdown;
	int32_t m_redTeamCount;
	int32_t m_state;
	int32_t m_totalBytes;

	uint16_t m_winsockVersionRequested;

	uint32_t m_listenThreadId;

	WSADATA	m_wsaData;

	typedef void (CServer::* TMethod)();

	TMethod m_event[CNetwork::ServerEvent::E_SE_MAX];
	TMethod m_frame[CServer::ServerState::E_MAX_STATE];

	CServer();
	CServer(CLocal* local, CErrorLog* errorLog);
	~CServer();

	void Activity();
	void AdvancePlayerTimers();
	void AdvanceServerTimers();
	void AwaitingConnection();
	void CheckCollectables();
	void ConsoleMessage();
	void Countdown();
	void CreateClient(SOCKET socket);
	void DestroyClient(CServerInfo* serverInfo);
	void Disconnect();
	void EndMatch();
	void Exit();
	void Frame();
	void GameRunning();
	void IdleTimeout();
	void Inference(CServerInfo* serverInfo);
	void InitializeWinsock();
	void LoadEnvironment();
	void MapChange();
	void NullActivity();
	void PlayerMovement();
	void PollClients();
	void ProcessEvent();
	void Ready();
	void ReadyCheck();
	void ReceiveClients();
	void RequestActivity(CServerInfo* serverInfo);
	void Reset();
	void ResetClients();
	void SendLoadEnvironment();
	void SendNetwork(CNetwork* network);
	void SendNetwork(CNetwork* network, CSocket* socket);
	void SendUpdates();
	void ShutdownClients();
	void ShutdownListen();
	void Start(const char* port);
	bool StartBot(CString* name);
	void StartBots();
	void Stop();
	void StopBot(CString* name);
	void StopBots();

	static unsigned int __stdcall ListenThread(void* obj);
};