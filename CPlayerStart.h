#pragma once

#include "framework.h"

#include "../GameCommon/CTimer.h"
#include "../GameCommon/CVec3f.h"

class CPlayerStart
{
public:

	CTimer* m_timer;

	CVec3f m_direction;
	CVec3f m_position;

	CPlayerStart();
	CPlayerStart(ULONGLONG time, CVec3f* position, CVec3f* direction);
	~CPlayerStart();
};