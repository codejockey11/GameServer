#include "CServerEnvironment.h"

/*
*/
CServerEnvironment::CServerEnvironment()
{
	memset(this, 0x00, sizeof(CServerEnvironment));
}

/*
*
* Collision
*
* B  E-F
* |\  \|
* A-C  D
*
* BAC is N1
* DFE is N2
*
* m_sectorSize represents a collection of triangles for an area of the terrain stored in a list.
* This will determine the number of terrain sectors for the width and height of the terrain.
* If the terrain is 256x256 dividing the terrain size by m_sectorSize = 8 yields 32x32 sectors.
* This grid will enable the use of a list per sector that defines the collision items.
*
*/
CServerEnvironment::CServerEnvironment(CErrorLog* errorLog, const char* scriptName)
{
	memset(this, 0x00, sizeof(CServerEnvironment));

	m_errorLog = errorLog;

	m_name = new CString(scriptName);

	m_wavefrontManager = new CServerWavefrontManager();

	m_redTeamStarts = new CLinkList<CPlayerStart>();
	m_blueTeamStarts = new CLinkList<CPlayerStart>();


	m_filename = new CString("C:/Users/junk_/source/repos/Game/main/maps/");
	m_filename->Append(m_name->m_text);
	m_filename->Append(".col");

	m_err = fopen_s(&m_fMaps, m_filename->m_text, "rb");

	if (m_err != 0)
	{
		m_errorLog->WriteError(true, "Error opening:%s\n", m_filename->m_text);

		return;
	}

	fread_s(&m_maxEntityCount, sizeof(int), sizeof(int), 1, m_fMaps);

	m_entity = new CEntity[m_maxEntityCount]();


	fread_s(&m_entity[m_entityCount].m_number, sizeof(int), sizeof(int), 1, m_fMaps);

	while (!feof(m_fMaps))
	{
		fread_s(&m_entity[m_entityCount].m_type, sizeof(unsigned char), sizeof(unsigned char), 1, m_fMaps);

		fread_s(&m_keyValueCount, sizeof(int), sizeof(int), 1, m_fMaps);

		m_entity[m_entityCount].Initialize(m_keyValueCount);

		for (int i = 0; i < m_keyValueCount; i++)
		{
			fread_s(m_key, CKeyValue::MAX_KEY, sizeof(char), CKeyValue::MAX_KEY, m_fMaps);
			fread_s(m_value, CKeyValue::MAX_VALUE, sizeof(char), CKeyValue::MAX_VALUE, m_fMaps);

			m_entity[m_entityCount].AddKeyValue(m_key, m_value);
		}

		switch (m_entity[m_entityCount].m_type)
		{
		case CEntity::Type::WORLDSPAWN:
		{
			int count = 0;

			m_entity[m_entityCount].GetKeyValue("mapSize", &m_mapSize);
			m_entity[m_entityCount].GetKeyValue("sectorSize", &m_sectorSize);

			m_sector = new CSector(m_mapSize.m_p.x, m_mapSize.m_p.z, m_mapSize.m_p.y, m_sectorSize);

			m_collisionPrimitives = new CHeapArray(sizeof(CLinkList<CCollisionPrimitive>), true, true, 3, m_sector->m_gridWidth, m_sector->m_gridHeight, m_sector->m_gridVertical);

			fread_s(&m_collisionIndex.m_p.x, sizeof(int), sizeof(int), 1, m_fMaps);

			while (m_collisionIndex.m_p.x != -1)
			{
				fread_s(&m_collisionIndex.m_p.y, sizeof(int) * 2, sizeof(int), 2, m_fMaps);

				m_collisionPrimitive = new CCollisionPrimitive();

				fread_s(&m_collisionPrimitive->m_a, sizeof(CVec3f) * 4, sizeof(CVec3f), 4, m_fMaps);

				m_collisionSector = (CLinkList<CCollisionPrimitive>*)m_collisionPrimitives->GetElement(3, m_collisionIndex.m_p.x, m_collisionIndex.m_p.y, m_collisionIndex.m_p.z);

				if (m_collisionSector != nullptr)
				{
					if (m_collisionSector->m_list == nullptr)
					{
						m_collisionSector->Constructor();
					}

					m_collisionSector->Append(m_collisionPrimitive, count);

					count++;
				}

				fread_s(&m_collisionIndex.m_p.x, sizeof(int), sizeof(int), 1, m_fMaps);
			}

			break;
		}
		case CEntity::Type::INFOPLAYERSTART:
		{
			m_entity[m_entityCount].GetKeyValue("team", &m_team);
			m_entity[m_entityCount].GetKeyValue("origin", &m_origin);
			m_entity[m_entityCount].GetKeyValue("direction", &m_direction);

			float t = m_origin.m_p.y;
			m_origin.m_p.y = m_origin.m_p.z;
			m_origin.m_p.z = t;

			m_playerStart = new CPlayerStart(15000, &m_origin, &m_direction);

			if (strncmp(m_team, "red", 3) == 0)
			{
				m_redTeamStarts->Append(m_playerStart, m_entity[m_entityCount].m_number);
			}
			else if (strncmp(m_team, "blue", 4) == 0)
			{
				m_blueTeamStarts->Append(m_playerStart, m_entity[m_entityCount].m_number);
			}

			break;
		}
		case CEntity::Type::COLLECTABLE:
		{
			m_entity[m_entityCount].GetKeyValue("origin", &m_origin);

			float t = m_origin.m_p.y;
			m_origin.m_p.y = m_origin.m_p.z;
			m_origin.m_p.z = t;

			char* filename = {};

			m_entity[m_entityCount].GetKeyValue("model", &filename);

			CServerWavefront* swf = m_wavefrontManager->Create(filename);

			
			m_sectorIndex = m_sector->GetSector(&m_origin);

			if (m_sectorCollectables == nullptr)
			{
				m_sectorCollectables = new CHeapArray(sizeof(CLinkList<CServerObject>), true, true, 2, m_sector->m_gridWidth, m_sector->m_gridHeight);
			}

			m_collectables = (CLinkList<CServerObject>*)m_sectorCollectables->GetElement(2, m_sectorIndex.m_p.x, m_sectorIndex.m_p.z);

			if (m_collectables->m_list == nullptr)
			{
				m_collectables->Constructor();
			}

			
			CServerObject* so = new CServerObject(swf->m_meshs);
			
			m_entity[m_entityCount].GetKeyValue("name", &filename);

			if (so->m_name)
			{
				delete so->m_name;
			}

			so->m_name = new CString(filename);
			
			so->SetPosition(&m_origin);

			m_collectables->Append(so, so->m_name->m_text);

			break;
		}
		case CEntity::Type::TERRAIN:
		{
			int count = 0;

			fread_s(&m_collisionIndex.m_p.x, sizeof(int), sizeof(int), 1, m_fMaps);

			while (m_collisionIndex.m_p.x != -1)
			{
				fread_s(&m_collisionIndex.m_p.y, sizeof(int) * 2, sizeof(int), 2, m_fMaps);

				m_collisionPrimitive = new CCollisionPrimitive();

				fread_s(&m_collisionPrimitive->m_a, sizeof(CVec3f) * 4, sizeof(CVec3f), 4, m_fMaps);

				m_collisionSector = (CLinkList<CCollisionPrimitive>*)m_collisionPrimitives->GetElement(3, m_collisionIndex.m_p.x, m_collisionIndex.m_p.y, m_collisionIndex.m_p.z);

				if (m_collisionSector != nullptr)
				{
					if (m_collisionSector->m_list == nullptr)
					{
						m_collisionSector->Constructor();
					}

					m_collisionSector->Append(m_collisionPrimitive, count);

					count++;
				}

				fread_s(&m_collisionIndex.m_p.x, sizeof(int), sizeof(int), 1, m_fMaps);
			}

			break;
		}
		default:
		{
			char* classname = {};
			
			m_entity[m_entityCount].GetKeyValue("classname", &classname);

			m_errorLog->WriteError(true, "Unhandled entity type:%s\n", classname);

			break;
		}
		}

		m_entityCount++;

		fread_s(&m_entity[m_entityCount].m_number, sizeof(int), sizeof(int), 1, m_fMaps);
	}

	delete[] m_entity;

	fclose(m_fMaps);
}

