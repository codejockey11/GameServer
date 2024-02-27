#include "CServer.h"

unsigned int __stdcall CServer_AccountThread(void* obj);
unsigned int __stdcall CServer_ListenThread(void* obj);

void ProcessMETARNodes(CServer* server, CServerInfo* serverInfo, CComPtr<IXMLDOMNodeList> nodeList);
void ProcessMETARNode(CServer* server, CServerInfo* serverInfo, CComPtr<IXMLDOMNodeList> nodeList);

void CServer_AccountInfo(CServer* server, CNetwork* network, CServerInfo* serverInfo);
void CServer_Activity(CServer* server, CNetwork* network, CServerInfo* serverInfo);
void CServer_Chat(CServer* server, CNetwork* network, CServerInfo* serverInfo);
void CServer_Disconnect(CServer* server, CNetwork* network, CServerInfo* serverInfo);
void CServer_Idle(CServer* server, CNetwork* network, CServerInfo* serverInfo);
void CServer_Info(CServer* server, CNetwork* network, CServerInfo* serverInfo);
void CServer_Join(CServer* server, CNetwork* network, CServerInfo* serverInfo);
void CServer_LoadEnvironment(CServer* server, CNetwork* network, CServerInfo* serverInfo);
void CServer_Ready(CServer* server, CNetwork* network, CServerInfo* serverInfo);

/*
*/
CServer::CServer()
{
	memset(this, 0x00, sizeof(CServer));

	m_errorLog = new CErrorLog("serverLog.txt");

	m_network = new CNetwork();

	m_frametime = new CFrametime();

	m_collision = new CCollision(m_frametime);

	m_serverInfo = new CHeapArray(sizeof(CServerInfo), true, true, 1, CServerInfo::E_MAX_CLIENTS);

	for (int i = 0; i < CServerInfo::E_MAX_CLIENTS; i++)
	{
		CServerInfo* serverInfo = (CServerInfo*)m_serverInfo->GetElement(1, i);

		serverInfo->m_isAvailable = true;
	}

	m_timerCountdown = new CTimer(1000);
	m_countdown = 5;


	pFunc[CNetwork::ServerEvent::E_SE_ACCOUNT_INFO] = &CServer_AccountInfo;
	pFunc[CNetwork::ServerEvent::E_SE_CHAT] = &CServer_Chat;
	pFunc[CNetwork::ServerEvent::E_SE_DISCONNECT] = &CServer_Disconnect;
	pFunc[CNetwork::ServerEvent::E_SE_LOAD_ENVIRONMENT] = &CServer_LoadEnvironment;
	pFunc[CNetwork::ServerEvent::E_SE_ACTIVITY] = &CServer_Activity;
	pFunc[CNetwork::ServerEvent::E_SE_IDLE] = &CServer_Idle;
	pFunc[CNetwork::ServerEvent::E_SE_INFO] = &CServer_Info;
	pFunc[CNetwork::ServerEvent::E_SE_JOIN] = &CServer_Join;
	pFunc[CNetwork::ServerEvent::E_SE_READY] = &CServer_Ready;
}

/*
*/
CServer::~CServer()
{
	CServer::Shutdown();

	delete m_timerCountdown;
	delete m_serverInfo;
	delete m_collision;
	delete m_frametime;
	delete m_network;
	delete m_errorLog;
}

/*
*/
void CServer::Accept()
{
	SOCKET tempSocket = accept(m_listenSocket, m_addrResult->ai_addr, (int*)&m_addrResult->ai_addrlen);

	if (tempSocket == SOCKET_ERROR)
	{
		m_errorLog->WriteWinsockErrorMessage(true, "CServer::Accept::accept:SOCKET_ERROR:");

		m_listenThreadRunning = false;
	}
	else if (tempSocket == INVALID_SOCKET)
	{
		m_errorLog->WriteWinsockErrorMessage(true, "CServer::Accept::accept:INVALID_SOCKET:");

		m_listenThreadRunning = false;
	}
	else
	{
		CServer::CreateClient(nullptr, tempSocket);
	}
}

