#include "CServerWavefront.h"

/*
*/
CServerWavefront::CServerWavefront()
{
	memset(this, 0x00, sizeof(CServerWavefront));
}

/*
*/
CServerWavefront::CServerWavefront(const char* filename)
{
	memset(this, 0x00, sizeof(CServerWavefront));

	m_name = new CString(filename);

	m_meshs = new CLinkList<CServerMesh>();

	m_vertex = new CVec3f[CServerWavefront::MAX_VERTICES]();
	m_normal = new CVec3f[CServerWavefront::MAX_VERTICES]();

	m_index = 0;

	m_objectScript.InitBuffer(filename);

	while (true)
	{
		if (m_objectScript.CheckEndOfBuffer())
		{
			break;
		}

		if (strncmp(m_objectScript.m_buffer, "v ", 2) == 0)
		{
			m_objectScript.Move(2);

			m_vertex[m_vertexCount].m_p.x = (float)atof(m_objectScript.GetToken());

			m_vertex[m_vertexCount].m_p.y = (float)atof(m_objectScript.GetToken());
			m_vertex[m_vertexCount].m_p.y *= -1.0f;

			m_vertex[m_vertexCount].m_p.z = (float)atof(m_objectScript.GetToken());

			m_vertexCount++;
		}
		else if (strncmp(m_objectScript.m_buffer, "vn ", 3) == 0)
		{
			m_objectScript.Move(3);

			m_normal[m_normalCount].m_p.x = (float)atof(m_objectScript.GetToken());
			m_normal[m_normalCount].m_p.y = (float)atof(m_objectScript.GetToken());
			m_normal[m_normalCount].m_p.z = (float)atof(m_objectScript.GetToken());

			m_normalCount++;
		}
		else if (strncmp(m_objectScript.m_buffer, "f ", 2) == 0)
		{
			m_objectScript.Move(2);

			sscanf_s(m_objectScript.m_buffer, "%i//%i %i//%i %i//%i\n", &m_vertexIndex[0], &m_normalIndex[0], &m_vertexIndex[1], &m_normalIndex[1], &m_vertexIndex[2], &m_normalIndex[2]);

			for (int i = 0; i < 3; i++)
			{
				m_vertices[m_index].m_p[i] = m_vertex[m_vertexIndex[i] - 1].m_p;
			}

			m_vertices[m_index].m_n = m_normal[m_normalIndex[0] - 1].m_p;

			m_index++;

			m_maxIndex = m_index;
		}

		m_objectScript.SkipEndOfLine();
	}

	CServerWavefront::LoadMeshList();

	m_isInitialized = true;
}

/*
*/
CServerWavefront::~CServerWavefront()
{
	delete m_meshs;
	delete m_name;
}

/*
*/
void CServerWavefront::WriteVertices(FILE* file)
{
	for (int i = 0; i < m_maxIndex; i++)
	{
		for (int v = 0; v < 3; v++)
		{
			fwrite(&m_vertices[i].m_p[v].x, sizeof(float), 1, file);
			fwrite(&m_vertices[i].m_p[v].y, sizeof(float), 1, file);
			fwrite(&m_vertices[i].m_p[v].z, sizeof(float), 1, file);
		}

		fwrite(&m_vertices[i].m_n.x, sizeof(float), 1, file);
		fwrite(&m_vertices[i].m_n.y, sizeof(float), 1, file);
		fwrite(&m_vertices[i].m_n.z, sizeof(float), 1, file);
	}
}

/*
*/
void CServerWavefront::LoadMeshList()
{
	m_mesh = new CServerMesh((BYTE*)m_vertices, m_maxIndex);

	m_meshs->Add(m_mesh, m_name->m_text);

	delete[] m_vertex;
	delete[] m_normal;
}