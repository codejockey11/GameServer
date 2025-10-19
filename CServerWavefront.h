#pragma once

#include "framework.h"

#include "../GameCommon/CLinkList.h"
#include "../GameCommon/CString.h"
#include "../GameCommon/CToken.h"
#include "../GameCommon/CVec3f.h"

#include "CServerMesh.h"
#include "CServerMeshVertex.h"

class CServerWavefront
{
public:

	enum
	{
		MAX_VERTICES = 4096
	};

	CLinkList<CServerMesh>* m_meshs;
	CServerMesh* m_mesh;
	CServerMeshVertex m_vertices[CServerWavefront::MAX_VERTICES];
	CString* m_name;
	CToken m_objectScript;
	CVec3f* m_normal;
	CVec3f* m_vertex;

	bool m_isInitialized;

	int m_index;
	int m_maxIndex;
	int m_normalIndex[3];
	int m_vertexIndex[3];

	UINT m_vertexCount;
	UINT m_normalCount;

	CServerWavefront();
	CServerWavefront(const char* filename);
	~CServerWavefront();

	void LoadMeshList();
	void WriteVertices(FILE* file);
};