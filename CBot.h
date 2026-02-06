#pragma once

#include "framework.h"

#include "../GameCommon/CCamera.h"
#include "../GameCommon/CErrorLog.h"
#include "../GameCommon/CFrametime.h"
#include "../GameCommon/CHeapArray.h"
#include "../GameCommon/CNetwork.h"
#include "../GameCommon/CServerInfo.h"

class CBot
{
public:

	enum State
	{
		E_IDLE = 0,
		E_ROAMING,
		E_FIGHTING,
		E_CHASING,
		E_FLEEING,
		E_ENTERING
	};

	enum EventState
	{
		E_INFERENCING = 0,
		E_REQUESTING,
		E_AWAITING_SERVER
	};

	bool m_isRunning;

	CCamera* m_camera;
	CErrorLog* m_errorLog;
	CFrametime* m_frametime;
	CHeapArray* m_players;
	CServerInfo* m_nearestPlayer;
	CServerInfo* m_serverInfo;

	HANDLE m_threadHandle;

	int32_t m_eventState;
	int32_t m_state;

	uint32_t m_threadId;

	CBot();
	~CBot();

	void Constructor(CErrorLog* errorLog, CFrametime* frametime, CHeapArray* players, CServerInfo* serverInfo);
	void Decision();
	void Deconstructor();
	CServerInfo* NearestEnemyPlayer();
	void SetIdle();
	void Start();
	void Stop();

	static unsigned int __stdcall InferenceThread(void* obj);
};

