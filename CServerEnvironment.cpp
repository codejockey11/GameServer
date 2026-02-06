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

	m_redTeamStarts = new CList();
	m_blueTeamStarts = new CList();

	m_collectables = new CList();

	m_filename = new CString("C:/Users/junk_/source/repos/Game/main/maps/");
	m_filename->Append(m_name->m_text);
	m_filename->Append(".col");

	m_err = fopen_s(&m_fMaps, m_filename->m_text, "rb");

	if (m_err != 0)
	{
		m_errorLog->WriteError(true, "Error opening:%s\n", m_filename->m_text);

		return;
	}

	fread_s(&m_maxEntityCount, sizeof(int32_t), sizeof(int32_t), 1, m_fMaps);

	m_entity = new CEntity[m_maxEntityCount]();

	fread_s(&m_entity[m_entityCount].m_number, sizeof(int32_t), sizeof(int32_t), 1, m_fMaps);

	while (!feof(m_fMaps))
	{
		fread_s(&m_entity[m_entityCount].m_type, sizeof(unsigned char), sizeof(unsigned char), 1, m_fMaps);

		fread_s(&m_keyValueCount, sizeof(int32_t), sizeof(int32_t), 1, m_fMaps);

		m_entity[m_entityCount].Constructor(m_keyValueCount);

		for (int32_t i = 0; i < m_keyValueCount; i++)
		{
			fread_s(m_key, CKeyValue::MAX_KEY, sizeof(char), CKeyValue::MAX_KEY, m_fMaps);
			fread_s(m_value, CKeyValue::MAX_VALUE, sizeof(char), CKeyValue::MAX_VALUE, m_fMaps);

			m_entity[m_entityCount].AddKeyValue(m_key, m_value);
		}

		switch (m_entity[m_entityCount].m_type)
		{
		case CEntity::Type::WORLDSPAWN:
		{
			m_collisionPrimitiveCount = 0;

			m_entity[m_entityCount].GetKeyValue("mapSize", &m_mapSize);
			m_entity[m_entityCount].GetKeyValue("sectorSize", &m_sectorSize);

			m_sector = new CSector(m_mapSize.m_p.x, m_mapSize.m_p.z, m_mapSize.m_p.y, m_sectorSize);

			m_collisionPrimitives = new CHeapArray(true, sizeof(CList), 3, m_sector->m_gridWidth, m_sector->m_gridDepth, m_sector->m_gridHeight);

			fread_s(&m_collisionIndex.m_p.x, sizeof(int32_t), sizeof(int32_t), 1, m_fMaps);

			while (m_collisionIndex.m_p.x != -1)
			{
				fread_s(&m_collisionIndex.m_p.y, sizeof(int32_t) * 2, sizeof(int32_t), 2, m_fMaps);

				m_collisionPrimitive = new CCollisionPrimitive();

				m_collisionPrimitive->ReadPrimitive(m_fMaps);

				m_collisionSector = (CList*)m_collisionPrimitives->GetElement(3, m_collisionIndex.m_p.x, m_collisionIndex.m_p.z, m_collisionIndex.m_p.y);

				if (m_collisionSector->m_list == nullptr)
				{
					m_collisionSector->Constructor();
				}

				m_collisionSector->Append(m_collisionPrimitive, m_collisionPrimitiveCount);

				m_collisionPrimitiveCount++;

				fread_s(&m_collisionIndex.m_p.x, sizeof(int32_t), sizeof(int32_t), 1, m_fMaps);
			}

			break;
		}
		case CEntity::Type::INFOPLAYERSTART:
		{
			m_entity[m_entityCount].GetKeyValue("team", &m_team);
			m_entity[m_entityCount].GetKeyValue("origin", &m_origin);

			m_float = m_origin.m_p.y;
			m_origin.m_p.y = m_origin.m_p.z;
			m_origin.m_p.z = m_float;

			m_entity[m_entityCount].GetKeyValue("direction", &m_direction);

			m_playerStart = new CPlayerStart(&m_origin, &m_direction);

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

			m_float = m_origin.m_p.y;
			m_origin.m_p.y = m_origin.m_p.z;
			m_origin.m_p.z = m_float;

			m_entity[m_entityCount].GetKeyValue("model", &m_modelFilename);

			m_wavefront = m_wavefrontManager->Create(m_modelFilename);

			m_serverObject = new CServerObject(m_wavefront->m_meshs);

			m_entity[m_entityCount].GetKeyValue("name", &m_modelName);

			m_serverObject->SetName(m_modelName);

			m_serverObject->SetPosition(&m_origin);

			m_collectables->Append(m_serverObject, m_serverObject->m_name->m_text);

			break;
		}
		case CEntity::Type::TERRAIN:
		{
			fread_s(&m_collisionIndex.m_p.x, sizeof(int32_t), sizeof(int32_t), 1, m_fMaps);

			while (m_collisionIndex.m_p.x != -1)
			{
				fread_s(&m_collisionIndex.m_p.y, sizeof(int32_t) * 2, sizeof(int32_t), 2, m_fMaps);

				m_collisionPrimitive = new CCollisionPrimitive();

				m_collisionPrimitive->ReadPrimitive(m_fMaps);

				m_collisionSector = (CList*)m_collisionPrimitives->GetElement(3, m_collisionIndex.m_p.x, m_collisionIndex.m_p.z, m_collisionIndex.m_p.y);

				if (m_collisionSector->m_list == nullptr)
				{
					m_collisionSector->Constructor();
				}

				m_collisionSector->Append(m_collisionPrimitive, m_collisionPrimitiveCount);

				m_collisionPrimitiveCount++;

				fread_s(&m_collisionIndex.m_p.x, sizeof(int32_t), sizeof(int32_t), 1, m_fMaps);
			}

			break;
		}
		default:
		{
			m_entity[m_entityCount].GetKeyValue("classname", &m_classname);

			m_errorLog->WriteError(true, "Unhandled entity type:%s\n", m_classname);

			break;
		}
		}

		m_entityCount++;

		fread_s(&m_entity[m_entityCount].m_number, sizeof(int32_t), sizeof(int32_t), 1, m_fMaps);
	}

	SAFE_DELETE_ARRAY(m_entity);

	fclose(m_fMaps);
}

