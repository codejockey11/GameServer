#pragma once

#include "framework.h"

#include "../GameCommon/CLinkList.h"

#include "CServerWavefront.h"

class CServerWavefrontManager
{
public:
	
	CLinkList<CServerWavefront>* m_models;
	CLinkListNode<CServerWavefront>* m_model;
	CServerWavefront* m_serverWavefront;

	CServerWavefrontManager();
	~CServerWavefrontManager();

	CServerWavefront* Create(const char* filename);
	void Delete(const char* filename);
	CServerWavefront* Get(const char* filename);
};