/*
*/
void CServer::CreateClient(CNetwork* network, SOCKET tempSocket)
{
	// Making a new server info object
	CServerInfo* serverInfo = {};

	int i = 0;

	for (; i < CServerInfo::E_MAX_CLIENTS; i++)
	{
		serverInfo = (CServerInfo*)m_serverInfo->GetElement(1, i);

		if (serverInfo->m_isAvailable)
		{
			serverInfo->m_clientNumber = i;

			if (serverInfo->m_socket != 0)
			{
				serverInfo->Shutdown();
			}

			// The recv socket
			serverInfo->m_socket = tempSocket;

			serverInfo->m_isAvailable = false;

			if (m_timerReload[i] == nullptr)
			{
				m_timerReload[i] = new CTimer(500);
			}

			serverInfo->Constructor(m_errorLog);

			break;
		}
	}

	if (i == CServerInfo::E_MAX_CLIENTS)
	{
		// Let the client know that the server is full
		CNetwork* n = new CNetwork(CNetwork::ServerEvent::E_SE_TO_CLIENT, CNetwork::ClientEvent::E_CE_SERVER_FULL,
			nullptr, 0,
			nullptr);

		CServer::SendNetwork(n, tempSocket);

		delete n;


		int result = shutdown(tempSocket, SD_BOTH);

		if (result != 0)
		{
			m_errorLog->WriteWinsockErrorMessage(true, "CServer::CreateClient:tempSocket:");
		}

		closesocket(tempSocket);

		return;
	}


	// Let the client know that their connection is established
	CNetwork* n = new CNetwork(CNetwork::ServerEvent::E_SE_TO_CLIENT, CNetwork::ClientEvent::E_CE_ACCEPTED,
		nullptr, 0,
		(void*)serverInfo);

	CServer::SendNetwork(n, serverInfo->m_socket);

	delete n;


	// Wait for the client to send their login info
	// Timeout this recv in case the client gets lost
	DWORD millis = 250;
	int iResult = setsockopt(serverInfo->m_socket, SOL_SOCKET, SO_RCVTIMEO, (char*)&millis, (int)sizeof(DWORD));

	int totalBytes = recv(serverInfo->m_socket, (char*)m_network, sizeof(CNetwork), 0);

	if (totalBytes == SOCKET_ERROR)
	{
		m_errorLog->WriteWinsockErrorMessage(true, "CServer::CreateClient::recv:");

		return;
	}


	serverInfo->SetName((char*)m_network->m_data);

	// This client is active on the server
	serverInfo->m_isRunning = true;

	// Start the client in the lobby
	serverInfo->m_state = CServerInfo::E_LOBBY;


	// Let everyone know that this client is active
	n = new CNetwork(CNetwork::ServerEvent::E_SE_TO_CLIENT, CNetwork::ClientEvent::E_CE_UPDATE,
		nullptr, 0,
		(void*)serverInfo);

	// Send to all the clients
	CServer::SendNetwork(n);

	delete n;
}

/*
*/
void CServer::CreateListenSocket(const char* port)
{
	m_addrHints.ai_family = AF_INET;
	m_addrHints.ai_socktype = SOCK_STREAM;
	m_addrHints.ai_protocol = IPPROTO_TCP;
	m_addrHints.ai_flags = AI_PASSIVE;

	int result = getaddrinfo(NULL, port, &m_addrHints, &m_addrResult);

	if (result != 0)
	{
		m_errorLog->WriteError(true, "CServer::CreateListenSocket::getaddrinfo:%i\n", result);

		return;
	}

	m_listenSocket = socket(m_addrResult->ai_family, m_addrResult->ai_socktype, m_addrResult->ai_protocol);

	if (m_listenSocket == INVALID_SOCKET)
	{
		m_errorLog->WriteWinsockErrorMessage(true, "CServer::CreateListenSocket::socket:");

		freeaddrinfo(m_addrResult);

		return;
	}

	result = bind(m_listenSocket, m_addrResult->ai_addr, (int)m_addrResult->ai_addrlen);

	if (result == SOCKET_ERROR)
	{
		m_errorLog->WriteWinsockErrorMessage(true, "CServer::CreateListenSocket::bind:");
	}
}

/*
*/
void CServer::DestroyClient(CServerInfo* serverInfo)
{
	m_errorLog->WriteError(true, "CServer::DestroyClient::Client Closing Connection:%s\n", serverInfo->m_name);

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

	
	serverInfo->m_isAvailable = true;

	CNetwork* n = new CNetwork(CNetwork::ServerEvent::E_SE_TO_CLIENT, CNetwork::ClientEvent::E_CE_UPDATE,
		nullptr, 0,
		(void*)serverInfo);

	// Send to all the clients
	CServer::SendNetwork(n);

	delete n;


	serverInfo->Shutdown();

	serverInfo->Reset();
}

/*
*/
void CServer::InitializeWinsock()
{
	WORD wVersionRequested = MAKEWORD(2, 2);

	int result = WSAStartup(wVersionRequested, &m_wsaData);

	if (result != 0)
	{
		m_errorLog->WriteError(true, "CServer::InitializeWinsock::WSAStartup:%i\n", result);
	}

	m_errorLog->WriteError(true, "CServer::InitializeWinsock::WSAStartup:%s\n", m_wsaData.szDescription);
}

