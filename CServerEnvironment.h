#pragma once

#include "framework.h"

#include "../GameCommon/CCollisionPrimitive.h"
#include "../GameCommon/CEntity.h"
#include "../GameCommon/CErrorLog.h"
#include "../GameCommon/CHeapArray.h"
#include "../GameCommon/CKeyValue.h"
#include "../GameCommon/CList.h"
#include "../GameCommon/CScript.h"
#include "../GameCommon/CSector.h"
#include "../GameCommon/CString.h"
#include "../GameCommon/CVec3i.h"

#include "CPlayerStart.h"
#include "CServerObject.h"
#include "CServerWavefrontManager.h"

class CServerEnvironment
{
public:

	CCollisionPrimitive* m_collisionPrimitive;
	CEntity* m_entity;
	CErrorLog* m_errorLog;

	char m_key[CKeyValue::MAX_KEY];
	char m_value[CKeyValue::MAX_VALUE];
	
	char* m_classname;
	char* m_modelFilename;
	char* m_modelName;
	char* m_team;

	CHeapArray* m_collisionPrimitives;
	CKeyValue m_keyValue;
	CList* m_blueTeamStarts;
	CList* m_collectables;
	CList* m_collisions;
	CList* m_collisionSector;
	CList* m_redTeamStarts;
	CListNode* m_node;
	CPlayerStart* m_playerStart;
	CScript m_mapScript;
	CSector* m_sector;
	CServerObject* m_serverObject;
	CServerWavefront* m_wavefront;
	CServerWavefrontManager* m_wavefrontManager;
	CString* m_filename;
	CString* m_name;
	CVec3f m_direction;
	CVec3f m_origin;
	CVec3i m_collisionIndex;
	CVec3i m_mapSize;
	CVec3i m_sectorIndex;

	errno_t m_err;

	FILE* m_fMaps;

	float m_float;

	int32_t m_collisionPrimitiveCount;
	int32_t m_entityCount;
	int32_t m_height;
	int32_t m_keyValueCount;
	int32_t m_mapSizeX;
	int32_t m_mapSizeY;
	int32_t m_mapSizeZ;
	int32_t m_maxEntityCount;
	int32_t m_primSize;
	int32_t m_sectorSize;
	int32_t m_width;

	CServerEnvironment();
	CServerEnvironment(CErrorLog* errorLog, const char* scriptName);
	~CServerEnvironment();
};