#include "CPlayerStart.h"

/*
*/
CPlayerStart::CPlayerStart()
{
	memset(this, 0x00, sizeof(CPlayerStart));
}

/*
*/
CPlayerStart::CPlayerStart(ULONGLONG time, CVec3f* position, CVec3f* direction)
{
	memset(this, 0x00, sizeof(CPlayerStart));

	m_position = (*position);

	m_direction = (*direction);

	m_timer = new CTimer(time);
}

/*
*/
CPlayerStart::~CPlayerStart()
{
	delete m_timer;
}