/*
*/
void CServer::ProcessEvent(CNetwork* network)
{
	CServerInfo* serverInfo = (CServerInfo*)network->m_serverInfo;

	CServerInfo* si = (CServerInfo*)m_serverInfo->GetElement(1, serverInfo->m_clientNumber);


	si->Constructor(m_errorLog);

	si->Initialize(serverInfo);

	
	if (strlen(si->m_chat) > 0)
	{
		CNetwork* n = new CNetwork(CNetwork::ServerEvent::E_SE_TO_CLIENT, CNetwork::ClientEvent::E_CE_CHAT,
			(void*)si->m_chat, (int)strlen(si->m_chat),
			(void*)si);

		// Send to all the clients
		CServer::SendNetwork(n);

		delete n;
	}


	pFunc[network->m_type](this, network, si);
}

/*
*/
void CServer::ReceiveClients()
{
	for (int i = 0; i < CServerInfo::E_MAX_CLIENTS; i++)
	{
		CServerInfo* serverInfo = (CServerInfo*)m_serverInfo->GetElement(1, i);

		if (serverInfo->m_isRunning)
		{
			switch (serverInfo->m_state)
			{
			case CServerInfo::E_LOADING:
			{
				break;
			}
			case CServerInfo::E_LOBBY:
			{
				CServer::ReceiveLobbySocket(serverInfo);

				break;
			}
			case CServerInfo::E_GAME:
			{
				if (m_timerReload[i]->m_isReloading)
				{
					m_timerReload[i]->Frame();
				}

				if (m_countdown > 0)
				{
					m_timerCountdown->Frame();
					if (m_timerCountdown->m_isReloading)
					{
					}
					else
					{
						m_timerCountdown->Start();

						m_countdown--;
					}
				}

				CServer::RequestActivity(serverInfo);

				CServer::ReceiveGameSocket(serverInfo);

				break;
			}
			}
		}
	}

	CServer::SendUpdates();
}

/*
*/
void CServer::ReceiveGameSocket(CServerInfo* serverInfo)
{
	int totalBytes = recv(serverInfo->m_socket, (char*)m_network, sizeof(CNetwork), 0);

	if (totalBytes > 0)
	{
		CServer::ProcessEvent(m_network);
	}
	else if (totalBytes == 0)
	{
		CServer::DestroyClient(serverInfo);
	}
	else if (WSAGetLastError() == WSAETIMEDOUT)
	{
		CServer_Idle(this, nullptr, serverInfo);
	}
	else if (totalBytes == INVALID_SOCKET)
	{
		m_errorLog->WriteWinsockErrorMessage(true, "CServer::ReceiveGameSocket::INVALID_SOCKET:");

		CServer::DestroyClient(serverInfo);
	}
	else if (totalBytes == SOCKET_ERROR)
	{
		m_errorLog->WriteWinsockErrorMessage(true, "CServer::ReceiveGameSocket::SOCKET_ERROR:");

		CServer::DestroyClient(serverInfo);
	}
}

/*
*/
void CServer::ReceiveLobbySocket(CServerInfo* serverInfo)
{
	int totalBytes = recv(serverInfo->m_socket, (char*)m_network, sizeof(CNetwork), 0);

	if (totalBytes > 0)
	{
		CServer::ProcessEvent(m_network);
	}
	else if (totalBytes == 0)
	{
		CServer::DestroyClient(serverInfo);
	}
	else if (WSAGetLastError() == WSAETIMEDOUT)
	{
		CServer_Idle(this, nullptr, serverInfo);
	}
	else if (totalBytes == INVALID_SOCKET)
	{
		m_errorLog->WriteWinsockErrorMessage(true, "CServer::ReceiveLobbySocket::INVALID_SOCKET:");

		CServer::DestroyClient(serverInfo);
	}
	else if (totalBytes == SOCKET_ERROR)
	{
		m_errorLog->WriteWinsockErrorMessage(true, "CServer::ReceiveLobbySocket::SOCKET_ERROR:");

		CServer::DestroyClient(serverInfo);
	}
}

/*
*/
void CServer::RequestActivity(CServerInfo* serverInfo)
{
	serverInfo->m_countdown = m_countdown;

	CNetwork* n = new CNetwork(CNetwork::ServerEvent::E_SE_TO_CLIENT, CNetwork::ClientEvent::E_CE_SEND_ACTIVITY,
		nullptr, 0,
		(void*)serverInfo);

	// Send to the client
	CServer::SendNetwork(n, serverInfo->m_socket);

	delete n;
}

