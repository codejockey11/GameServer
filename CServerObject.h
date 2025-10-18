#pragma once

#include "framework.h"

#include "../GameCommon/CFrametime.h"
#include "../GameCommon/CLinkList.h"
#include "../GameCommon/CString.h"
#include "../GameCommon/CTimer.h"
#include "../GameCommon/CVec3f.h"

#include "CServerMesh.h"
#include "CServerVertexBuffer.h"

class CServerObject
{
public:

	CLinkList<CServerMesh>* m_meshs;
	CLinkListNode<CServerMesh>* m_mesh;
	CServerVertexBuffer* m_vertexBuffers[CServerMesh::E_MESH_COUNT];
	CString* m_name;
	CTimer* m_limboTimer;
	
	float m_amplitude;
	float m_bob;
	float m_count;
	float m_freq;
	float m_spin;

	int m_materialCount;

	XMFLOAT4 m_environmentPosition;
	XMFLOAT4 m_position;
	XMFLOAT4 m_rotation;
	XMFLOAT4 m_scale;
	
	CServerObject();
	CServerObject(CLinkList<CServerMesh>* meshs);
	~CServerObject();

	void Animation(CFrametime* frametime);
	void SetPosition(float x, float y, float z);
	void SetPosition(CVec3f* position);
	void SetRotation(float x, float y, float z);
	void SetRotation(CVec3f* position);
	void SetScale(float x, float y, float z);
	void UpdateBuffer();
};