#include "CServerWavefrontManager.h"

/*
*/
CServerWavefrontManager::CServerWavefrontManager()
{
	memset(this, 0x00, sizeof(CServerWavefrontManager));

	m_models = new CList();
}

/*
*/
CServerWavefrontManager::~CServerWavefrontManager()
{
	m_node = m_models->m_list;

	while ((m_node) && (m_node->m_object))
	{
		m_serverWavefront = (CServerWavefront*)m_node->m_object;

		SAFE_DELETE(m_serverWavefront);

		m_node = m_models->Delete(m_node);
	}

	SAFE_DELETE(m_models);
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

	SAFE_DELETE(m_serverWavefront);

	return nullptr;
}

/*
*/
void CServerWavefrontManager::Delete(const char* filename)
{
	m_node = m_models->Search(filename);

	if (m_node)
	{
		m_serverWavefront = (CServerWavefront*)m_node->m_object;

		SAFE_DELETE(m_serverWavefront);

		m_models->Delete(m_node);
	}
}

/*
*/
CServerWavefront* CServerWavefrontManager::Get(const char* filename)
{
	m_node = m_models->Search(filename);

	if (m_node)
	{
		return (CServerWavefront*)m_node->m_object;
	}

	return nullptr;
}