#include "CModelManager.h"

/*
*/
CModelManager::CModelManager()
{
	memset(this, 0x00, sizeof(CModelManager));

	m_models = new CLinkList<CModel>();
}

/*
*/
CModelManager::~CModelManager()
{
	delete m_models;
}

/*
*/
CModel* CModelManager::Create(const char* filename)
{
	CModel* model = CModelManager::Get(filename);

	if (model)
	{
		return model;
	}

	model = new CModel(filename);

	if (model->m_isInitialized)
	{
		m_models->Add(model, filename);

		return model;
	}

	delete model;
#ifdef _DEBUG
	OutputDebugStringA("CModelManager::Create\n");
#endif
	return nullptr;
}

/*
*/
void CModelManager::Delete(const char* filename)
{
	CLinkListNode<CModel>* model = m_models->Search(filename);

	if (model)
	{
		m_models->Delete(model);
	}
}

/*
*/
CModel* CModelManager::Get(const char* filename)
{
	CLinkListNode<CModel>* model = m_models->Search(filename);

	if (model)
	{
		return model->m_object;
	}

	return nullptr;
}