#include "CWavefrontManager.h"

/*
*/
CWavefrontManager::CWavefrontManager()
{
	memset(this, 0x00, sizeof(CWavefrontManager));

	m_models = new CLinkList<CWavefront>();
}

/*
*/
CWavefrontManager::~CWavefrontManager()
{
	delete m_models;
}

/*
*/
CWavefront* CWavefrontManager::Create(const char* filename, const char* materialname)
{
	CWavefront* model = CWavefrontManager::Get(filename);

	if (model)
	{
		return model;
	}

	model = new CWavefront(filename, materialname);

	if (model->m_isInitialized)
	{
		m_models->Add(model, filename);

		return model;
	}

	delete model;
#ifdef _DEBUG
	OutputDebugStringA("CWavefrontManager::MakeTexture\n");
#endif
	return nullptr;
}

/*
*/
void CWavefrontManager::Delete(const char* filename)
{
	CLinkListNode<CWavefront>* model = m_models->Search(filename);

	if (model)
	{
		m_models->Delete(model);
	}
}

/*
*/
CWavefront* CWavefrontManager::Get(const char* filename)
{
	CLinkListNode<CWavefront>* model = m_models->Search(filename);

	if (model)
	{
		return model->m_object;
	}

	return nullptr;
}