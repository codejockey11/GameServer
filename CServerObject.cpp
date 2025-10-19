#include "CServerObject.h"

/*
*/
CServerObject::CServerObject()
{
	memset(this, 0x00, sizeof(CServerObject));
}

/*
*/
CServerObject::CServerObject(CLinkList<CServerMesh>* meshs)
{
	memset(this, 0x00, sizeof(CServerObject));

	m_limboTimer = new CTimer(5000);

	m_meshs = meshs;

	m_mesh = m_meshs->m_list;

	while (m_mesh->m_object)
	{
		m_vertexBuffers[m_materialCount] = new CServerVertexBuffer(m_mesh->m_object->m_vertexCount, (void*)m_mesh->m_object->m_vertices);

		m_materialCount++;

		m_mesh = m_mesh->m_next;
	}

	m_amplitude = 0.125f;
	
	m_freq = 4.0f;
}

/*
*/
CServerObject::~CServerObject()
{
	m_mesh = m_meshs->m_list;

	m_materialCount = 0;

	while (m_mesh->m_object)
	{
		delete m_vertexBuffers[m_materialCount];
		
		m_vertexBuffers[m_materialCount] = nullptr;

		m_materialCount++;

		m_mesh = m_mesh->m_next;
	}

	delete m_limboTimer;
	delete m_name;
}

/*
*/
void CServerObject::Animation(CFrametime* frametime)
{
	m_count += m_freq * frametime->m_frametime;

	m_bob = m_environmentPosition.y + (m_amplitude * sinf(m_count));
	
	m_spin += 90.0f * frametime->m_frametime;
	
	if (m_spin > 359.99f)
	{
		m_spin -= 359.99f;
	}
	
	m_position.y = m_bob;
	
	m_rotation.y = m_spin;
}

/*
*/
void CServerObject::SetPosition(float x, float y, float z)
{
	m_position.x = x;
	m_position.y = y;
	m_position.z = z;

	m_environmentPosition = m_position;
}

/*
*/
void CServerObject::SetPosition(CVec3f* position)
{
	m_position.x = position->m_p.x;
	m_position.y = position->m_p.y;
	m_position.z = position->m_p.z;

	m_environmentPosition = m_position;
}

/*
*/
void CServerObject::SetRotation(float x, float y, float z)
{
	m_rotation.x = x;
	m_rotation.y = y;
	m_rotation.z = z;
}

/*
*/
void CServerObject::SetRotation(CVec3f* rotation)
{
	m_rotation.x = rotation->m_p.x;
	m_rotation.y = rotation->m_p.y;
	m_rotation.z = rotation->m_p.z;
}

/*
*/
void CServerObject::SetScale(float x, float y, float z)
{
	m_scale.x = x;
	m_scale.y = y;
	m_scale.z = z;
}

/*
*/
void CServerObject::UpdateBuffer()
{
	m_mesh = m_meshs->m_list;

	m_materialCount = 0;

	while (m_mesh->m_object)
	{
		m_vertexBuffers[m_materialCount]->m_scale.x = m_scale.x;
		m_vertexBuffers[m_materialCount]->m_scale.y = m_scale.y;
		m_vertexBuffers[m_materialCount]->m_scale.z = m_scale.z;

		m_vertexBuffers[m_materialCount]->m_position.x = m_position.x;
		m_vertexBuffers[m_materialCount]->m_position.y = m_position.y;
		m_vertexBuffers[m_materialCount]->m_position.z = m_position.z;

		m_vertexBuffers[m_materialCount]->m_rotation.x = m_rotation.x;
		m_vertexBuffers[m_materialCount]->m_rotation.y = m_rotation.y;
		m_vertexBuffers[m_materialCount]->m_rotation.z = m_rotation.z;

		m_vertexBuffers[m_materialCount]->UpdateRotation();

		m_vertexBuffers[m_materialCount]->UpdateBuffer(m_mesh->m_object->m_vertices);

		m_materialCount++;

		m_mesh = m_mesh->m_next;
	}
}