#include "CMatchTime.h"

/*
*/
CMatchTime::CMatchTime()
{
	memset(this, 0x00, sizeof(CMatchTime));
}

/*
*/
CMatchTime::CMatchTime(int32_t seconds)
{
	memset(this, 0x00, sizeof(CMatchTime));

	m_timer = new CTimer(1000);

	m_seconds = seconds;
}

/*
*/
CMatchTime::~CMatchTime()
{
	SAFE_DELETE(m_timer);
}

/*
*/
void CMatchTime::Frame(uint64_t ft)
{
	m_timer->Frame(ft);

	if (m_timer->m_isReloading)
	{
		return;
	}

	m_timer->Start();

	m_totalSeconds++;

	if (m_totalSeconds == m_seconds)
	{
		m_ended = true;
	}
}

/*
*/
void CMatchTime::Start()
{
	m_started = true;

	m_ended = false;

	m_timer->Start();
}