#include "CServerInfo.h"

/*
*/
CServerInfo::CServerInfo()
{
	memset(this, 0x00, sizeof(CServerInfo));

	m_acceleration = 8.0f;
}

/*
*/
CServerInfo::CServerInfo(CErrorLog* errorLog)
{
	memset(this, 0x00, sizeof(CServerInfo));

	m_errorLog = errorLog;

	m_acceleration = 8.0f;
}

/*
*/
CServerInfo::~CServerInfo()
{
}

/*
*/
void CServerInfo::Constructor(CErrorLog* errorLog)
{
	m_errorLog = errorLog;

	m_acceleration = 8.0f;
}

/*
*/
void CServerInfo::Initialize(CServerInfo* serverInfo)
{
	if (serverInfo == nullptr)
	{
		return;
	}

	// Some of the items from the client are not needed and will cause problems if initialized
	CServerInfo::SetName(serverInfo->m_name);
	CServerInfo::SetChat(serverInfo->m_chat);

	m_direction = serverInfo->m_direction;
	//m_position = serverInfo->m_position;

	m_action = serverInfo->m_action;
	m_state = serverInfo->m_state;
	m_team = serverInfo->m_team;

	m_isAvailable = serverInfo->m_isAvailable;
	//m_isFalling = serverInfo->m_isFalling;
	m_isRunning = serverInfo->m_isRunning;

	m_clientNumber = serverInfo->m_clientNumber;
	//m_countdown = serverInfo->m_countdown;

	//m_acceleration = serverInfo->m_acceleration;
	//m_freefallVelocity = serverInfo->m_freefallVelocity;
	//m_idleTime = serverInfo->m_idleTime;
	//m_velocity = serverInfo->m_velocity;

	m_socket = serverInfo->m_socket;
}

/*
*/
void CServerInfo::Reset()
{
	memset(this, 0x00, sizeof(CServerInfo));

	m_isAvailable = true;
}

/*
*/
void CServerInfo::SetChat(const char* chat)
{
	memset((void*)m_chat, 0x00, CServerInfo::E_CHAT_SIZE);

	if (strlen(chat) >= CServerInfo::E_CHAT_SIZE)
	{
		memcpy_s((void*)m_chat, CServerInfo::E_CHAT_SIZE, (void*)chat, (rsize_t)CServerInfo::E_CHAT_SIZE - 1);

		return;
	}

	memcpy_s((void*)m_chat, CServerInfo::E_CHAT_SIZE, (void*)chat, strlen(chat));
}

/*
*/
void CServerInfo::SetDirection(XMFLOAT3* direction)
{
	m_direction.p.x = direction->x;
	m_direction.p.y = direction->y;
	m_direction.p.z = direction->z;
}

/*
*/
void CServerInfo::SetName(const char* name)
{
	memset((void*)m_name, 0x00, CServerInfo::E_NAME_SIZE);

	if (strlen(name) >= CServerInfo::E_NAME_SIZE)
	{
		memcpy_s((void*)m_name, CServerInfo::E_NAME_SIZE, (void*)name, (rsize_t)CServerInfo::E_NAME_SIZE - 1);

		return;
	}

	memcpy_s((void*)m_name, CServerInfo::E_NAME_SIZE, (void*)name, strlen(name));
}

/*
*/
void CServerInfo::Shutdown()
{
	m_isRunning = false;

	if (m_socket == 0)
	{
		return;
	}

	int result = shutdown(m_socket, SD_BOTH);

	if (result != 0)
	{
		m_errorLog->WriteWinsockErrorMessage(true, "CServerInfo::Shutdown:shutdown:");
	}

	closesocket(m_socket);

	m_socket = 0;
}