#include "CServerVertexBuffer.h"

/*
*/
CServerVertexBuffer::CServerVertexBuffer()
{
	memset(this, 0x00, sizeof(CServerVertexBuffer));
}

/*
*/
CServerVertexBuffer::CServerVertexBuffer(UINT count, void* vertices)
{
	memset(this, 0x00, sizeof(CServerVertexBuffer));

	m_count = count;

	m_matrixYpr = XMMatrixIdentity();

	m_matrixTranslation = XMMatrixIdentity();

	m_matrixScaling = XMMatrixIdentity();

	m_matrixFinal = XMMatrixIdentity();

	m_vertices = new CServerMeshVertex[m_count]();

	memcpy((void*)m_vertices, vertices, sizeof(CServerMeshVertex) * m_count);
}

/*
*/
CServerVertexBuffer::~CServerVertexBuffer()
{
	delete[] m_vertices;
}

/*
*/
void CServerVertexBuffer::UpdateRotation()
{
	m_pitch = m_rotation.x * (float)M_PI / 180.0f;
	m_yaw = m_rotation.y * (float)M_PI / 180.0f;
	m_roll = m_rotation.z * (float)M_PI / 180.0f;

	m_matrixScaling = XMMatrixScaling(m_scale.x, m_scale.y, m_scale.z);

	m_matrixYpr = XMMatrixRotationRollPitchYaw(m_pitch, m_yaw, m_roll);

	m_matrixTranslation = XMMatrixTranslation(m_position.x, m_position.y, m_position.z);

	m_matrixFinal = XMMatrixMultiply(XMMatrixMultiply(m_matrixYpr, m_matrixScaling), m_matrixTranslation);
}

/*
*/
void CServerVertexBuffer::UpdateBuffer(void* vertices)
{
	m_buffer = m_vertices;

	m_vertex = (CServerMeshVertex*)vertices;

	for (UINT i = 0; i < m_count; i++)
	{
		for (UINT v = 0; v < 3; v++)
		{
			m_vectorPosition = XMLoadFloat3(&m_vertex->m_p[v]);

			m_vectorTransform = XMVector3TransformCoord(m_vectorPosition, m_matrixFinal);

			m_buffer->m_p[v] = XMFLOAT3(XMVectorGetX(m_vectorTransform), XMVectorGetY(m_vectorTransform), XMVectorGetZ(m_vectorTransform));
		}

		m_vectorNormal = XMLoadFloat3(&m_vertex->m_n);

		m_vectorTransform = XMVector3TransformCoord(m_vectorNormal, m_matrixYpr);

		m_buffer->m_n = XMFLOAT3(XMVectorGetX(m_vectorTransform), XMVectorGetY(m_vectorTransform), XMVectorGetZ(m_vectorTransform));


		m_buffer++;
		m_vertex++;
	}
}