/*
*/
CServerEnvironment::~CServerEnvironment()
{
	if (m_sector)
	{
		for (int32_t y = 0; y < m_sector->m_gridHeight; y++)
		{
			for (int32_t z = 0; z < m_sector->m_gridDepth; z++)
			{
				for (int32_t x = 0; x < m_sector->m_gridWidth; x++)
				{
					m_collisions = (CList*)m_collisionPrimitives->GetElement(3, x, z, y);

					if (m_collisions->m_list)
					{
						m_node = m_collisions->m_list;

						while ((m_node) && (m_node->m_object))
						{
							m_collisionPrimitive = (CCollisionPrimitive*)m_node->m_object;

							SAFE_DELETE(m_collisionPrimitive);

							m_node = m_collisions->Delete(m_node);
						}
					}
				}
			}
		}
	}

	SAFE_DELETE(m_collisionPrimitives);

	m_node = m_collectables->m_list;

	while ((m_node) && (m_node->m_object))
	{
		m_serverObject = (CServerObject*)m_node->m_object;

		SAFE_DELETE(m_serverObject);

		m_node = m_collectables->Delete(m_node);
	}

	SAFE_DELETE(m_collectables);

	SAFE_DELETE(m_sector);

	m_node = m_blueTeamStarts->m_list;

	while ((m_node) && (m_node->m_object))
	{
		m_playerStart = (CPlayerStart*)m_node->m_object;

		SAFE_DELETE(m_playerStart);

		m_node = m_blueTeamStarts->Delete(m_node);
	}

	SAFE_DELETE(m_blueTeamStarts);

	m_node = m_redTeamStarts->m_list;

	while ((m_node) && (m_node->m_object))
	{
		m_playerStart = (CPlayerStart*)m_node->m_object;

		SAFE_DELETE(m_playerStart);

		m_node = m_redTeamStarts->Delete(m_node);
	}

	SAFE_DELETE(m_redTeamStarts);

	SAFE_DELETE(m_wavefrontManager);
	SAFE_DELETE(m_filename);
	SAFE_DELETE(m_name);
}