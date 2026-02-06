#include "CServer.h"

/*
*/
CServer::CServer()
{
	memset(this, 0x00, sizeof(CServer));
}

/*
*/
CServer::CServer(CLocal* local, CErrorLog* errorLog)
{
	memset(this, 0x00, sizeof(CServer));

	m_winsockVersionRequested = MAKEWORD(2, 2);

	m_local = local;

	m_errorLog = errorLog;

	m_networkReceive = new CNetwork();

	m_frametime = new CFrametime();

	m_collision = new CCollision();

	m_playerBox = new CPlayerBox();

	m_serverInfos = new CHeapArray(true, sizeof(CServerInfo), 1, CServerInfo::E_MAX_CLIENTS);

	for (int32_t i = 0; i < CServerInfo::E_MAX_CLIENTS; i++)
	{
		CServerInfo* serverInfo = (CServerInfo*)m_serverInfos->GetElement(1, i);

		serverInfo->Constructor();

		serverInfo->m_socket->SetErrorLog(m_errorLog);
	}

	m_bot = new CBot[CServerInfo::E_MAX_CLIENTS]();

	m_countdown = 5;

	m_countdownTime = new CTimer(1000);

	m_matchTime = new CMatchTime(1000);

	m_endMatchTime = new CMatchTime(1000);

	m_event[CNetwork::ServerEvent::E_SE_ACTIVITY] = &CServer::Activity;
	m_event[CNetwork::ServerEvent::E_SE_DISCONNECT] = &CServer::Disconnect;
	m_event[CNetwork::ServerEvent::E_SE_EXIT] = &CServer::Exit;
	m_event[CNetwork::ServerEvent::E_SE_NULL_ACTIVITY] = &CServer::NullActivity;
	m_event[CNetwork::ServerEvent::E_SE_READY] = &CServer::Ready;
	m_event[CNetwork::ServerEvent::E_SE_READY_CHECK] = &CServer::ReadyCheck;

	m_frame[CServer::ServerState::E_COUNTDOWN] = &CServer::Countdown;
	m_frame[CServer::ServerState::E_END_MATCH] = &CServer::EndMatch;
	m_frame[CServer::ServerState::E_GAME_RUNNING] = &CServer::GameRunning;
	m_frame[CServer::ServerState::E_AWAITING_CONNECTIONS] = &CServer::AwaitingConnection;

	m_mapList = new CList();

	CString* mapList = new CString(m_local->m_installPath->m_text);

	mapList->Append("mapList.txt");

	m_err = fopen_s(&m_fMapList, mapList->m_text, "rb");

	if (m_err == 0)
	{
		fscanf_s(m_fMapList, "%s", &m_mapListName, CServer::E_MAX_MAP_NAME);

		while (!feof(m_fMapList))
		{
			m_mapName = new CString(m_mapListName);

			m_mapList->Append(m_mapName, m_mapListName);

			memset(m_mapListName, 0x00, CServer::E_MAX_MAP_NAME);

			fscanf_s(m_fMapList, "%s", &m_mapListName, CServer::E_MAX_MAP_NAME);
		}

		fclose(m_fMapList);
	}

	SAFE_DELETE(mapList);

	m_mapListNode = m_mapList->m_list;

	m_currentMap = (CString*)m_mapListNode->m_object;

	CServer::LoadEnvironment();

	m_state = CServer::ServerState::E_AWAITING_CONNECTIONS;
}

/*
*/
CServer::~CServer()
{
	if (m_listenThreadRunning)
	{
		CServer::Stop();
	}

	SAFE_DELETE_ARRAY(m_bot);

	m_node = m_mapList->m_list;

	while ((m_node) && m_node->m_object)
	{
		m_mapName = (CString*)m_node->m_object;

		SAFE_DELETE(m_mapName);

		m_node = m_node->m_next;
	}

	SAFE_DELETE(m_mapList);

	SAFE_DELETE(m_matchTime);
	SAFE_DELETE(m_countdownTime);
	SAFE_DELETE(m_serverInfos);
	SAFE_DELETE(m_playerBox);
	SAFE_DELETE(m_collision);
	SAFE_DELETE(m_frametime);
	SAFE_DELETE(m_networkReceive);

	WSACleanup();
}

