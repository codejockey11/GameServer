#include "CVertexBuffer.h"

/*
*/
CVertexBuffer::CVertexBuffer()
{
	memset(this, 0x00, sizeof(CVertexBuffer));
}

/*
*/
CVertexBuffer::CVertexBuffer(UINT count, void* vertices)
{
	memset(this, 0x00, sizeof(CVertexBuffer));

	m_count = count;

	m_matrixYpr = XMMatrixIdentity();

	m_matrixTranslation = XMMatrixIdentity();

	m_matrixScaling = XMMatrixIdentity();

	m_matrixFinal = XMMatrixIdentity();

	m_serverVertices = (CVertexNT*)malloc(sizeof(CVertexNT) * m_count);

	if (m_serverVertices == 0)
	{
		return;
	}

	memcpy((void*)m_serverVertices, vertices, sizeof(CVertexNT) * m_count);
}

/*
*/
CVertexBuffer::~CVertexBuffer()
{
	if (m_serverVertices)
	{
		free(m_serverVertices);
	}
}

/*
*/
void CVertexBuffer::UpdateRotation()
{
	// pitch yaw roll to radians
	m_pitch = m_rotation.x * _PiDiv180;
	m_yaw = m_rotation.y * _PiDiv180;
	m_roll = m_rotation.z * _PiDiv180;

	m_matrixScaling = XMMatrixScaling(m_scale.x, m_scale.y, m_scale.z);

	m_matrixYpr = XMMatrixRotationRollPitchYaw(m_pitch, m_yaw, m_roll);

	m_matrixTranslation = XMMatrixTranslation(m_position.x, m_position.y, m_position.z);

	m_matrixFinal = XMMatrixMultiply(XMMatrixMultiply(m_matrixYpr, m_matrixScaling), m_matrixTranslation);
}

/*
*/
void CVertexBuffer::UpdateServer(void* vertices)
{
	CVertexNT* pData = m_serverVertices;

	CVertexNT* vertex = (CVertexNT*)vertices;

	for (UINT i = 0; i < m_count; i++)
	{
		FXMVECTOR vp = XMLoadFloat3(&vertex->p);

		XMVECTOR v = XMVector3TransformCoord(vp, m_matrixFinal);

		pData->p = XMFLOAT3(XMVectorGetX(v), XMVectorGetY(v), XMVectorGetZ(v));


		FXMVECTOR vn = XMLoadFloat3(&vertex->n);

		// Never transform the normals with a translated matrix
		v = XMVector3TransformCoord(vn, m_matrixYpr);

		pData->n = XMFLOAT3(XMVectorGetX(v), XMVectorGetY(v), XMVectorGetZ(v));


		pData++;
		vertex++;
	}
}