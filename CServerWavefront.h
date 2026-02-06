#pragma once

#include "framework.h"

#include "../GameCommon/CList.h"
#include "../GameCommon/CScript.h"
#include "../GameCommon/CString.h"
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

	bool m_isInitialized;

	CList* m_meshs;
	CListNode* m_node;
	CScript m_objectScript;
	CServerMesh* m_mesh;
	CServerMeshVertex m_vertices[CServerWavefront::MAX_VERTICES];
	CString* m_name;
	CVec3f* m_normal;
	CVec3f* m_vertex;

	int32_t m_index;
	int32_t m_maxIndex;
	int32_t m_normalCount;
	int32_t m_normalIndex[3];
	int32_t m_vertexCount;
	int32_t m_vertexIndex[3];

	CServerWavefront();
	CServerWavefront(const char* filename);
	~CServerWavefront();

	void LoadMeshList();
	void WriteVertices(FILE* file);
};