/*
*/
void CServer::Activity()
{
	CServer::AdvancePlayerTimers();
	CServer::CheckCollectables();

	m_playerActive = false;

	bool andAttack = false;
	bool andForward = false;
	bool andBackward = false;
	bool andLeft = false;
	bool andRight = false;

	CVec3f up = m_serverInfo->m_direction.Cross(&m_serverInfo->m_right);

	for (int32_t i = 0; i < CNetwork::E_MAX_ACTIVITY; i++)
	{
		if (m_serverInfo->m_activity[i] == CNetwork::ClientActivity::E_CA_ATTACK)
		{
			andAttack = true;

			m_playerActive = true;
		}

		if (m_serverInfo->m_activity[i] == CNetwork::ClientActivity::E_CA_FORWARD)
		{
			andForward = true;

			m_playerActive = true;
		}

		if (m_serverInfo->m_activity[i] == CNetwork::ClientActivity::E_CA_BACKWARD)
		{
			andBackward = true;

			m_playerActive = true;
		}

		if (m_serverInfo->m_activity[i] == CNetwork::ClientActivity::E_CA_STEP_LEFT)
		{
			andLeft = true;

			m_playerActive = true;
		}

		if (m_serverInfo->m_activity[i] == CNetwork::ClientActivity::E_CA_STEP_RIGHT)
		{
			andRight = true;

			m_playerActive = true;
		}
	}

	if (andAttack)
	{
		if (!m_serverInfo->m_timerReload->m_isReloading)
		{
			m_serverInfo->m_timerReload->Start();

			m_serverInfo->m_state = 'A';

			m_serverInfo->m_reloadTime = 0;

			CNetwork* n = new CNetwork(CNetwork::ServerEvent::E_SE_TO_CLIENT, CNetwork::ClientEvent::E_CE_QUE_SOUND,
				(void*)m_serverInfo, sizeof(CServerInfo),
				(void*)"0", 1);

			CServer::SendNetwork(n);

			SAFE_DELETE(n);
		}
	}

	if ((andForward) && (andLeft))
	{
		m_serverInfo->m_velocity += m_playerBox->m_acceleration * m_frametime->m_frametime;

		m_serverInfo->m_direction = m_serverInfo->m_direction.RotateAngleByAxis(-45.0f * DEG2RAD, &up);
	}
	else if ((andForward) && (andRight))
	{
		m_serverInfo->m_velocity += m_playerBox->m_acceleration * m_frametime->m_frametime;

		m_serverInfo->m_direction = m_serverInfo->m_direction.RotateAngleByAxis(45.0f * DEG2RAD, &up);
	}
	else if ((andBackward) && (andLeft))
	{
		m_serverInfo->m_velocity += m_playerBox->m_acceleration * m_frametime->m_frametime;

		m_serverInfo->m_direction = m_serverInfo->m_direction.RotateAngleByAxis(-135.0f * DEG2RAD, &up);
	}
	else if ((andBackward) && (andRight))
	{
		m_serverInfo->m_velocity += m_playerBox->m_acceleration * m_frametime->m_frametime;

		m_serverInfo->m_direction = m_serverInfo->m_direction.RotateAngleByAxis(135.0f * DEG2RAD, &up);
	}
	else if ((andForward) && (andBackward))
	{
		m_serverInfo->m_velocity = 0.0f;
	}
	else if ((andLeft) && (andRight))
	{
		m_serverInfo->m_velocity = 0.0f;
	}
	else if (andForward)
	{
		m_serverInfo->m_velocity += m_playerBox->m_acceleration * m_frametime->m_frametime;
	}
	else if (andBackward)
	{
		m_serverInfo->m_velocity += m_playerBox->m_acceleration * m_frametime->m_frametime;

		m_serverInfo->m_direction = m_serverInfo->m_direction.RotateAngleByAxis(180.0f * DEG2RAD, &up);
	}
	else if (andLeft)
	{
		m_serverInfo->m_direction = m_serverInfo->m_direction.RotateAngleByAxis(-90.0f * DEG2RAD, &up);

		m_serverInfo->m_velocity += m_playerBox->m_acceleration * m_frametime->m_frametime;
	}
	else if (andRight)
	{
		m_serverInfo->m_direction = m_serverInfo->m_direction.RotateAngleByAxis(90.0f * DEG2RAD, &up);

		m_serverInfo->m_velocity += m_playerBox->m_acceleration * m_frametime->m_frametime;
	}


	m_serverInfo->m_direction.Normalize();

	/*
	if (m_serverInfo->m_velocity > m_playerBox->m_maxFreeflight)
	{
		m_serverInfo->m_velocity = m_playerBox->m_maxFreeflight;
	}
	else if (m_serverInfo->m_velocity < -m_playerBox->m_maxFreeflight)
	{
		m_serverInfo->m_velocity = -m_playerBox->m_maxFreeflight;
	}

	if (m_playerActive)
	{
		m_serverInfo->m_idleTime = 0;
	}
	else
	{
		m_serverInfo->m_velocity = 0.0f;
	}

	m_serverInfo->m_position += m_serverInfo->m_direction * (m_frametime->m_frametime * m_serverInfo->m_velocity);
	m_serverInfo->m_lastDirection = m_serverInfo->m_direction;
	
	return;
	*/

	if (m_serverInfo->m_velocity > m_playerBox->m_maxVelocity)
	{
		m_serverInfo->m_velocity = m_playerBox->m_maxVelocity;
	}
	else if (m_serverInfo->m_velocity < -m_playerBox->m_maxVelocity)
	{
		m_serverInfo->m_velocity = -m_playerBox->m_maxVelocity;
	}


	if (m_playerActive)
	{
		m_serverInfo->m_idleTime = 0;
	}
	else
	{
		m_serverInfo->m_velocity = 0.0f;
	}

	CServer::PlayerMovement();
}

/*
*/
void CServer::AdvancePlayerTimers()
{
	m_serverInfo->m_timerReload->Frame(m_frametime->m_totalTime);

	if (m_serverInfo->m_timerReload->m_isReloading)
	{
		m_serverInfo->m_reloadTime += (int32_t)m_frametime->m_totalTime;
	}

	m_serverInfo->m_timerIdle->Frame(m_frametime->m_totalTime);

	if (m_serverInfo->m_timerIdle->m_isReloading)
	{
		m_serverInfo->m_idleTime += (int32_t)m_frametime->m_totalTime;
	}

	/*
	if (m_serverInfo->m_idleTime > 10000)
	{
		CServer::IdleTimeout();
	}
	*/
}

/*
*/
void CServer::AdvanceServerTimers()
{
	m_frametime->Frame();

	m_matchTime->Frame(m_frametime->m_totalTime);

	/*
	if (m_matchTime->m_totalSeconds == 10)
	{
		m_state = CServer::ServerState::E_END_MATCH;
	}
	*/

	m_node = m_serverEnvironment->m_collectables->m_list;

	while ((m_node) && (m_node->m_object))
	{
		m_collectable = (CServerObject*)m_node->m_object;

		if (m_collectable->m_limboTimer->m_isReloading)
		{
			m_collectable->m_limboTimer->Frame(m_frametime->m_totalTime);

			if (!m_collectable->m_limboTimer->m_isReloading)
			{
				CNetwork* n = new CNetwork(CNetwork::ServerEvent::E_SE_TO_CLIENT, CNetwork::ClientEvent::E_CE_UPDATE_COLLECTABLE,
					(void*)m_serverInfo, sizeof(CServerInfo),
					nullptr, 0);

				char isVisible = 't';

				sprintf_s((char*)n->m_data, CNetwork::E_DATA_SIZE, "%s %c", m_collectable->m_name->m_text, isVisible);

				CServer::SendNetwork(n);

				SAFE_DELETE(n);
			}
		}

		m_node = m_node->m_next;
	}
}