/*
*/
void CServer::SendNetwork(CNetwork* network)
{
	for (int i = 0; i < CServerInfo::E_MAX_CLIENTS; i++)
	{
		CServerInfo* serverInfo = (CServerInfo*)m_serverInfo->GetElement(1, i);

		if (serverInfo->m_isRunning)
		{
			int totalBytes = send(serverInfo->m_socket, (char*)network, sizeof(CNetwork), 0);

			if (totalBytes == SOCKET_ERROR)
			{
				m_errorLog->WriteWinsockErrorMessage(true, "CServer::SendNetwork(All)::send:");

				serverInfo->m_socket = 0;
			}
		}
	}
}

/*
*/
void CServer::SendNetwork(CNetwork* network, SOCKET tempSocket)
{
	int totalBytes = send(tempSocket, (char*)network, sizeof(CNetwork), 0);

	if (totalBytes == SOCKET_ERROR)
	{
		m_errorLog->WriteWinsockErrorMessage(true, "CServer::SendNetwork::send:");
	}
}

/*
*/
void CServer::SendUpdates()
{
	for (int i = 0; i < CServerInfo::E_MAX_CLIENTS; i++)
	{
		CServerInfo* serverInfo = (CServerInfo*)m_serverInfo->GetElement(1, i);

		if ((serverInfo->m_isRunning) && (serverInfo->m_state == CServerInfo::E_GAME))
		{
			CNetwork* n = new CNetwork(CNetwork::ServerEvent::E_SE_TO_CLIENT, CNetwork::ClientEvent::E_CE_UPDATE,
				nullptr, 0,
				(void*)serverInfo);

			// Send to all the clients
			CServer::SendNetwork(n);

			delete n;
		}
	}
}

/*
*/
void CServer::Shutdown()
{
	CServer::ShutdownListen();

	CServer::ShutdownClients();

	if (m_serverEnvironment != nullptr)
	{
		delete m_serverEnvironment;

		m_serverEnvironment = nullptr;
	}

	m_blueTeamCount = 0;
	m_redTeamCount = 0;

	WSACleanup();
}

/*
*/
void CServer::ShutdownClients()
{
	for (int i = 0; i < CServerInfo::E_MAX_CLIENTS; i++)
	{
		if (m_timerReload[i] != nullptr)
		{
			delete m_timerReload[i];

			m_timerReload[i] = nullptr;
		}

		CServerInfo* serverInfo = (CServerInfo*)m_serverInfo->GetElement(1, i);

		serverInfo->Shutdown();
	}
}

/*
*/
void CServer::ShutdownListen()
{
	m_listenThreadRunning = false;

	// This shutdown will end the thread with WSAENOTCONN and WSAEINTR being acceptable
	int result = shutdown(m_listenSocket, SD_BOTH);

	if (result != 0)
	{
		m_errorLog->WriteWinsockErrorMessage(true, "CServer::ShutdownListen:shutdown:");
	}

	closesocket(m_listenSocket);

	m_listenSocket = 0;

	// Thread is hanging on the accept so wait for it to finish.
	// The informational messages can be ignored WSAENOTCONN because no one is connected and
	// WSAEINTR because the accept socket was interupted with the shutdown function.
	WaitForSingleObject(m_listenThreadHandle, 500);

	CloseHandle(m_listenThreadHandle);

	m_listenThreadHandle = 0;

	freeaddrinfo(m_addrResult);

	m_addrResult = 0;
}

/*
*/
void CServer::StartServer(const char* port)
{
	CServer::InitializeWinsock();
	CServer::CreateListenSocket(port);
	CServer::StartListenSocket();
}

/*
*/
void CServer::StartListenSocket()
{
	if (listen(m_listenSocket, SOMAXCONN) == SOCKET_ERROR)
	{
		m_errorLog->WriteWinsockErrorMessage(true, "CServer::StartListenSocket::listen:");

		return;
	}

	m_listenThreadRunning = true;

	m_listenThreadHandle = (HANDLE)_beginthreadex(NULL, sizeof(CServer),
		&CServer_ListenThread,
		(void*)this,
		0,
		&m_listenThreadId);
}

/*
*/
void CServer_AccountInfo(CServer* server, CNetwork* network, CServerInfo* serverInfo)
{
	// Object to store the clients account information
	CAccountInfo* accountInfo = new CAccountInfo(server, serverInfo, nullptr);

	// Thread to parse the XML retrieved from the account management system
	HANDLE handle = (HANDLE)_beginthreadex(
		NULL,
		sizeof(CAccountInfo),
		&CServer_AccountThread,
		(void*)accountInfo,
		0,
		nullptr);

	if (handle == 0)
	{
		server->m_errorLog->WriteError(true, "CServer_AccountInfo::_beginthreadex:CServer_AccountThread:%i\n", handle);

		delete accountInfo;

		return;
	}

	// Wait for the account thread to complete
	WaitForSingleObject(handle, INFINITE);

	// No need to hang onto the account information
	delete accountInfo;
}