/*
*/
CServerEnvironment::~CServerEnvironment()
{
	if (m_sector != nullptr)
	{
		for (UINT py = 0; py < m_sector->m_gridVertical; py++)
		{
			for (UINT pz = 0; pz < m_sector->m_gridHeight; pz++)
			{
				for (UINT px = 0; px < m_sector->m_gridWidth; px++)
				{
					CLinkList<CCollisionPrimitive>* collisions = (CLinkList<CCollisionPrimitive>*)m_collisionPrimitives->GetElement(3, px, pz, py);

					if (collisions->m_list != nullptr)
					{
						CLinkListNode<CCollisionPrimitive>* lln = collisions->m_list;

						while (lln->m_object)
						{
							delete lln->m_object;

							lln = lln->m_next;
						}
					}
				}
			}
		}
	}

	if (m_sectorCollectables != nullptr)
	{
		if (m_sector != nullptr)
		{
			for (UINT pz = 0; pz < m_sector->m_gridHeight; pz++)
			{
				for (UINT px = 0; px < m_sector->m_gridWidth; px++)
				{
					m_collectables = (CLinkList<CServerObject>*)m_sectorCollectables->GetElement(2, px, pz);

					if (m_collectables->m_list != nullptr)
					{
						CLinkListNode<CServerObject>* lln = m_collectables->m_list;

						while (lln->m_object)
						{
							delete lln->m_object;

							lln = lln->m_next;
						}
					}
				}
			}
		}
	}

	delete m_collisionPrimitives;
	delete m_sectorCollectables;
	delete m_sector;
	delete m_blueTeamStarts;
	delete m_redTeamStarts;
	delete m_wavefrontManager;
	delete m_filename;
	delete m_name;
}