/*
*/
void CServer::AwaitingConnection()
{
	if (m_connections != CServerInfo::E_MAX_CLIENTS)
	{
		return;
	}

	for (int32_t i = 0; i < CServerInfo::E_MAX_CLIENTS; i++)
	{
		CServerInfo* serverInfo = (CServerInfo*)m_serverInfos->GetElement(1, i);

		if (!serverInfo->m_isBot)
		{
			CNetwork* n = new CNetwork(CNetwork::ServerEvent::E_SE_TO_CLIENT, CNetwork::ClientEvent::E_CE_LOADING,
				(void*)serverInfo, sizeof(CServerInfo),
				nullptr, 0);

			CServer::SendNetwork(n, serverInfo->m_socket);

			SAFE_DELETE(n);

			for (int32_t ii = 0; ii < CServerInfo::E_MAX_CLIENTS; ii++)
			{
				CServerInfo* serverInfo2 = (CServerInfo*)m_serverInfos->GetElement(1, ii);

				n = new CNetwork(CNetwork::ServerEvent::E_SE_TO_CLIENT, CNetwork::ClientEvent::E_CE_LOAD_PLAYER,
					(void*)serverInfo2, sizeof(CServerInfo),
					(void*)serverInfo2->m_modelName, CServerInfo::E_MODEL_NAME_SIZE);

				CServer::SendNetwork(n, serverInfo->m_socket);

				SAFE_DELETE(n);
			}

			n = new CNetwork(CNetwork::ServerEvent::E_SE_TO_CLIENT, CNetwork::ClientEvent::E_CE_LOAD_ENVIRONMENT,
				(void*)serverInfo, sizeof(CServerInfo),
				(void*)m_currentMap->m_text, (int32_t)m_currentMap->m_length);

			CServer::SendNetwork(n, serverInfo->m_socket);

			SAFE_DELETE(n);
		}
	}

	m_state = CServer::ServerState::E_COUNTDOWN;
}

/*
*/
void CServer::CheckCollectables()
{
	if (m_serverEnvironment->m_collectables == nullptr)
	{
		return;
	}

	m_node = m_serverEnvironment->m_collectables->m_list;

	while ((m_node) && (m_node->m_object))
	{
		m_collectable = (CServerObject*)m_node->m_object;

		if (!m_collectable->m_limboTimer->m_isReloading)
		{
			CVec3f cp = CVec3f(m_collectable->m_position);

			float px = m_serverInfo->m_position.m_p.x - cp.m_p.x;
			float py = m_serverInfo->m_position.m_p.y - cp.m_p.y;
			float pz = m_serverInfo->m_position.m_p.z - cp.m_p.z;

			float length = CVec3f(px, py, pz).Length();

			if (length <= 16.0f)
			{
				m_collectable->m_limboTimer->Start();

				CNetwork* n = new CNetwork(CNetwork::ServerEvent::E_SE_TO_CLIENT, CNetwork::ClientEvent::E_CE_UPDATE_COLLECTABLE,
					(void*)m_serverInfo, sizeof(CServerInfo),
					nullptr, 0);

				CVec3f position = CVec3f(m_collectable->m_position);

				CVec3i index = m_serverEnvironment->m_sector->GetSector(&position);

				char isVisible = 'f';

				sprintf_s((char*)n->m_data, CNetwork::E_DATA_SIZE, "%s %c", m_collectable->m_name->m_text, isVisible);

				CServer::SendNetwork(n);

				SAFE_DELETE(n);
			}
		}

		m_node = m_node->m_next;
	}
}

/*
*/
void CServer::ConsoleMessage()
{
	m_errorLog->WriteError(true, "CServer::ConsoleMessage::%s\n", (char*)m_networkReceive->m_data);

	CNetwork* n = new CNetwork(CNetwork::ServerEvent::E_SE_TO_CLIENT, CNetwork::ClientEvent::E_CE_CONSOLE_MESSAGE,
		(void*)m_serverInfo, sizeof(CServerInfo),
		(void*)m_networkReceive->m_data, (int32_t)strlen((char*)m_networkReceive->m_data));

	CServer::SendNetwork(n);

	SAFE_DELETE(n);
}

/*
*/
void CServer::Countdown()
{
	m_frametime->Frame();

	m_countdownTime->Frame(m_frametime->m_totalTime);

	if (m_countdownTime->m_isReloading)
	{
	}
	else
	{
		m_countdownTime->Start();

		m_countdown--;

		if (m_countdown == 0)
		{
			m_matchTime->Start();

			m_state = CServer::ServerState::E_GAME_RUNNING;

			return;
		}
	}

	for (int32_t i = 0; i < CServerInfo::E_MAX_CLIENTS; i++)
	{
		CServerInfo* serverInfo = (CServerInfo*)m_serverInfos->GetElement(1, i);

		if (serverInfo->m_isRunning)
		{
			CNetwork* n = new CNetwork(CNetwork::ServerEvent::E_SE_TO_CLIENT, CNetwork::ClientEvent::E_CE_SEND_NULL_ACTIVITY,
				(void*)serverInfo, sizeof(CServerInfo),
				nullptr, 0);

			CServer::SendNetwork(n, serverInfo->m_socket);

			SAFE_DELETE(n);
		}
	}

	CServer::ReceiveClients();

	CServer::SendUpdates();
}