/*
*/
void CServer_Activity(CServer* server, CNetwork* network, CServerInfo* serverInfo)
{
	if (server->m_countdown > 0)
	{
		return;
	}


	bool isIdle = true;

	for (int i = 0; i < CNetwork::E_DATA_SIZE; i++)
	{
		if (network->m_data[i] == CNetwork::ClientActivity::E_CA_ATTACK)
		{
			if (!server->m_timerReload[serverInfo->m_clientNumber]->m_isReloading)
			{
				server->m_timerReload[serverInfo->m_clientNumber]->Start();

				serverInfo->m_idleTime = 0.0f;

				CNetwork* n = new CNetwork(CNetwork::ServerEvent::E_SE_TO_CLIENT, CNetwork::ClientEvent::E_CE_ATTACK,
					nullptr, 0,
					(void*)serverInfo);

				// Send to all the clients
				server->SendNetwork(n);

				delete n;

				return;
			}
		}
		else if (network->m_data[i] != 0x00)
		{
			isIdle = false;

			break;
		}
	}

	if (isIdle)
	{
		CServer_Idle(server, nullptr, serverInfo);

		server->m_collision->Movement(serverInfo);

		return;
	}


	serverInfo->m_idleTime = 0.0f;

	serverInfo->m_action = network->m_data[0];

	server->m_collision->Movement(serverInfo);

	serverInfo->m_action = 0x00;


	// Check if the client collided with a collectable item.
	// Grab the collectable list for the cube where the client is located.
	int px = (int)(serverInfo->m_position.p.x + (server->m_serverEnvironment->m_width / 2.0f)) / server->m_serverEnvironment->m_gridUnits;
	int pz = (int)(serverInfo->m_position.p.z + (server->m_serverEnvironment->m_height / 2.0f)) / server->m_serverEnvironment->m_gridUnits;

	CLinkList<CObject>* collectables = (CLinkList<CObject>*)server->m_serverEnvironment->m_collectables->GetElement(2, px, pz);

	// Make sure the location is not out-of-range.
	if (collectables != nullptr)
	{
		// Make sure something is in the list.
		if (collectables->m_count > 0)
		{
			CLinkListNode<CObject>* collectable = collectables->m_list;

			while (collectable->m_object)
			{
				if (!collectable->m_object->m_limboTimer->m_isReloading)
				{
					CVertex p = serverInfo->m_position - CVertex(collectable->m_object->m_position.x, collectable->m_object->m_position.y, collectable->m_object->m_position.z);

					float l = p.Length();

					if (fabs(l) <= 4.0f)
					{
						collectable->m_object->m_limboTimer->Start();

						CNetwork* n = new CNetwork(CNetwork::ServerEvent::E_SE_TO_CLIENT, CNetwork::ClientEvent::E_CE_UPDATE_COLLECTABLE,
							nullptr, 0,
							(void*)serverInfo);

						sprintf_s((char*)n->m_data, CNetwork::E_DATA_SIZE, "%s %i %i", "item01", px, pz);

						// Send to all the clients
						server->SendNetwork(n);

						delete n;
					}
				}
				else
				{
					collectable->m_object->m_limboTimer->Frame();
				}

				collectable = collectable->m_next;
			}
		}
	}
}

/*
*/
void CServer_Chat(CServer* server, CNetwork* network, CServerInfo* serverInfo)
{
	CNetwork* n = new CNetwork(CNetwork::ServerEvent::E_SE_TO_CLIENT, CNetwork::ClientEvent::E_CE_CHAT,
		(char*)network->m_data, (int)strlen((char*)network->m_data),
		(void*)serverInfo);

	// Send to all the clients
	server->SendNetwork(n);

	server->m_errorLog->WriteError(true, "CServer_Chat::%s\n", (char*)network->m_data);

	delete n;
}

/*
*/
void CServer_Disconnect(CServer* server, CNetwork* network, CServerInfo* serverInfo)
{
	server->m_errorLog->WriteError(true, "CServer_Disconnect::Client Closing Connection:%s\n", serverInfo->m_name);

	CString* message = new CString(serverInfo->m_name);

	message->Concat(" disconnected\n");

	CNetwork* n = new CNetwork(CNetwork::ServerEvent::E_SE_TO_CLIENT, CNetwork::ClientEvent::E_CE_CHAT,
		(char*)message->GetText(), message->GetLength(),
		(void*)serverInfo);

	// Send to all the clients
	server->SendNetwork(n);

	delete message;


	switch (serverInfo->m_team)
	{
	case CServerInfo::Team::E_TEAM_RED:
	{
		server->m_redTeamCount--;

		break;
	}
	case CServerInfo::Team::E_TEAM_BLUE:
	{
		server->m_blueTeamCount--;

		break;
	}
	}


	serverInfo->m_isRunning = false;

	serverInfo->Shutdown();
}

