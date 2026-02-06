#include "CBot.h"

/*
*/
CBot::CBot()
{
	memset(this, 0x00, sizeof(CBot));
}

/*
*/
CBot::~CBot()
{
	if (m_isRunning)
	{
		CBot::Stop();
	}

	SAFE_DELETE(m_camera);
}

/*
*/
void CBot::Constructor(CErrorLog* errorLog, CFrametime* frametime, CHeapArray* players, CServerInfo* serverInfo)
{
	memset(this, 0x00, sizeof(CBot));

	m_errorLog = errorLog;

	m_frametime = frametime;

	m_players = players;

	m_serverInfo = serverInfo;

	m_camera = new CCamera();
}

/*
*/
void CBot::Decision()
{
	memset(m_serverInfo->m_activity, 0x00, CServerInfo::E_MAX_ACTIVITY);

	switch (m_state)
	{
	case CBot::State::E_IDLE:
	{
		break;
	}
	case CBot::State::E_ROAMING:
	{
		m_serverInfo->m_activity[0] = CNetwork::ClientActivity::E_CA_FORWARD;

		m_camera->UpdateView();

		m_serverInfo->SetDirection(&m_camera->m_look);
		
		m_serverInfo->SetRight(&m_camera->m_right);

		m_nearestPlayer = CBot::NearestEnemyPlayer();

		if (m_nearestPlayer != nullptr)
		{
			m_state = CBot::State::E_FIGHTING;
		}

		break;
	}
	case CBot::State::E_FIGHTING:
	{
		m_nearestPlayer = CBot::NearestEnemyPlayer();

		if (m_nearestPlayer == nullptr)
		{
			m_state = CBot::State::E_ROAMING;
		}
		else
		{
			m_serverInfo->m_activity[0] = CNetwork::ClientActivity::E_CA_ATTACK;

			m_serverInfo->m_velocity = 0.0f;
		}

		break;
	}
	case CBot::State::E_CHASING:
	{
		break;
	}
	case CBot::State::E_FLEEING:
	{
		break;
	}
	case CBot::State::E_ENTERING:
	{
		m_camera->Constructor(1024.0f, 768.0f, &m_serverInfo->m_position, 45.0f, 1.0f, 50000.0f, (16.0f / 9.0f));

		m_camera->UpdateRotation(m_serverInfo->m_direction.m_p.x, m_serverInfo->m_direction.m_p.y, m_serverInfo->m_direction.m_p.z);

		m_camera->UpdateView();

		m_serverInfo->SetDirection(&m_camera->m_look);
		
		m_serverInfo->SetRight(&m_camera->m_right);

		m_state = CBot::State::E_ROAMING;

		break;
	}
	}
}

/*
*/
void CBot::Deconstructor()
{
	CBot::~CBot();
}

/*
*/
CServerInfo* CBot::NearestEnemyPlayer()
{
	CServerInfo* nearest = nullptr;

	float distance = 8.0f;

	for (int32_t i = 0; i < CServerInfo::E_MAX_CLIENTS; i++)
	{
		if (i != m_serverInfo->m_clientNumber)
		{
			CServerInfo* serverInfo = (CServerInfo*)m_players->GetElement(1, i);

			if ((serverInfo->m_isRunning) || (serverInfo->m_isBot))
			{
				if (m_serverInfo->m_team != serverInfo->m_team)
				{
					CVec3f p = m_serverInfo->m_position - serverInfo->m_position;

					float l = p.Length();

					if (l < distance)
					{
						distance = l;

						nearest = serverInfo;
					}
				}
			}
		}
	}

	return nearest;
}

/*
*/
void CBot::SetIdle()
{
	m_state = CBot::State::E_IDLE;

	m_eventState = CBot::EventState::E_INFERENCING;
}

/*
*/
void CBot::Start()
{
	m_errorLog->WriteError(true, "CBot::Start::Thread Starting\n");

	CBot::SetIdle();

	m_isRunning = true;

	m_threadHandle = (HANDLE)_beginthreadex(NULL, sizeof(CBot), &CBot::InferenceThread, (void*)this, 0, &m_threadId);

	CloseHandle(m_threadHandle);

	m_threadHandle = 0;
}

/*
*/
void CBot::Stop()
{
	m_errorLog->WriteError(true, "CBot::Stop::Thread Ending\n");

	CBot::SetIdle();

	m_isRunning = false;
}

/*
*/
unsigned __stdcall CBot::InferenceThread(void* obj)
{
	CBot* bot = (CBot*)obj;

	while (bot->m_isRunning)
	{
		switch (bot->m_eventState)
		{
		case CBot::EventState::E_REQUESTING:
		{
			bot->m_eventState = CBot::EventState::E_AWAITING_SERVER;

			break;
		}
		case CBot::EventState::E_INFERENCING:
		{
			bot->Decision();

			break;
		}
		}
	}

	_endthreadex(0);

	return 0;
}