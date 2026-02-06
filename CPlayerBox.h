#pragma once

#include "framework.h"

#include "../GameCommon/CCollisionPrimitive.h"
#include "../GameCommon/CVec3f.h"

class CPlayerBox
{
public:

	// larger number = smoother movement = slower frame
	enum
	{
		E_MAX_SIDES = 10
	};

	CCollisionPrimitive* m_nearest[CPlayerBox::E_MAX_SIDES];
	CVec3f m_n[CPlayerBox::E_MAX_SIDES];
	CVec3f m_pop[CPlayerBox::E_MAX_SIDES];

	float m_acceleration;
	float m_dist[CPlayerBox::E_MAX_SIDES];
	float m_freefallAcceleration;
	float m_height;
	float m_injurySpeed;
	float m_maxCollision;
	float m_maxVelocity;
	float m_maxFreeflight;
	float m_terminalVelocity;
	float m_width;

	CPlayerBox();
	~CPlayerBox();

	void Reset();
};