/*
*/
void CServer_Idle(CServer* server, CNetwork* network, CServerInfo* serverInfo)
{
	serverInfo->m_idleTime += server->m_frametime->m_frametime;

	// Client idle state too long
	// 60 seconds is a minute times 60 minutes is an hour
	if (serverInfo->m_idleTime >= (60.0f * 60.0f))
	{
		CServer_Disconnect(server, network, serverInfo);

		return;
	}
}

/*
*/
void CServer_Info(CServer* server, CNetwork* network, CServerInfo* serverInfo)
{
	for (int i = 0; i < CServerInfo::E_MAX_CLIENTS; i++)
	{
		CServerInfo* si = (CServerInfo*)server->m_serverInfo->GetElement(1, i);

		CNetwork* n = new CNetwork(CNetwork::ServerEvent::E_SE_TO_CLIENT, CNetwork::ClientEvent::E_CE_INFO,
			(void*)si->m_name, (int)strlen(si->m_name),
			(void*)si);

		// Send all clients to the client entering
		if (si->m_clientNumber != serverInfo->m_clientNumber)
		{
			server->SendNetwork(n, serverInfo->m_socket);
		}

		delete n;
	}
}

/*
*/
void CServer_Join(CServer* server, CNetwork* network, CServerInfo* serverInfo)
{
	// When there is an active environment notify the client to load the environment
	if (server->m_serverEnvironment != nullptr)
	{
		CNetwork* n = new CNetwork(CNetwork::ServerEvent::E_SE_TO_CLIENT, CNetwork::ClientEvent::E_CE_LOAD_ENVIRONMENT,
			(void*)server->m_serverEnvironment->m_name->GetText(), server->m_serverEnvironment->m_name->GetLength(),
			(void*)serverInfo);

		server->SendNetwork(n, serverInfo->m_socket);

		delete n;
	}
}

/*
*/
void CServer_LoadEnvironment(CServer* server, CNetwork* network, CServerInfo* serverInfo)
{
	if (server->m_serverEnvironment)
	{
		delete server->m_serverEnvironment;
	}

	server->m_errorLog->WriteError(true, "CServer_LoadEnvironment::%s\n", (char*)network->m_data);

	server->m_serverEnvironment = new CServerEnvironment((char*)network->m_data);

	server->m_collision->m_environment = server->m_serverEnvironment;
}

/*
*/
void CServer_Ready(CServer* server, CNetwork* network, CServerInfo* serverInfo)
{
	if (server->m_redTeamCount > server->m_blueTeamCount)
	{
		serverInfo->m_team = CServerInfo::E_TEAM_BLUE;

		server->m_blueTeamCount++;
	}
	else
	{
		serverInfo->m_team = CServerInfo::Team::E_TEAM_RED;

		server->m_redTeamCount++;
	}


	srand((UINT)time(NULL));

	int location = rand() % 4;

	if (serverInfo->m_team == CServerInfo::Team::E_TEAM_RED)
	{
		serverInfo->m_position = server->m_serverEnvironment->m_redTeamStarts->Search(location)->m_object->m_position;
		serverInfo->m_direction = server->m_serverEnvironment->m_redTeamStarts->Search(location)->m_object->m_direction;
	}

	if (serverInfo->m_team == CServerInfo::Team::E_TEAM_BLUE)
	{
		serverInfo->m_position = server->m_serverEnvironment->m_blueTeamStarts->Search(location)->m_object->m_position;
		serverInfo->m_direction = server->m_serverEnvironment->m_blueTeamStarts->Search(location)->m_object->m_direction;
	}

	serverInfo->m_isFalling = true;


	CNetwork* n = new CNetwork(CNetwork::ServerEvent::E_SE_TO_CLIENT, CNetwork::ClientEvent::E_CE_ENTER,
		(void*)serverInfo->m_name, (int)strlen(serverInfo->m_name),
		(void*)serverInfo);

	// Send to all the clients
	server->SendNetwork(n);

	delete n;
}

