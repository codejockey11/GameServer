#pragma once

#include "framework.h"

#include "CVertex.h"

class CVertexBuffer
{
public:

	XMFLOAT4 m_position;
	XMFLOAT4 m_rotation;
	XMFLOAT4 m_scale;
	
	XMMATRIX m_matrixFinal;
	XMMATRIX m_matrixScaling;
	XMMATRIX m_matrixTranslation;
	XMMATRIX m_matrixYpr;
	
	float m_pitch;
	float m_roll;
	float m_yaw;

	CVertexBuffer();
	CVertexBuffer(UINT count, void* vertices);
	~CVertexBuffer();

	void LoadBuffer(void* vertices);
	void Update(void* vertices);
	void UpdateRotation();
	void UpdateServer(void* vertices);

private:

	BYTE m_type;
	
	CVertexNT* m_serverVertices;

	UINT m_count;
};