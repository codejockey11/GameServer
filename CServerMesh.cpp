#include "CServerMesh.h"

/*
*/
CServerMesh::CServerMesh()
{
	memset(this, 0x00, sizeof(CServerMesh));
}

/*
*/
CServerMesh::CServerMesh(BYTE* vertices, int count)
{
	m_vertexCount = count;

	m_bufferSize = m_vertexCount * sizeof(CServerMeshVertex);

	m_vertices = new BYTE[m_bufferSize]();

	memcpy((void*)m_vertices, (void*)vertices, m_bufferSize);
}

/*
*/
CServerMesh::~CServerMesh()
{
	delete[] m_vertices;
}