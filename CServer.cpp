#include "CServer.h"

/*
*/
CServer::CServer()
{
	memset(this, 0x00, sizeof(CServer));

	m_winsockVersionRequested = MAKEWORD(2, 2);

	m_errorLog = new CErrorLog("C:/Users/junk_/source/repos/Game/GameServerLog.txt");

	m_networkReceive = new CNetwork();

	m_frametime = new CFrametime();

	m_collision = new CCollision();

	m_serverInfos = new CHeapArray(sizeof(CServerInfo), true, true, 1, CServerInfo::E_MAX_CLIENTS);

	for (int i = 0; i < CServerInfo::E_MAX_CLIENTS; i++)
	{
		m_clientServerInfo = (CServerInfo*)m_serverInfos->GetElement(1, i);

		m_clientServerInfo->Constructor();

		m_clientServerInfo->m_socket->SetErrorLog(m_errorLog);

		m_clientServerInfo->m_isAvailable = true;
	}

	m_countdownTime = new CTimer(1000);

	m_countdown = 5;

	m_matchTime = new CMatchTime(1000);

	m_endMatchTime = new CMatchTime(1000);

	m_method[CNetwork::ServerEvent::E_SE_ACTIVITY] = &CServer::Activity;
	m_method[CNetwork::ServerEvent::E_SE_CHAT] = &CServer::Chat;
	m_method[CNetwork::ServerEvent::E_SE_DISCONNECT] = &CServer::Disconnect;
	m_method[CNetwork::ServerEvent::E_SE_EXIT] = &CServer::Exit;
	m_method[CNetwork::ServerEvent::E_SE_NULL_ACTIVITY] = &CServer::NullActivity;
	m_method[CNetwork::ServerEvent::E_SE_INFO] = &CServer::Info;
	m_method[CNetwork::ServerEvent::E_SE_READY] = &CServer::Ready;
	m_method[CNetwork::ServerEvent::E_SE_READY_CHECK] = &CServer::ReadyCheck;

	m_frame[CServer::ServerState::E_COUNTDOWN] = &CServer::Countdown;
	m_frame[CServer::ServerState::E_END_MATCH] = &CServer::EndMatch;
	m_frame[CServer::ServerState::E_GAME_RUNNING] = &CServer::GameRunning;
	m_frame[CServer::ServerState::E_AWAITING_CONNECTION] = &CServer::AwaitingConnection;

	m_mapList = new CLinkList<CString>();

	m_err = fopen_s(&m_fMapList, "C:/Users/junk_/source/repos/Game/mapList.txt", "rb");

	if (m_err == 0)
	{
		char mapListName[32] = {};

		fscanf_s(m_fMapList, "%s", &mapListName, 32);

		while (!feof(m_fMapList))
		{
			CString* name = new CString(mapListName);

			m_mapList->Append(name, mapListName);

			memset(mapListName, 0x00, 32);

			fscanf_s(m_fMapList, "%s", &mapListName, 32);
		}

		fclose(m_fMapList);
	}

	m_currentMap = m_mapList->m_list;

	CServer::LoadEnvironment(m_currentMap->m_object);

	m_state = CServer::ServerState::E_AWAITING_CONNECTION;
}

/*
*/
CServer::~CServer()
{
	if (m_listenThreadRunning)
	{
		CServer::Stop();
	}

	delete m_mapList;
	delete m_mapName;
	delete m_matchTime;
	delete m_countdownTime;
	delete m_serverInfos;
	delete m_collision;
	delete m_frametime;
	delete m_networkReceive;
	delete m_errorLog;

	WSACleanup();
}