/*
*/
void CServer::CreateClient(SOCKET socket)
{
	CServerInfo* acceptServerInfo = new CServerInfo();

	acceptServerInfo->Constructor();

	acceptServerInfo->m_socket->m_socket = socket;

	CNetwork* n = new CNetwork(CNetwork::ServerEvent::E_SE_TO_CLIENT, CNetwork::ClientEvent::E_CE_ACCEPTED,
		(void*)acceptServerInfo, sizeof(CServerInfo),
		nullptr, 0);

	CServer::SendNetwork(n, acceptServerInfo->m_socket);

	SAFE_DELETE(n);

	acceptServerInfo->m_socket->SetReceiveTimeout(50);

	CNetwork network = {};

	acceptServerInfo->m_socket->Receive((char*)&network, sizeof(CNetwork));

	acceptServerInfo->SetClient((CServerInfo*)&network.m_serverInfo);

	CServerInfo* serverInfo = {};

	bool wasReconnect = false;

	bool wasAdded = false;

	for (int32_t i = 0; i < CServerInfo::E_MAX_CLIENTS; i++)
	{
		serverInfo = (CServerInfo*)m_serverInfos->GetElement(1, i);

		if (strcmp(serverInfo->m_playerName, acceptServerInfo->m_playerName) == 0)
		{
			serverInfo->m_socket->Shutdown();

			serverInfo->Initialize(i, socket, false, false, true, false, acceptServerInfo->m_playerName, acceptServerInfo->m_modelName, m_currentMap->m_text);

			serverInfo->m_isConnected = true;

			wasReconnect = true;

			wasAdded = true;

			break;
		}
	}

	if (!wasReconnect)
	{
		for (int32_t i = 0; i < CServerInfo::E_MAX_CLIENTS; i++)
		{
			serverInfo = (CServerInfo*)m_serverInfos->GetElement(1, i);

			if (serverInfo->m_isAvailable)
			{
				serverInfo->Initialize(i, socket, false, false, false, false, acceptServerInfo->m_playerName, acceptServerInfo->m_modelName, m_currentMap->m_text);

				serverInfo->m_isConnected = true;

				wasAdded = true;

				m_connections++;

				break;
			}
		}
	}

	if ((wasAdded) && ((m_state == CServer::ServerState::E_COUNTDOWN) || (m_state == CServer::ServerState::E_GAME_RUNNING)))
	{
		n = new CNetwork(CNetwork::ServerEvent::E_SE_TO_CLIENT, CNetwork::ClientEvent::E_CE_LOADING,
			(void*)serverInfo, sizeof(CServerInfo),
			nullptr, 0);

		CServer::SendNetwork(n, serverInfo->m_socket);

		SAFE_DELETE(n);

		for (int32_t i = 0; i < CServerInfo::E_MAX_CLIENTS; i++)
		{
			CServerInfo* serverInfo2 = (CServerInfo*)m_serverInfos->GetElement(1, i);

			n = new CNetwork(CNetwork::ServerEvent::E_SE_TO_CLIENT, CNetwork::ClientEvent::E_CE_LOAD_PLAYER,
				(void*)serverInfo2, sizeof(CServerInfo),
				(void*)serverInfo2->m_modelName, CServerInfo::E_MODEL_NAME_SIZE);

			CServer::SendNetwork(n, serverInfo->m_socket);

			SAFE_DELETE(n);
		}

		n = new CNetwork(CNetwork::ServerEvent::E_SE_TO_CLIENT, CNetwork::ClientEvent::E_CE_LOAD_ENVIRONMENT,
			(void*)serverInfo, sizeof(CServerInfo),
			(void*)m_currentMap->m_text, (int32_t)m_currentMap->m_length);

		CServer::SendNetwork(n, serverInfo->m_socket);

		SAFE_DELETE(n);

		SAFE_DELETE(acceptServerInfo);

		return;
	}

	if (m_connections == CServerInfo::E_MAX_CLIENTS + 1)
	{
		n = new CNetwork(CNetwork::ServerEvent::E_SE_TO_CLIENT, CNetwork::ClientEvent::E_CE_SERVER_FULL,
			(void*)acceptServerInfo, sizeof(CServerInfo),
			nullptr, 0);

		CServer::SendNetwork(n, acceptServerInfo->m_socket);

		SAFE_DELETE(n);

		SAFE_DELETE(acceptServerInfo);

		m_connections--;
	}
}

/*
*/
void CServer::DestroyClient(CServerInfo* serverInfo)
{
	m_errorLog->WriteError(true, "CServer::DestroyClient:%s\n", serverInfo->m_playerName);

	serverInfo->Reset();

	serverInfo->m_socket->Shutdown();
}

/*
*/
void CServer::Disconnect()
{
	m_errorLog->WriteError(true, "CServer::Disconnect:%s\n", m_serverInfo->m_playerName);

	CString* message = new CString(m_serverInfo->m_playerName);

	message->Append(" disconnected\n");

	CNetwork* n = new CNetwork(CNetwork::ServerEvent::E_SE_TO_CLIENT, CNetwork::ClientEvent::E_CE_CONSOLE_MESSAGE,
		(void*)m_serverInfo, sizeof(CServerInfo),
		(char*)message->m_text, message->m_length);

	CServer::SendNetwork(n);

	SAFE_DELETE(n);
	SAFE_DELETE(message);

	switch (m_serverInfo->m_team)
	{
	case CServerInfo::Team::E_TEAM_RED:
	{
		m_redTeamCount--;

		break;
	}
	case CServerInfo::Team::E_TEAM_BLUE:
	{
		m_blueTeamCount--;

		break;
	}
	}

	CServer::Exit();

	CServer::DestroyClient(m_serverInfo);
}

/*
*/
void CServer::EndMatch()
{
	m_frametime->Frame();

	m_endMatchTime->Frame(m_frametime->m_totalTime);

	if (m_endMatchTime->m_totalSeconds == 10)
	{
		m_matchTime->m_totalSeconds = 0;
		m_endMatchTime->m_totalSeconds = 0;

		m_mapListNode = m_mapListNode->m_next;

		if (m_mapListNode->m_object == nullptr)
		{
			m_mapListNode = m_mapList->m_list;
		}

		m_currentMap = (CString*)m_mapListNode->m_object;

		CServer::MapChange();
	}
}

/*
*/
void CServer::Exit()
{
	m_errorLog->WriteError(true, "CServer::Exit:%s\n", m_serverInfo->m_playerName);

	CNetwork* n = new CNetwork(CNetwork::ServerEvent::E_SE_TO_CLIENT, CNetwork::ClientEvent::E_CE_EXIT,
		(void*)m_serverInfo, sizeof(CServerInfo),
		nullptr, 0);

	CServer::SendNetwork(n);

	SAFE_DELETE(n);
}

/*
*/
void CServer::Frame()
{
	(this->*m_frame[m_state])();
}

/*
*/
void CServer::GameRunning()
{
	CServer::AdvanceServerTimers();

	CServer::PollClients();

	CServer::ReceiveClients();

	CServer::SendUpdates();

	// seconds / fps
	int32_t sleepTime = (1000 / 120) - (int32_t)(m_frametime->m_totalTime);

	if (sleepTime > 0)
	{
		Sleep(sleepTime);
	}
}

