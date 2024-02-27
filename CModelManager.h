#pragma once

#include "framework.h"

#include "CLinkList.h"
#include "CModel.h"

class CModelManager
{
public:

	CModelManager();
	~CModelManager();

	CModel* Create(const char* filename);
	void Delete(const char* filename);
	CModel* Get(const char* filename);

private:

	CLinkList<CModel>* m_models;
};