/*
* Account retrieve and send to client
*/
unsigned int __stdcall CServer_AccountThread(void* obj)
{
	HRESULT hr = OleInitialize(0);


	CAccountInfo* accountInfo = (CAccountInfo*)obj;

	CServer* server = (CServer*)accountInfo->m_server;

	CServerInfo* serverInfo = accountInfo->m_serverInfo;


	server->m_errorLog->WriteError(true, "CServer_AccountThread Starting\n");

	// http request (server would be on same machine so not much need for https) to retrieve the clients account data in an XML format
	// or can be accomplished with MySql and formatting the result rows as required.
	accountInfo->RequestAccountInfo();

	// Concatenating all the buffers that were returned
	CString* buffer = new CString("");

	CLinkListNode<CString>* bufferNode = accountInfo->m_httpRequest->m_buffers->m_list;

	while (bufferNode->m_object)
	{
		buffer->Concat(bufferNode->m_object->GetText());

		bufferNode = bufferNode->m_next;
	}

#ifdef _DEBUG
	server->m_errorLog->WriteBytes(false, buffer->GetText());
#endif

	// loadXML is expecting the string to be encoded utf-16
	CComBSTR arg(buffer->GetWText());

	// done with the buffer
	delete buffer;

	// Begin XML parse
	CComPtr<IXMLDOMDocument> m_iXMLDoc;

	hr = m_iXMLDoc.CoCreateInstance(__uuidof(DOMDocument60));

	VARIANT_BOOL bSuccess = {};

	hr = m_iXMLDoc->loadXML(arg, &bSuccess);

	if (hr == S_OK)
	{
		CComPtr<IXMLDOMElement> docRoot;
		CComPtr<IXMLDOMNodeList> nodeList;
		CComPtr<IXMLDOMNode> nodeTemp;

		DOMNodeType nodeType = {};

		hr = m_iXMLDoc->get_documentElement(&docRoot);

		hr = docRoot->get_childNodes(&nodeList);

		BSTR nodeName[128] = {};
		BSTR nodeValue[128] = {};

		hr = nodeList->nextNode(&nodeTemp);

		while (hr == S_OK)
		{
			nodeTemp->get_nodeType(&nodeType);

			switch (nodeType)
			{
			case NODE_ELEMENT:
			{
				nodeTemp->get_nodeName(nodeName);

				nodeTemp->get_text(nodeValue);

				CString* node = new CString(nodeName[0]);

				if (node->Compare("data") == 0)
				{
					CComPtr<IXMLDOMNodeList> metarNodes;

					hr = nodeTemp->get_childNodes(&metarNodes);

					ProcessMETARNodes(server, serverInfo, metarNodes);

					metarNodes.Release();
				}
				else
				{
					CString* value = new CString(nodeValue[0]);

					node->Concat(" ");

					node->Concat(value->GetText());


					CNetwork* n = new CNetwork(CNetwork::ServerEvent::E_SE_TO_CLIENT, CNetwork::ClientEvent::E_CE_CHAT,
						(char*)node->GetText(), node->GetLength(),
						(void*)serverInfo);

					// Send to all the clients
					server->SendNetwork(n, serverInfo->m_socket);

					delete n;


					delete value;
				}

				delete node;

				break;
			}
			case NODE_ATTRIBUTE:
			{
				break;
			}
			case NODE_TEXT:
			{
				break;
			}
			case NODE_COMMENT:
			{
				break;
			}
			}

			nodeTemp.Release();

			hr = nodeList->nextNode(&nodeTemp);
		}

		nodeTemp.Release();
		nodeList.Release();
		docRoot.Release();
	}
	else
	{
		CNetwork* n = new CNetwork(CNetwork::ServerEvent::E_SE_TO_CLIENT, CNetwork::ClientEvent::E_CE_CHAT,
			(char*)"Failed", 6,
			(void*)serverInfo);

		// Send to all the clients
		server->SendNetwork(n, serverInfo->m_socket);
	}

	server->m_errorLog->WriteError(true, "CServer_AccountThread Ending\n");

	OleUninitialize();

	_endthreadex(0);


	return 1;
}

/*
*/
void ProcessMETARNodes(CServer* server, CServerInfo* serverInfo, CComPtr<IXMLDOMNodeList> nodeList)
{
	CComPtr<IXMLDOMNode> nodeTemp;

	DOMNodeType nodeType = {};

	BSTR nodeName[128] = {};
	BSTR nodeValue[128] = {};

	HRESULT hr = nodeList->nextNode(&nodeTemp);

	while (hr == S_OK)
	{
		nodeTemp->get_nodeType(&nodeType);

		switch (nodeType)
		{
		case NODE_ELEMENT:
		{
			nodeTemp->get_nodeName(nodeName);

			nodeTemp->get_text(nodeValue);

			CString* node = new CString(nodeName[0]);

			if (node->Compare("METAR") == 0)
			{
				CComPtr<IXMLDOMNodeList> metarNodes;

				hr = nodeTemp->get_childNodes(&metarNodes);

				ProcessMETARNode(server, serverInfo, metarNodes);

				metarNodes.Release();
			}

			delete node;

			break;
		}
		}

		nodeTemp.Release();

		hr = nodeList->nextNode(&nodeTemp);
	}

	nodeTemp.Release();
}