/*
*/
void CServer::IdleTimeout()
{
	m_errorLog->WriteError(true, "CServer::IdleTimeout:%s\n", m_serverInfo->m_playerName);

	CServer::Disconnect();
}

/*
*/
void CServer::Inference(CServerInfo* serverInfo)
{
	if (m_state != CServer::ServerState::E_GAME_RUNNING)
	{
		return;
	}

	CBot* bot = &m_bot[serverInfo->m_clientNumber];

	if ((bot->m_state == CBot::State::E_ENTERING) || (bot->m_state == CBot::State::E_IDLE))
	{
		return;
	}

	bot->m_eventState = CBot::EventState::E_REQUESTING;

	bool isRequesting = true;

	while (isRequesting)
	{
		if (!bot->m_isRunning)
		{
			break;
		}

		switch (bot->m_eventState)
		{
		case CBot::EventState::E_REQUESTING:
		{
			break;
		}
		case CBot::EventState::E_AWAITING_SERVER:
		{
			CNetwork n = CNetwork(CNetwork::ClientEvent::E_CE_TO_SERVER, CNetwork::ServerEvent::E_SE_ACTIVITY,
				(void*)bot->m_serverInfo, sizeof(CServerInfo),
				nullptr, 0);

			memcpy(m_networkReceive, &n, sizeof(CNetwork));

			CServer::ProcessEvent();

			bot->m_eventState = CBot::EventState::E_INFERENCING;

			isRequesting = false;

			break;
		}
		}
	}
}

/*
*/
void CServer::InitializeWinsock()
{
	m_winsockStartupResult = WSAStartup(m_winsockVersionRequested, &m_wsaData);

	if (m_winsockStartupResult != 0)
	{
		m_errorLog->WriteError(true, "CServer::InitializeWinsock::WSAStartup:%i\n", m_winsockStartupResult);
	}

	m_errorLog->WriteError(true, "CServer::InitializeWinsock::WSAStartup:%s\n", m_wsaData.szDescription);
}

/*
*/
void CServer::LoadEnvironment()
{
	m_errorLog->WriteError(true, "CServer::LoadEnvironment:%s\n", m_currentMap->m_text);

	SAFE_DELETE(m_serverEnvironment);

	m_serverEnvironment = new CServerEnvironment(m_errorLog, m_currentMap->m_text);

	m_errorLog->WriteError(true, "CServer::LoadEnvironment:Completed\n");
}

/*
*/
void CServer::MapChange()
{
	CServer::Reset();

	CServer::LoadEnvironment();

	CServer::StartBots();

	m_state = CServer::ServerState::E_AWAITING_CONNECTIONS;
}

/*
*/
void CServer::NullActivity()
{

}

/*
*/
void CServer::PlayerMovement()
{
	CVec3f position = m_serverInfo->m_position;

	CVec3f direction = m_serverInfo->m_direction;

	int32_t segments = 4;

	float distance = (m_serverInfo->m_velocity * m_frametime->m_frametime) * (1.0f / (float)segments);

	float freefallVelocity = (m_serverInfo->m_freefallVelocity * m_frametime->m_frametime) * (1.0f / (float)segments);

	for (int32_t i = 0; i < segments; i++)
	{
		m_sectorIndex = m_serverEnvironment->m_sector->GetSector(&position);

		m_serverInfo->m_px = m_sectorIndex.m_p.x;
		m_serverInfo->m_py = m_sectorIndex.m_p.y;
		m_serverInfo->m_pz = m_sectorIndex.m_p.z;

		m_collisionPrimitives = (CList*)m_serverEnvironment->m_collisionPrimitives->GetElement(3, m_sectorIndex.m_p.x, m_sectorIndex.m_p.z, m_sectorIndex.m_p.y);

		if ((m_collisionPrimitives == nullptr) || (m_collisionPrimitives->m_count == 0))
		{
			m_serverInfo->m_position += m_serverInfo->m_direction * (m_serverInfo->m_velocity * m_frametime->m_frametime);

			m_serverInfo->m_lastDirection = m_serverInfo->m_direction;

			return;
		}

		m_playerBox->Reset();

		float nearestDistance = 99999.99f;

		for (int b = 0; b < CPlayerBox::E_MAX_SIDES; b++)
		{
			m_node = m_collisionPrimitives->m_list;

			while ((m_node) && (m_node->m_object))
			{
				m_collisionPrimitive = (CCollisionPrimitive*)m_node->m_object;

				if (m_collision->IntersectPlane(&m_collisionPrimitive->m_n, &m_collisionPrimitive->m_a, &position, &m_playerBox->m_n[b]))
				{
					if ((b == 1) || (m_collision->m_length <= m_playerBox->m_maxCollision))
					{
						m_pointOnPlane = position + (m_playerBox->m_n[b] * m_collision->m_length);

						if (m_collision->RayTriangleIntersect(&m_pointOnPlane, &m_collisionPrimitive->m_a, &m_collisionPrimitive->m_b, &m_collisionPrimitive->m_c))
						{
							if (m_collision->m_length < nearestDistance)
							{
								nearestDistance = m_collision->m_length;
							}

							if (m_collision->m_length < m_playerBox->m_dist[b])
							{
								m_playerBox->m_pop[b] = m_pointOnPlane;

								m_playerBox->m_dist[b] = m_collision->m_length;

								m_playerBox->m_nearest[b] = m_collisionPrimitive;
							}
						}
					}
				}

				m_node = m_node->m_next;
			}
		}

		CPlane p = {};
		CPlane r = {};

		CLine3D l = {};

		if (m_serverInfo->m_isFreefall)
		{
			direction.m_p.y -= freefallVelocity;

			position.m_p.y -= freefallVelocity;
		}

		if (m_playerBox->m_dist[0] < m_playerBox->m_height)
		{
			p.m_normal = m_playerBox->m_nearest[0]->m_n;

			r.m_normal = m_serverInfo->m_right;

			l = p.PlanePlaneIntersection(&r);

			direction.m_p.y = l.m_D.m_p.y;

			position.m_p.y = m_playerBox->m_pop[0].m_p.y - m_playerBox->m_height;
		}

		if (m_playerBox->m_dist[1] < m_playerBox->m_height + 1.0f)
		{
			p.m_normal = m_playerBox->m_nearest[1]->m_n;

			r.m_normal = m_serverInfo->m_right * -1.0f;

			l = p.PlanePlaneIntersection(&r);

			direction.m_p.y = l.m_D.m_p.y;

			position.m_p.y = m_playerBox->m_pop[1].m_p.y + m_playerBox->m_height;

			m_serverInfo->m_surface = m_playerBox->m_nearest[1]->m_surface;

			m_serverInfo->m_groundNormal = m_playerBox->m_nearest[1]->m_n;

			m_serverInfo->m_isFreefall = false;
		}
		else if (m_playerBox->m_dist[1] > m_playerBox->m_height)
		{
			m_serverInfo->m_isFreefall = true;
		}

		for (int32_t c = 2; c < CPlayerBox::E_MAX_SIDES; c++)
		{
			if (m_playerBox->m_dist[c] < m_playerBox->m_width)
			{
				CVec3f d = m_playerBox->m_n[0].Cross(&m_playerBox->m_nearest[c]->m_n);

				d.Normalize();

				if (m_serverInfo->m_direction.Dot(&d) < 0.0f)
				{
					d *= -1.0f;
				}

				direction.m_p.x = d.m_p.x;
				direction.m_p.z = d.m_p.z;

				position.m_p.x += m_playerBox->m_nearest[c]->m_n.m_p.x * (m_playerBox->m_width - m_playerBox->m_dist[c]);
				position.m_p.z += m_playerBox->m_nearest[c]->m_n.m_p.z * (m_playerBox->m_width - m_playerBox->m_dist[c]);

				if (m_serverInfo->m_isFreefall)
				{
					direction.m_p.x = m_serverInfo->m_lastDirection.m_p.x;
					direction.m_p.z = m_serverInfo->m_lastDirection.m_p.z;
				}
			}
		}

		direction.Normalize();

		if (distance < nearestDistance)
		{
			position += (direction * distance);
		}
		else
		{
			position += (direction * nearestDistance);
		}
	}

	if (m_serverInfo->m_isFreefall)
	{
		m_serverInfo->m_freefallVelocity += m_playerBox->m_freefallAcceleration * m_frametime->m_frametime;

		if (m_serverInfo->m_freefallVelocity > m_playerBox->m_terminalVelocity)
		{
			m_serverInfo->m_freefallVelocity = m_playerBox->m_terminalVelocity;
		}
	}
	else
	{
		if (m_serverInfo->m_freefallVelocity >= m_playerBox->m_injurySpeed)
		{
			CNetwork* n = new CNetwork(CNetwork::ServerEvent::E_SE_TO_CLIENT, CNetwork::ClientEvent::E_CE_QUE_SOUND,
				(void*)m_serverInfo, sizeof(CServerInfo),
				(void*)"0", 1);

			CServer::SendNetwork(n);

			SAFE_DELETE(n);
		}

		m_serverInfo->m_freefallVelocity = 0.0f;
	}

	m_serverInfo->m_position = position;

	m_serverInfo->m_lastDirection = direction;
}