/*
*/
void CServer::Activity()
{
	CServer::AdvanceTimers();
	CServer::CheckCollectables();

	m_playerActive = false;

	for (int i = 0; i < CNetwork::E_MAX_ACTIVITY; i++)
	{
		switch (m_networkReceive->m_data[i])
		{
		case CNetwork::ClientActivity::E_CA_ATTACK:
		{
			if (!m_serverInfo->m_timerReload->m_isReloading)
			{
				m_serverInfo->m_timerReload->Start();

				m_serverInfo->m_state = 'A';

				m_serverInfo->m_reloadTime = 0;

				m_networkSend = new CNetwork(CNetwork::ServerEvent::E_SE_TO_CLIENT, CNetwork::ClientEvent::E_CE_QUE_SOUND,
					(void*)m_serverInfo, sizeof(CServerInfo),
					(void*)"0", 1);

				CServer::SendNetwork(m_networkSend);

				delete m_networkSend;
			}

			m_playerActive = true;

			m_serverInfo->m_idleTime = 0;

			break;
		}
		case CNetwork::ClientActivity::E_CA_STOP:
		{
			if (m_serverInfo->m_velocity != 0.0f)
			{
				m_serverInfo->m_velocity /= 2.0f;
			}

			m_playerActive = true;

			m_serverInfo->m_idleTime = 0;

			break;
		}
		case CNetwork::ClientActivity::E_CA_FORWARD:
		{
			m_serverInfo->m_velocity += m_frametime->m_frametime * m_serverInfo->m_acceleration;

			m_serverInfo->m_idleTime = 0;

			m_playerActive = true;

			break;
		}
		case CNetwork::ClientActivity::E_CA_BACKWARD:
		{
			m_serverInfo->m_velocity -= m_frametime->m_frametime * m_serverInfo->m_acceleration;

			m_serverInfo->m_idleTime = 0;

			m_playerActive = true;

			break;
		}
		case CNetwork::ClientActivity::E_CA_STEP_LEFT:
		{
			m_serverInfo->m_direction = m_serverInfo->m_right * -1.0f;

			m_serverInfo->m_idleTime = 0;

			m_playerActive = true;

			break;
		}
		case CNetwork::ClientActivity::E_CA_STEP_RIGHT:
		{
			m_serverInfo->m_direction = m_serverInfo->m_right;

			m_serverInfo->m_idleTime = 0;

			m_playerActive = true;

			break;
		}
		default:
		{
			break;
		}
		}
	}

	if (!m_playerActive)
	{
		m_serverInfo->m_velocity /= (1.0f + m_frametime->m_frametime);

		if ((m_serverInfo->m_velocity > -1.0f) && ((m_serverInfo->m_velocity < 1.0f)))
		{
			m_serverInfo->m_velocity = 0.0f;
		}

		if (!m_serverInfo->m_timerIdle->m_isReloading)
		{
			m_serverInfo->m_timerIdle->Start();
		}
	}
	else
	{
		m_serverInfo->m_idleTime = 0;
	}

	m_serverInfo->m_direction.Normalize();

	if (m_serverInfo->m_velocity > 512.0f)
	{
		m_serverInfo->m_velocity = 512.0f;
	}
	else if (m_serverInfo->m_velocity < -512.0f)
	{
		m_serverInfo->m_velocity = -512.0f;
	}

	//m_serverInfo->m_position += m_serverInfo->m_direction * (m_frametime->m_frametime * m_serverInfo->m_velocity);
	//m_serverInfo->m_lastDirection = m_serverInfo->m_direction;
	//return;

	/*
	Ground Collision

	B  E-F
	|\  \|
	A-C  D

	BAC is N1
	DFE is N2
	*/

	m_sectorIndex = m_serverEnvironment->m_sector->GetSector(&m_serverInfo->m_position);

	m_serverInfo->m_px = m_sectorIndex.m_p.x;
	m_serverInfo->m_py = m_sectorIndex.m_p.y;
	m_serverInfo->m_pz = m_sectorIndex.m_p.z;

	m_collisionPrimitives = (CLinkList<CCollisionPrimitive>*)m_serverEnvironment->m_collisionPrimitives->GetElement(3, m_sectorIndex.m_p.x, m_sectorIndex.m_p.y, m_sectorIndex.m_p.z);

	if (m_collisionPrimitives == nullptr)
	{
		m_serverInfo->m_position += m_serverInfo->m_direction * (m_frametime->m_frametime * m_serverInfo->m_velocity);

		m_serverInfo->m_lastDirection = m_serverInfo->m_direction;

		return;
	}

	if (m_collisionPrimitives->m_count == 0)
	{
		m_serverInfo->m_position += m_serverInfo->m_direction * (m_frametime->m_frametime * m_serverInfo->m_velocity);

		m_serverInfo->m_lastDirection = m_serverInfo->m_direction;

		return;
	}

	if (m_serverInfo->m_isFreefall)
	{
		m_serverInfo->m_freefallVelocity += m_frametime->m_frametime * 5.0f;
	}

	m_collisionPrimitive = m_collisionPrimitives->m_list;

	CVec3f m_down = { 0.0f, -1.0f, 0.0f };

	while (m_collisionPrimitive->m_object)
	{
		if (m_collision->IntersectPlane(&m_collisionPrimitive->m_object->m_n1, &m_collisionPrimitive->m_object->m_a, &m_serverInfo->m_position, &m_down))
		{
			m_pointOnPlane = CVec3f(m_serverInfo->m_position.m_p.x, m_serverInfo->m_position.m_p.y, m_serverInfo->m_position.m_p.z);

			m_pointOnPlane.m_p.y -= m_collision->m_length;

			if (m_collision->RayTriangleIntersect(&m_pointOnPlane, &m_collisionPrimitive->m_object->m_a, &m_collisionPrimitive->m_object->m_b, &m_collisionPrimitive->m_object->m_c))
			{
				if ((m_collision->m_length >= 0.0f) && (m_collision->m_length <= 2.0f))
				{
					m_serverInfo->m_position.m_p.y = m_pointOnPlane.m_p.y + 2.0f;

					if (m_serverInfo->m_freefallVelocity > 2.0f)
					{
						m_networkSend = new CNetwork(CNetwork::ServerEvent::E_SE_TO_CLIENT, CNetwork::ClientEvent::E_CE_QUE_SOUND,
							(void*)m_serverInfo, sizeof(CServerInfo),
							(void*)"1", 1);

						CServer::SendNetwork(m_networkSend);

						delete m_networkSend;
					}

					m_serverInfo->m_isFreefall = false;

					m_serverInfo->m_freefallVelocity = 0.0f;

					break;
				}
				else
				{
					m_serverInfo->m_isFreefall = true;
				}
			}
		}

		m_collisionPrimitive = m_collisionPrimitive->m_next;
	}

	m_serverInfo->m_lastDirection = m_serverInfo->m_direction;

	m_serverInfo->m_position += m_serverInfo->m_direction * (m_frametime->m_frametime * m_serverInfo->m_velocity);
}

