#pragma once

#include "framework.h"

#include "../GameCommon/CTimer.h"
#include "../GameCommon/CVec3f.h"

class CPlayerStart
{
public:

	CVec3f m_direction;
	CVec3f m_position;

	CPlayerStart();
	CPlayerStart(CVec3f* position, CVec3f* direction);
	~CPlayerStart();
};