#pragma once

#include "framework.h"

#include "../GameCommon/CTimer.h"

class CMatchTime
{
public:

	CTimer* m_timer;
	
	bool m_ended;
	bool m_started;
	
	int m_seconds;
	int m_totalSeconds;

	CMatchTime();
	CMatchTime(int seconds);
	~CMatchTime();

	void Frame(ULONGLONG ft);
	void Start();
};