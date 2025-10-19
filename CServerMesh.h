#pragma once

#include "framework.h"

#include "CServerMeshVertex.h"

class CServerMesh
{
public:

	enum
	{
		E_MESH_COUNT = 8
	};

	BYTE* m_vertices;

	int m_bufferSize;

	UINT m_vertexCount;

	CServerMesh();
	CServerMesh(BYTE* vertices, int count);
	~CServerMesh();
};