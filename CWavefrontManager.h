#pragma once

#include "framework.h"

#include "CLinkList.h"
#include "CWavefront.h"

class CWavefrontManager
{
public:

	CWavefrontManager();
	~CWavefrontManager();

	CWavefront* Create(const char* filename, const char* materialname);
	void Delete(const char* filename);
	CWavefront* Get(const char* filename);

private:

	CLinkList<CWavefront>* m_models;
};