/*
*/
void CServer::PollClients()
{
	for (int32_t i = 0; i < CServerInfo::E_MAX_CLIENTS; i++)
	{
		CServerInfo* serverInfo = (CServerInfo*)m_serverInfos->GetElement(1, i);

		if (serverInfo->m_isConnected)
		{
			if (serverInfo->m_isRunning)
			{
				CServer::RequestActivity(serverInfo);
			}
			else
			{
				CNetwork* n = new CNetwork(CNetwork::ServerEvent::E_SE_TO_CLIENT, CNetwork::ClientEvent::E_CE_READY_CHECK,
					(void*)serverInfo, sizeof(CServerInfo),
					nullptr, 0);

				CServer::SendNetwork(n, serverInfo->m_socket);

				SAFE_DELETE(n);
			}
		}
	}
}

/*
*/
void CServer::ProcessEvent()
{
	CServerInfo* serverInfo = (CServerInfo*)m_networkReceive->m_serverInfo;

	m_serverInfo = (CServerInfo*)m_serverInfos->GetElement(1, serverInfo->m_clientNumber);

	m_serverInfo->SetServer(serverInfo);

	if (strlen((const char*)m_networkReceive->m_data) > 0)
	{
		CServer::ConsoleMessage();
	}

	(this->*m_event[m_networkReceive->m_type])();
}

/*
*/
void CServer::Ready()
{
	m_errorLog->WriteError(true, "CServer::Ready:%s\n", m_serverInfo->m_playerName);

	if (!m_serverInfo->m_isReconnect)
	{
		if (m_redTeamCount > m_blueTeamCount)
		{
			m_serverInfo->m_team = CServerInfo::E_TEAM_BLUE;

			m_blueTeamCount++;
		}
		else
		{
			m_serverInfo->m_team = CServerInfo::Team::E_TEAM_RED;

			m_redTeamCount++;
		}

		switch (m_serverInfo->m_team)
		{
		case CServerInfo::Team::E_TEAM_RED:
		{
			m_node = m_serverEnvironment->m_redTeamStarts->m_list;

			while ((m_node) && (m_node->m_object))
			{
				m_playerStart = (CPlayerStart*)m_node->m_object;

				m_serverInfo->m_position = m_playerStart->m_position;
				m_serverInfo->m_direction = m_playerStart->m_direction;
				m_serverInfo->m_lastDirection = m_serverInfo->m_direction;

				m_node = m_node->m_next;
			}

			break;
		}
		case CServerInfo::Team::E_TEAM_BLUE:
		{
			m_node = m_serverEnvironment->m_blueTeamStarts->m_list;

			while ((m_node) && (m_node->m_object))
			{
				m_playerStart = (CPlayerStart*)m_node->m_object;

				m_serverInfo->m_position = m_playerStart->m_position;
				m_serverInfo->m_direction = m_playerStart->m_direction;
				m_serverInfo->m_lastDirection = m_serverInfo->m_direction;

				m_node = m_node->m_next;
			}

			break;
		}
		}
	}

	if (m_serverInfo->m_isBot)
	{
		m_bot[m_serverInfo->m_clientNumber].m_state = CBot::State::E_ENTERING;
	}
	else
	{
		m_serverInfo->m_isRunning = true;
	}

	CNetwork* n = new CNetwork(CNetwork::ServerEvent::E_SE_TO_CLIENT, CNetwork::ClientEvent::E_CE_ENTER,
		(void*)m_serverInfo, sizeof(CServerInfo),
		nullptr, 0);

	CServer::SendNetwork(n);

	SAFE_DELETE(n);
}