/*
*/
void ProcessMETARNode(CServer* server, CServerInfo* serverInfo, CComPtr<IXMLDOMNodeList> nodeList)
{
	CComPtr<IXMLDOMNode> nodeTemp;

	DOMNodeType nodeType = {};

	BSTR nodeName[128] = {};
	BSTR nodeValue[128] = {};

	HRESULT hr = nodeList->nextNode(&nodeTemp);

	while (hr == S_OK)
	{
		nodeTemp->get_nodeType(&nodeType);

		switch (nodeType)
		{
		case NODE_ELEMENT:
		{
			nodeTemp->get_nodeName(nodeName);

			nodeTemp->get_text(nodeValue);

			CString* node = new CString(nodeName[0]);

			// child nodes
			if (node->Compare("quality_control_flags") == 0)
			{
				CComPtr<IXMLDOMNodeList> nodeFlagList;
				CComPtr<IXMLDOMNode> nodeFlags;

				nodeTemp->get_childNodes(&nodeFlagList);

				HRESULT hr = nodeFlagList->nextNode(&nodeFlags);

				while (hr == S_OK)
				{
					BSTR flagName[128] = {};
					BSTR flagValue[128] = {};

					nodeFlags->get_nodeName(flagName);
					nodeFlags->get_text(flagValue);

					CString* attrName = new CString(flagName[0]);

					CString* attrValue = new CString(flagValue[0]);

					attrName->Concat(" ");

					attrName->Concat(attrValue->GetText());


					CNetwork* n = new CNetwork(CNetwork::ServerEvent::E_SE_TO_CLIENT, CNetwork::ClientEvent::E_CE_CHAT,
						(char*)attrName->GetText(), attrName->GetLength(),
						(void*)serverInfo);

					// Send to all the clients
					server->SendNetwork(n, serverInfo->m_socket);

					delete n;


					delete attrValue;

					delete attrName;

					nodeFlags.Release();

					hr = nodeFlagList->nextNode(&nodeFlags);
				}

				nodeFlags.Release();
				nodeFlagList.Release();
			}
			// attribute nodes
			else if (node->Compare("sky_condition") == 0)
			{
				CComPtr<IXMLDOMNamedNodeMap> attributes;
				CComPtr<IXMLDOMNode> nodeAttribute;

				long count = {};

				nodeTemp->get_attributes(&attributes);
				attributes->get_length(&count);

				HRESULT hr = attributes->nextNode(&nodeAttribute);

				while (hr == S_OK)
				{
					BSTR attributeName[128] = {};
					BSTR attributeValue[128] = {};

					nodeAttribute->get_nodeName(attributeName);
					nodeAttribute->get_text(attributeValue);

					CString* attrName = new CString(attributeName[0]);

					CString* attrValue = new CString(attributeValue[0]);

					attrName->Concat(" ");

					attrName->Concat(attrValue->GetText());


					CNetwork* n = new CNetwork(CNetwork::ServerEvent::E_SE_TO_CLIENT, CNetwork::ClientEvent::E_CE_CHAT,
						(char*)attrName->GetText(), attrName->GetLength(),
						(void*)serverInfo);

					// Send to all the clients
					server->SendNetwork(n, serverInfo->m_socket);

					delete n;


					delete attrValue;

					delete attrName;

					nodeAttribute.Release();

					hr = attributes->nextNode(&nodeAttribute);
				}

				nodeAttribute.Release();
				attributes.Release();
			}
			// element tag and value
			else
			{
				CString* value = new CString(nodeValue[0]);

				node->Concat(" ");

				node->Concat(value->GetText());


				CNetwork* n = new CNetwork(CNetwork::ServerEvent::E_SE_TO_CLIENT, CNetwork::ClientEvent::E_CE_CHAT,
					(char*)node->GetText(), node->GetLength(),
					(void*)serverInfo);

				// Send to all the clients
				server->SendNetwork(n, serverInfo->m_socket);

				delete n;

				delete value;
			}

			delete node;

			break;
		}
		}

		nodeTemp.Release();

		hr = nodeList->nextNode(&nodeTemp);
	}

	nodeTemp.Release();
}


/*
* Waiting for connection from remote clients
*/
unsigned __stdcall CServer_ListenThread(void* obj)
{
	CServer* server = (CServer*)obj;

	server->m_errorLog->WriteError(true, "CServer_ListenThread Starting\n");

	while (server->m_listenThreadRunning)
	{
		if (server->m_listenSocket)
		{
			server->Accept();
		}
	}


	server->m_errorLog->WriteError(true, "CServer_ListenThread Ending\n");

	_endthreadex(0);

	return 1;
}