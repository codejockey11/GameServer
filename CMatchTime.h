#pragma once

#include "framework.h"

#include "../GameCommon/CTimer.h"

class CMatchTime
{
public:

	bool m_ended;
	bool m_started;

	CTimer* m_timer;

	int32_t m_seconds;
	int32_t m_totalSeconds;

	CMatchTime();
	CMatchTime(int32_t seconds);
	~CMatchTime();

	void Frame(uint64_t ft);
	void Start();
};