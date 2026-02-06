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

	int32_t m_bufferSize;

	int32_t m_vertexCount;

	CServerMesh();
	CServerMesh(BYTE* vertices, int32_t count);
	~CServerMesh();
};