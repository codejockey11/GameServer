#include "CServerWavefrontManager.h"

/*
*/
CServerWavefrontManager::CServerWavefrontManager()
{
	memset(this, 0x00, sizeof(CServerWavefrontManager));

	m_models = new CLinkList<CServerWavefront>();
}

/*
*/
CServerWavefrontManager::~CServerWavefrontManager()
{
	delete m_models;
}

/*
*/
CServerWavefront* CServerWavefrontManager::Create(const char* filename)
{
	m_serverWavefront = CServerWavefrontManager::Get(filename);

	if (m_serverWavefront)
	{
		return m_serverWavefront;
	}

	m_serverWavefront = new CServerWavefront(filename);

	if (m_serverWavefront->m_isInitialized)
	{
		m_models->Add(m_serverWavefront, filename);

		return m_serverWavefront;
	}

	delete m_serverWavefront;

	return nullptr;
}

/*
*/
void CServerWavefrontManager::Delete(const char* filename)
{
	m_model = m_models->Search(filename);

	if (m_model)
	{
		m_models->Delete(m_model);
	}
}

/*
*/
CServerWavefront* CServerWavefrontManager::Get(const char* filename)
{
	m_model = m_models->Search(filename);

	if (m_model)
	{
		return m_model->m_object;
	}

	return nullptr;
}