/*
*/
void CServer::AdvanceTimers()
{
	m_serverInfo->m_timerReload->Frame(m_frametime->m_totalTime);

	if (m_serverInfo->m_timerReload->m_isReloading)
	{
		m_serverInfo->m_reloadTime += (int)m_frametime->m_totalTime;
	}

	m_serverInfo->m_timerIdle->Frame(m_frametime->m_totalTime);

	if (m_serverInfo->m_timerIdle->m_isReloading)
	{
		m_serverInfo->m_idleTime += (int)m_frametime->m_totalTime;
	}

	if (m_serverInfo->m_idleTime > 10000)
	{
		//CServer::IdleTimeout();
	}
}

/*
*/
void CServer::AwaitingConnection()
{
	CServer::ReceiveClients();
}

/*
*/
void CServer::Chat()
{
	m_networkSend = new CNetwork(CNetwork::ServerEvent::E_SE_TO_CLIENT, CNetwork::ClientEvent::E_CE_CHAT,
		(void*)m_serverInfo, sizeof(CServerInfo),
		(void*)m_networkReceive->m_data, (int)strlen((char*)m_networkReceive->m_data));

	CServer::SendNetwork(m_networkSend);

	m_errorLog->WriteError(true, "CServer::Chat::%s\n", (char*)m_networkReceive->m_data);

	delete m_networkSend;
}

