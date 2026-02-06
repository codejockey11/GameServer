#pragma once

#include "framework.h"

#include "../GameCommon/CList.h"

#include "CServerWavefront.h"

class CServerWavefrontManager
{
public:
	
	CList* m_models;
	CListNode* m_node;
	CServerWavefront* m_serverWavefront;

	CServerWavefrontManager();
	~CServerWavefrontManager();

	CServerWavefront* Create(const char* filename);
	void Delete(const char* filename);
	CServerWavefront* Get(const char* filename);
};