/*
*/
void CServer::ReadyCheck()
{

}

/*
*/
void CServer::ReceiveClients()
{
	for (int32_t i = 0; i < CServerInfo::E_MAX_CLIENTS; i++)
	{
		CServerInfo* serverInfo = (CServerInfo*)m_serverInfos->GetElement(1, i);

		if (serverInfo->m_isBot)
		{
			CServer::Inference(serverInfo);
		}
		else if (serverInfo->m_isConnected)
		{
			m_totalBytes = serverInfo->m_socket->Receive((char*)m_networkReceive, sizeof(CNetwork));

			if (m_totalBytes > 0)
			{
				CServer::ProcessEvent();
			}
		}
	}
}

/*
*/
void CServer::RequestActivity(CServerInfo* serverInfo)
{
	CNetwork* n = new CNetwork(CNetwork::ServerEvent::E_SE_TO_CLIENT, CNetwork::ClientEvent::E_CE_SEND_ACTIVITY,
		(void*)serverInfo, sizeof(CServerInfo),
		nullptr, 0);

	CServer::SendNetwork(n, serverInfo->m_socket);

	SAFE_DELETE(n);
}

/*
*/
void CServer::Reset()
{
	CServer::StopBots();

	CServer::ResetClients();

	m_redTeamCount = 0;
	m_blueTeamCount = 0;

	m_countdown = 5;

	m_matchTime->m_totalSeconds = 0;
}

/*
*/
void CServer::ResetClients()
{
	for (int32_t i = 0; i < CServerInfo::E_MAX_CLIENTS; i++)
	{
		CServerInfo* serverInfo = (CServerInfo*)m_serverInfos->GetElement(1, i);

		if (serverInfo->m_isConnected)
		{
			serverInfo->m_freefallVelocity = 0.0f;
			serverInfo->m_velocity = 0.0f;
			serverInfo->m_reloadTime = 0;
			serverInfo->m_idleTime = 0;
			serverInfo->m_surface = 0;

			serverInfo->m_isRunning = false;
			serverInfo->m_isBot = false;

			serverInfo->m_timerReload->m_isReloading = false;
			serverInfo->m_timerIdle->m_isReloading = false;

			serverInfo->m_socket->SetReceiveTimeout(1);

			CNetwork m_network = {};

			while (serverInfo->m_socket->Receive((char*)&m_network, sizeof(CNetwork)) > 0)
			{

			}

			serverInfo->m_socket->SetReceiveTimeout(50);
		}
	}
}

/*
*/
void CServer::SendLoadEnvironment()
{
	for (int32_t i = 0; i < CServerInfo::E_MAX_CLIENTS; i++)
	{
		CServerInfo* serverInfo = (CServerInfo*)m_serverInfos->GetElement(1, i);

		if (serverInfo->m_isConnected)
		{
			CNetwork* n = new CNetwork(CNetwork::ServerEvent::E_SE_TO_CLIENT, CNetwork::ClientEvent::E_CE_LOAD_ENVIRONMENT,
				(void*)serverInfo, sizeof(CServerInfo),
				(void*)m_currentMap->m_text, m_currentMap->m_length);

			CServer::SendNetwork(n, serverInfo->m_socket);

			SAFE_DELETE(n);
		}
	}
}

/*
*/
void CServer::SendNetwork(CNetwork* network)
{
	for (int32_t i = 0; i < CServerInfo::E_MAX_CLIENTS; i++)
	{
		CServerInfo* serverInfo = (CServerInfo*)m_serverInfos->GetElement(1, i);

		if (serverInfo->m_isRunning)
		{
			serverInfo->m_socket->Send((char*)network, sizeof(CNetwork));
		}
	}
}

/*
*/
void CServer::SendNetwork(CNetwork* network, CSocket* socket)
{
	socket->Send((char*)network, sizeof(CNetwork));
}

/*
*/
void CServer::SendUpdates()
{
	for (int32_t i = 0; i < CServerInfo::E_MAX_CLIENTS; i++)
	{
		CServerInfo* serverInfo = (CServerInfo*)m_serverInfos->GetElement(1, i);

		if ((serverInfo->m_isRunning) || (serverInfo->m_isBot))
		{
			serverInfo->m_countdown = m_countdown;

			serverInfo->m_matchTimeSeconds = m_matchTime->m_totalSeconds;

			CNetwork* n = new CNetwork(CNetwork::ServerEvent::E_SE_TO_CLIENT, CNetwork::ClientEvent::E_CE_UPDATE,
				(void*)serverInfo, sizeof(CServerInfo),
				nullptr, 0);

			CServer::SendNetwork(n);

			SAFE_DELETE(n);
		}
	}

	for (int32_t i = 0; i < CServerInfo::E_MAX_CLIENTS; i++)
	{
		CServerInfo* serverInfo = (CServerInfo*)m_serverInfos->GetElement(1, i);

		if (serverInfo->m_isRunning)
		{
			CNetwork* n = new CNetwork(CNetwork::ServerEvent::E_SE_TO_CLIENT, CNetwork::ClientEvent::E_CE_DRAW_FRAME,
				(void*)serverInfo, sizeof(CServerInfo),
				nullptr, 0);

			CServer::SendNetwork(n, serverInfo->m_socket);

			SAFE_DELETE(n);
		}
	}
}