/*
*/
void CServer::CheckCollectables()
{
	if (m_serverEnvironment->m_sectorCollectables == nullptr)
	{
		return;
	}

	m_sectorIndex = m_serverEnvironment->m_sector->GetSector(&m_serverInfo->m_position);

	m_collectables = (CLinkList<CServerObject>*)m_serverEnvironment->m_sectorCollectables->GetElement(2, m_sectorIndex.m_p.x, m_sectorIndex.m_p.z);

	if (m_collectables != nullptr)
	{
		if (m_collectables->m_count > 0)
		{
			m_collectable = m_collectables->m_list;

			while (m_collectable->m_object)
			{
				if (!m_collectable->m_object->m_limboTimer->m_isReloading)
				{
					CVec3f cp = CVec3f(m_collectable->m_object->m_position);
					
					float px = m_serverInfo->m_position.m_p.x - cp.m_p.x;
					float py = m_serverInfo->m_position.m_p.y - cp.m_p.y;
					float pz = m_serverInfo->m_position.m_p.z - cp.m_p.z;

					float length = CVec3f(px, py, pz).Length();

					if (length <= 8.0f)
					{
						m_collectable->m_object->m_limboTimer->Start();

						m_networkSend = new CNetwork(CNetwork::ServerEvent::E_SE_TO_CLIENT, CNetwork::ClientEvent::E_CE_UPDATE_COLLECTABLE,
							(void*)m_serverInfo, sizeof(CServerInfo),
							nullptr, 0);

						CVec3f position = CVec3f(m_collectable->m_object->m_position);

						CVec3i index = m_serverEnvironment->m_sector->GetSector(&position);

						sprintf_s((char*)m_networkSend->m_data, CNetwork::E_DATA_SIZE, "%s %i %i f", m_collectable->m_object->m_name->m_text, index.m_p.x, index.m_p.z);

						CServer::SendNetwork(m_networkSend);

						delete m_networkSend;
					}
				}

				m_collectable = m_collectable->m_next;
			}
		}
	}
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

	for (int i = 0; i < CServerInfo::E_MAX_CLIENTS; i++)
	{
		m_clientServerInfo = (CServerInfo*)m_serverInfos->GetElement(1, i);

		if (m_clientServerInfo->m_isRunning)
		{
			m_networkSend = new CNetwork(CNetwork::ServerEvent::E_SE_TO_CLIENT, CNetwork::ClientEvent::E_CE_SEND_NULL_ACTIVITY,
				(void*)m_clientServerInfo, sizeof(CServerInfo),
				nullptr, 0);

			CServer::SendNetwork(m_networkSend, m_clientServerInfo->m_socket);

			delete m_networkSend;
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

	delete n;

	acceptServerInfo->m_socket->SetReceiveTimeout(5000);

	CNetwork network = {};

	int totalBytes = acceptServerInfo->m_socket->Receive((char*)&network, sizeof(CNetwork));

	delete acceptServerInfo;

	if (totalBytes <= 0)
	{
		return;
	}

	acceptServerInfo = (CServerInfo*)&network.m_serverInfo;

	CServerInfo* serverInfo = {};

	bool wasReconnect = false;

	for (int i = 0; i < CServerInfo::E_MAX_CLIENTS; i++)
	{
		serverInfo = (CServerInfo*)m_serverInfos->GetElement(1, i);

		if (strcmp(serverInfo->m_playerName, acceptServerInfo->m_playerName) == 0)
		{
			serverInfo->m_socket->Shutdown();

			serverInfo->m_socket->m_socket = socket;

			serverInfo->m_isAvailable = false;
			serverInfo->m_isReconnect = true;
			serverInfo->m_isRunning = false;

			serverInfo->SetModelName(acceptServerInfo->m_modelName);

			if (strncmp(serverInfo->m_lastMap, m_mapName->m_text, m_mapName->m_length) != 0)
			{
				serverInfo->m_isReconnect = false;

				serverInfo->SetLastMapName(m_mapName->m_text);
			}

			serverInfo->m_clientNumber = i;

			wasReconnect = true;

			break;
		}
	}

	if (!wasReconnect)
	{
		for (int i = 0; i < CServerInfo::E_MAX_CLIENTS; i++)
		{
			serverInfo = (CServerInfo*)m_serverInfos->GetElement(1, i);

			if (serverInfo->m_isAvailable)
			{
				serverInfo->m_socket->m_socket = socket;

				serverInfo->m_isAvailable = false;
				serverInfo->m_isReconnect = false;
				serverInfo->m_isRunning = false;

				serverInfo->SetPlayerName(acceptServerInfo->m_playerName);
				serverInfo->SetModelName(acceptServerInfo->m_modelName);
				serverInfo->SetLastMapName(m_mapName->m_text);

				serverInfo->m_clientNumber = i;

				break;
			}
		}
	}

	n = new CNetwork(CNetwork::ServerEvent::E_SE_TO_CLIENT, CNetwork::ClientEvent::E_CE_LOAD_ENVIRONMENT,
		(void*)serverInfo, sizeof(CServerInfo),
		(void*)m_mapName->m_text, (int)m_mapName->m_length);

	CServer::SendNetwork(n, serverInfo->m_socket);

	delete n;

	serverInfo->m_isConnected = true;
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

	m_networkSend = new CNetwork(CNetwork::ServerEvent::E_SE_TO_CLIENT, CNetwork::ClientEvent::E_CE_CHAT,
		(void*)m_serverInfo, sizeof(CServerInfo),
		(char*)message->m_text, message->m_length);

	CServer::SendNetwork(m_networkSend);

	delete m_networkSend;

	delete message;

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

		if (m_currentMap->m_next == nullptr)
		{
			m_currentMap = m_mapList->m_list;
		}
		else
		{
			m_currentMap = m_currentMap->m_next;

			if (m_currentMap->m_object == nullptr)
			{
				m_currentMap = m_mapList->m_list;
			}
		}

		CServer::MapChange(m_currentMap->m_object);
	}
}

/*
*/
void CServer::Exit()
{
	m_errorLog->WriteError(true, "CServer::Exit:%s\n", m_serverInfo->m_playerName);

	m_networkSend = new CNetwork(CNetwork::ServerEvent::E_SE_TO_CLIENT, CNetwork::ClientEvent::E_CE_EXIT,
		(void*)m_serverInfo, sizeof(CServerInfo),
		nullptr, 0);

	CServer::SendNetwork(m_networkSend);

	delete m_networkSend;
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
	m_frametime->Frame();

	m_matchTime->Frame(m_frametime->m_totalTime);

	CServer::PollClients();
	CServer::ReceiveClients();

	if (m_serverEnvironment->m_sectorCollectables != nullptr)
	{
		for (UINT z = 0; z < m_serverEnvironment->m_sector->m_gridHeight; z++)
		{
			for (UINT x = 0; x < m_serverEnvironment->m_sector->m_gridWidth; x++)
			{
				m_collectables = (CLinkList<CServerObject>*)m_serverEnvironment->m_sectorCollectables->GetElement(2, x, z);

				if (m_collectables != nullptr)
				{
					if (m_collectables->m_count > 0)
					{
						m_collectable = m_collectables->m_list;

						while (m_collectable->m_object)
						{
							if (m_collectable->m_object->m_limboTimer->m_isReloading)
							{
								m_collectable->m_object->m_limboTimer->Frame(m_frametime->m_totalTime);

								if (!m_collectable->m_object->m_limboTimer->m_isReloading)
								{
									m_networkSend = new CNetwork(CNetwork::ServerEvent::E_SE_TO_CLIENT, CNetwork::ClientEvent::E_CE_UPDATE_COLLECTABLE,
										(void*)m_serverInfo, sizeof(CServerInfo),
										nullptr, 0);

									sprintf_s((char*)m_networkSend->m_data, CNetwork::E_DATA_SIZE, "%s %i %i t", m_collectable->m_object->m_name->m_text, x, z);

									CServer::SendNetwork(m_networkSend);

									delete m_networkSend;
								}
							}

							m_collectable = m_collectable->m_next;
						}
					}
				}
			}
		}
	}

	CServer::SendUpdates();

	if (m_matchTime->m_totalSeconds == 10)
	{
		//m_state = CServer::ServerState::E_END_MATCH;
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
void CServer::Info()
{
	m_errorLog->WriteError(true, "CServer::Info:%s\n", m_serverInfo->m_playerName);

	for (int i = 0; i < CServerInfo::E_MAX_CLIENTS; i++)
	{
		m_clientServerInfo = (CServerInfo*)m_serverInfos->GetElement(1, i);

		m_networkSend = new CNetwork(CNetwork::ServerEvent::E_SE_TO_CLIENT, CNetwork::ClientEvent::E_CE_INFO,
			(void*)m_clientServerInfo, sizeof(CServerInfo),
			(void*)m_clientServerInfo->m_playerName, (int)strlen(m_clientServerInfo->m_playerName));

		if (m_clientServerInfo->m_clientNumber != m_serverInfo->m_clientNumber)
		{
			CServer::SendNetwork(m_networkSend, m_serverInfo->m_socket);
		}

		delete m_networkSend;
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
void CServer::LoadEnvironment(CString* mapName)
{
	m_mapName = new CString(mapName->m_text);

	m_errorLog->WriteError(true, "CServer::LoadEnvironment:%s\n", m_mapName->m_text);

	m_serverEnvironment = new CServerEnvironment(m_errorLog, mapName->m_text);

	m_errorLog->WriteError(true, "CServer::LoadEnvironment:Completed\n");
}

/*
*/
void CServer::MapChange(CString* mapName)
{
	CServer::Reset();

	CServer::SendClientsLoadEnvironment(mapName);

	CServer::LoadEnvironment(mapName);
}

/*
*/
void CServer::NullActivity()
{

}

/*
*/
void CServer::PollClients()
{
	for (int i = 0; i < CServerInfo::E_MAX_CLIENTS; i++)
	{
		m_clientServerInfo = (CServerInfo*)m_serverInfos->GetElement(1, i);

		if (m_clientServerInfo->m_isConnected)
		{
			if (m_clientServerInfo->m_isRunning)
			{
				CServer::RequestActivity(m_clientServerInfo);
			}
			else
			{
				m_networkSend = new CNetwork(CNetwork::ServerEvent::E_SE_TO_CLIENT, CNetwork::ClientEvent::E_CE_READY_CHECK,
					(void*)m_clientServerInfo, sizeof(CServerInfo),
					nullptr, 0);

				CServer::SendNetwork(m_networkSend, m_clientServerInfo->m_socket);

				delete m_networkSend;
			}
		}
	}
}

/*
*/
void CServer::ProcessEvent()
{
	m_clientServerInfo = (CServerInfo*)m_networkReceive->m_serverInfo;

	m_serverInfo = (CServerInfo*)m_serverInfos->GetElement(1, m_clientServerInfo->m_clientNumber);

	m_serverInfo->SetServer(m_clientServerInfo);

	if (strlen(m_serverInfo->m_chat) > 0)
	{
		m_networkSend = new CNetwork(CNetwork::ServerEvent::E_SE_TO_CLIENT, CNetwork::ClientEvent::E_CE_CHAT,
			(void*)m_serverInfo, sizeof(CServerInfo),
			(void*)m_serverInfo->m_chat, (int)strlen(m_serverInfo->m_chat));

		CServer::SendNetwork(m_networkSend);

		m_serverInfo->SetChat("");

		delete m_networkSend;
	}

	(this->*m_method[m_networkReceive->m_type])();
}

/*
*/
void CServer::Ready()
{
	m_errorLog->WriteError(true, "CServer::Ready:%s\n", m_serverInfo->m_playerName);

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
		m_playerStart = m_serverEnvironment->m_redTeamStarts->m_list;

		while (m_playerStart->m_object)
		{
			m_playerStart->m_object->m_timer->Frame(m_frametime->m_totalTime);

			if (!m_playerStart->m_object->m_timer->m_isReloading)
			{
				m_serverInfo->m_position = m_playerStart->m_object->m_position;
				m_serverInfo->m_direction = m_playerStart->m_object->m_direction;

				m_playerStart->m_object->m_timer->Start();

				break;
			}

			m_playerStart = m_playerStart->m_next;
		}

		break;
	}
	case CServerInfo::Team::E_TEAM_BLUE:
	{
		m_playerStart = m_serverEnvironment->m_blueTeamStarts->m_list;

		while (m_playerStart->m_object)
		{
			m_playerStart->m_object->m_timer->Frame(m_frametime->m_totalTime);

			if (!m_playerStart->m_object->m_timer->m_isReloading)
			{
				m_serverInfo->m_position = m_playerStart->m_object->m_position;
				m_serverInfo->m_direction = m_playerStart->m_object->m_direction;

				m_playerStart->m_object->m_timer->Start();

				break;
			}

			m_playerStart = m_playerStart->m_next;
		}

		break;
	}
	}

	m_serverInfo->m_isRunning = true;

	m_networkSend = new CNetwork(CNetwork::ServerEvent::E_SE_TO_CLIENT, CNetwork::ClientEvent::E_CE_ENTER,
		(void*)m_serverInfo, sizeof(CServerInfo),
		nullptr, 0);

	CServer::SendNetwork(m_networkSend);

	delete m_networkSend;

	if (m_state == CServer::ServerState::E_AWAITING_CONNECTION)
	{
		m_state = CServer::ServerState::E_COUNTDOWN;
	}
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
	for (int i = 0; i < CServerInfo::E_MAX_CLIENTS; i++)
	{
		m_clientServerInfo = (CServerInfo*)m_serverInfos->GetElement(1, i);

		if (m_clientServerInfo->m_isConnected)
		{
			m_totalBytes = m_clientServerInfo->m_socket->Receive((char*)m_networkReceive, sizeof(CNetwork));

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
	m_networkSend = new CNetwork(CNetwork::ServerEvent::E_SE_TO_CLIENT, CNetwork::ClientEvent::E_CE_SEND_ACTIVITY,
		(void*)serverInfo, sizeof(CServerInfo),
		nullptr, 0);

	CServer::SendNetwork(m_networkSend, serverInfo->m_socket);

	delete m_networkSend;
}

/*
*/
void CServer::Reset()
{
	if (m_serverEnvironment)
	{
		delete m_mapName;

		m_mapName = nullptr;

		delete m_serverEnvironment;

		m_serverEnvironment = nullptr;
	}

	CServer::ResetClients();

	m_redTeamCount = 0;
	m_blueTeamCount = 0;

	m_countdown = 5;

	m_matchTime->m_totalSeconds = 0;

	m_state = CServer::ServerState::E_AWAITING_CONNECTION;
}

/*
*/
void CServer::ResetClients()
{
	CNetwork network = {};

	for (int i = 0; i < CServerInfo::E_MAX_CLIENTS; i++)
	{
		m_clientServerInfo = (CServerInfo*)m_serverInfos->GetElement(1, i);

		if (m_clientServerInfo->m_isConnected)
		{
			m_clientServerInfo->m_freefallVelocity = 0.0f;
			m_clientServerInfo->m_velocity = 0.0f;
			m_clientServerInfo->m_reloadTime = 0;
			m_clientServerInfo->m_idleTime = 0;

			m_clientServerInfo->m_isRunning = false;

			m_clientServerInfo->m_timerReload->m_isReloading = false;
			m_clientServerInfo->m_timerIdle->m_isReloading = false;

			m_clientServerInfo->m_socket->SetReceiveTimeout(1);

			while (m_clientServerInfo->m_socket->Receive((char*)&network, sizeof(CNetwork)) > 0)
			{

			}

			m_clientServerInfo->m_socket->SetReceiveTimeout(5000);
		}
	}
}

/*
*/
void CServer::SendClientsLoadEnvironment(CString* mapName)
{
	for (int i = 0; i < CServerInfo::E_MAX_CLIENTS; i++)
	{
		m_clientServerInfo = (CServerInfo*)m_serverInfos->GetElement(1, i);

		if (m_clientServerInfo->m_isConnected)
		{
			m_networkSend = new CNetwork(CNetwork::ServerEvent::E_SE_TO_CLIENT, CNetwork::ClientEvent::E_CE_LOAD_ENVIRONMENT,
				(void*)m_clientServerInfo, sizeof(CServerInfo),
				(void*)mapName->m_text, (int)mapName->m_length);

			CServer::SendNetwork(m_networkSend, m_clientServerInfo->m_socket);

			delete m_networkSend;
		}
	}
}

/*
*/
void CServer::SendNetwork(CNetwork* network)
{
	for (int i = 0; i < CServerInfo::E_MAX_CLIENTS; i++)
	{
		m_clientServerInfo = (CServerInfo*)m_serverInfos->GetElement(1, i);

		if (m_clientServerInfo->m_isRunning)
		{
			m_clientServerInfo->m_socket->Send((char*)network, sizeof(CNetwork));
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
	for (int i = 0; i < CServerInfo::E_MAX_CLIENTS; i++)
	{
		m_clientServerInfo = (CServerInfo*)m_serverInfos->GetElement(1, i);

		if (m_clientServerInfo->m_isRunning)
		{
			m_networkSend = new CNetwork(CNetwork::ServerEvent::E_SE_TO_CLIENT, CNetwork::ClientEvent::E_CE_UPDATE,
				(void*)m_clientServerInfo, sizeof(CServerInfo),
				nullptr, 0);

			CServer::SendNetwork(m_networkSend);

			delete m_networkSend;
		}
	}

	for (int i = 0; i < CServerInfo::E_MAX_CLIENTS; i++)
	{
		m_clientServerInfo = (CServerInfo*)m_serverInfos->GetElement(1, i);

		if (m_clientServerInfo->m_isRunning)
		{
			m_clientServerInfo->m_countdown = m_countdown;

			m_clientServerInfo->m_matchTimeSeconds = m_matchTime->m_totalSeconds;

			m_networkSend = new CNetwork(CNetwork::ServerEvent::E_SE_TO_CLIENT, CNetwork::ClientEvent::E_CE_DRAW_FRAME,
				(void*)m_clientServerInfo, sizeof(CServerInfo),
				nullptr, 0);

			CServer::SendNetwork(m_networkSend, m_clientServerInfo->m_socket);

			delete m_networkSend;
		}
	}
}

/*
*/
void CServer::ShutdownClients()
{
	m_errorLog->WriteError(true, "CServer::ShutdownClients\n");

	for (int i = 0; i < CServerInfo::E_MAX_CLIENTS; i++)
	{
		m_clientServerInfo = (CServerInfo*)m_serverInfos->GetElement(1, i);

		m_clientServerInfo->Clear();
	}
}

/*
*/
void CServer::ShutdownListen()
{
	m_errorLog->WriteError(true, "CServer::ShutdownListen\n");

	if (!m_listenThreadRunning)
	{
		m_errorLog->WriteError(true, "CServer::ShutdownListen::ListenThread not running\n");

		return;
	}

	m_listenThreadRunning = false;

	m_listenSocket->ShutdownListen();

	m_errorLog->WriteError(true, "CServer::ShutdownListen::ListenThread Ending\n");

	WaitForSingleObject(m_listenThreadHandle, INFINITE);

	CloseHandle(m_listenThreadHandle);

	m_listenThreadHandle = 0;

	delete m_listenSocket;

	m_listenSocket = nullptr;
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

	m_state = CServer::ServerState::E_AWAITING_CONNECTION;

	m_errorLog->WriteError(true, "CServer::ListenThread Starting\n");

	m_listenThreadHandle = (HANDLE)_beginthreadex(NULL, sizeof(CServer),
		&CServer::ListenThread,
		(void*)this,
		0,
		&m_listenThreadId);
}

/*
*/
void CServer::Stop()
{
	m_errorLog->WriteError(true, "CServer::Stop\n");

	CServer::ShutdownClients();
	CServer::ShutdownListen();

	m_countdown = 5;

	delete m_serverEnvironment;

	m_serverEnvironment = nullptr;

	m_blueTeamCount = 0;
	m_redTeamCount = 0;
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