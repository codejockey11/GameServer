#pragma once

#include "framework.h"

#include "CServerMeshVertex.h"

class CServerVertexBuffer
{
public:

	CServerMeshVertex* m_buffer;
	CServerMeshVertex* m_vertex;
	CServerMeshVertex* m_vertices;

	float m_pitch;
	float m_roll;
	float m_yaw;

	int32_t m_count;

	XMFLOAT4 m_position;
	XMFLOAT4 m_rotation;
	XMFLOAT4 m_scale;

	XMMATRIX m_matrixFinal;
	XMMATRIX m_matrixScaling;
	XMMATRIX m_matrixTranslation;
	XMMATRIX m_matrixYpr;

	XMVECTOR m_vectorNormal;
	XMVECTOR m_vectorPosition;
	XMVECTOR m_vectorTransform;

	CServerVertexBuffer();
	CServerVertexBuffer(int32_t count, void* vertices);
	~CServerVertexBuffer();

	void UpdateBuffer(void* vertices);
	void UpdateRotation();
};