/*
*/
void CServer::ShutdownClients()
{
	m_errorLog->WriteError(true, "CServer::ShutdownClients\n");

	for (int32_t i = 0; i < CServerInfo::E_MAX_CLIENTS; i++)
	{
		CServerInfo* serverInfo = (CServerInfo*)m_serverInfos->GetElement(1, i);

		if (serverInfo->m_isBot)
		{
			m_bot[i].Stop();
		}

		CNetwork* n = new CNetwork(CNetwork::ServerEvent::E_SE_TO_CLIENT, CNetwork::ClientEvent::E_CE_SERVER_SHUTDOWN,
			(void*)serverInfo, sizeof(CServerInfo),
			nullptr, 0);

		CServer::SendNetwork(n);

		SAFE_DELETE(n);

		Sleep(5);

		serverInfo->Clear();
	}
}

/*
*/
void CServer::ShutdownListen()
{
	if (!m_listenThreadRunning)
	{
		m_errorLog->WriteError(true, "CServer::ShutdownListen::ListenThread not running\n");

		return;
	}

	m_errorLog->WriteError(true, "CServer::ShutdownListen\n");

	m_listenThreadRunning = false;

	m_listenSocket->ShutdownListen();

	SAFE_DELETE(m_listenSocket);
}

/*
*/
void CServer::Start(const char* port)
{
	m_errorLog->WriteError(true, "CServer::Start:%s\n", port);

	CServer::InitializeWinsock();

	m_listenSocket = new CSocket(m_errorLog);

	m_listenSocket->CreateListenSocket(port);

	m_listenSocket->Listen();

	m_listenThreadRunning = true;

	m_state = CServer::ServerState::E_AWAITING_CONNECTIONS;

	m_listenThreadHandle = (HANDLE)_beginthreadex(NULL, sizeof(CServer), &CServer::ListenThread, (void*)this, 0, &m_listenThreadId);

	CloseHandle(m_listenThreadHandle);

	m_listenThreadHandle = 0;
}

/*
*/
bool CServer::StartBot(CString* name)
{
	for (int32_t i = 0; i < CServerInfo::E_MAX_CLIENTS; i++)
	{
		CServerInfo* serverInfo = (CServerInfo*)m_serverInfos->GetElement(1, i);

		if (strcmp(serverInfo->m_playerName, name->m_text) == 0)
		{
			m_errorLog->WriteError(true, "CServer::StartBot:Already started:%s\n", name->m_text);

			return false;
		}
	}

	m_errorLog->WriteError(true, "CServer::StartBot:%s\n", name->m_text);

	for (int32_t i = 0; i < CServerInfo::E_MAX_CLIENTS; i++)
	{
		CServerInfo* serverInfo = (CServerInfo*)m_serverInfos->GetElement(1, i);

		if (serverInfo->m_isAvailable)
		{
			serverInfo->Initialize(i, NULL, true, false, false, false, name->m_text, "model/monkey.obj", m_currentMap->m_text);

			m_botCount++;

			m_connections++;

			m_serverInfo = serverInfo;

			m_bot[serverInfo->m_clientNumber].Deconstructor();

			m_bot[serverInfo->m_clientNumber].Constructor(m_errorLog, m_frametime, m_serverInfos, serverInfo);

			m_bot[serverInfo->m_clientNumber].Start();

			CServer::Ready();

			return true;
		}
	}

	m_errorLog->WriteError(true, "CServer::StartBot:At max clients:%s\n", name->m_text);

	return false;
}

/*
*/
void CServer::StartBots()
{
	for (int32_t i = 0; i < CServerInfo::E_MAX_CLIENTS; i++)
	{
		m_serverInfo = (CServerInfo*)m_serverInfos->GetElement(1, i);

		if (m_serverInfo->m_isBot)
		{
			CServer::Ready();
		}
	}
}

/*
*/
void CServer::Stop()
{
	m_errorLog->WriteError(true, "CServer::Stop\n");

	CServer::ShutdownClients();

	CServer::ShutdownListen();

	CServer::Reset();
}

/*
*/
void CServer::StopBot(CString* name)
{
	m_errorLog->WriteError(true, "CServer::StopBot:%s\n", name->m_text);

	int32_t i = 0;

	for (i = 0; i < CServerInfo::E_MAX_CLIENTS; i++)
	{
		CServerInfo* serverInfo = (CServerInfo*)m_serverInfos->GetElement(1, i);

		if (strcmp(serverInfo->m_playerName, name->m_text) == 0)
		{
			m_bot[i].Stop();

			switch (serverInfo->m_team)
			{
			case CServerInfo::Team::E_TEAM_RED:
			{
				m_redTeamCount--;

				break;
			}
			case CServerInfo::Team::E_TEAM_BLUE:
			{
				m_blueTeamCount--;

				break;
			}
			}

			m_connections--;

			CNetwork* n = new CNetwork(CNetwork::ServerEvent::E_SE_TO_CLIENT, CNetwork::ClientEvent::E_CE_EXIT,
				(void*)serverInfo, sizeof(CServerInfo),
				nullptr, 0);

			CServer::SendNetwork(n);

			SAFE_DELETE(n);

			serverInfo->Reset();

			return;
		}
	}

	if (i == CServerInfo::E_MAX_CLIENTS)
	{
		m_errorLog->WriteError(true, "CServer::StopBot:Not found:%s\n", name->m_text);
	}
}

/*
*/
void CServer::StopBots()
{
	for (int32_t i = 0; i < CServerInfo::E_MAX_CLIENTS; i++)
	{
		CServerInfo* serverInfo = (CServerInfo*)m_serverInfos->GetElement(1, i);

		if (serverInfo->m_isBot)
		{
			m_bot[i].SetIdle();

			serverInfo->m_freefallVelocity = 0.0f;
			serverInfo->m_velocity = 0.0f;
			serverInfo->m_reloadTime = 0;
			serverInfo->m_idleTime = 0;
			serverInfo->m_surface = 0;

			serverInfo->m_timerReload->m_isReloading = false;
			serverInfo->m_timerIdle->m_isReloading = false;
		}
	}
}

/*
*/
unsigned __stdcall CServer::ListenThread(void* obj)
{
	CServer* server = (CServer*)obj;

	while (server->m_listenThreadRunning)
	{
		SOCKET socket = server->m_listenSocket->Accept();

		if (socket)
		{
			if (server->m_state != CServer::ServerState::E_END_MATCH)
			{
				server->CreateClient(socket);
			}
			else
			{
				closesocket(socket);
			}
		}
	}

	_endthreadex(0);

	return 0;
}