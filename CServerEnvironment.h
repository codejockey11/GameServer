#pragma once

#include "framework.h"

#include "../GameCommon/CCollisionPrimitive.h"
#include "../GameCommon/CEntity.h"
#include "../GameCommon/CErrorLog.h"
#include "../GameCommon/CHeapArray.h"
#include "../GameCommon/CKeyValue.h"
#include "../GameCommon/CLinkList.h"
#include "../GameCommon/CSector.h"
#include "../GameCommon/CString.h"
#include "../GameCommon/CToken.h"
#include "../GameCommon/CVec3i.h"

#include "CServerObject.h"
#include "CServerWavefrontManager.h"
#include "CPlayerStart.h"

class CServerEnvironment
{
public:

	CCollisionPrimitive* m_collisionPrimitive;
	CEntity* m_entity;
	CErrorLog* m_errorLog;
	CHeapArray* m_sectorCollectables;
	CHeapArray* m_collisionPrimitives;
	CKeyValue m_keyValue;
	CLinkList<CCollisionPrimitive>* m_collisionSector;
	CLinkList<CPlayerStart>* m_blueTeamStarts;
	CLinkList<CPlayerStart>* m_redTeamStarts;
	CLinkList<CServerObject>* m_collectables;
	CPlayerStart* m_playerStart;
	CSector* m_sector;
	CServerWavefrontManager* m_wavefrontManager;
	CString* m_filename;
	CString* m_name;
	CToken m_mapScript;
	CVec3f m_origin;
	CVec3f m_direction;
	CVec3i m_collisionIndex;
	CVec3i m_mapSize;
	CVec3i m_sectorIndex;

	char m_key[CKeyValue::MAX_KEY];
	char m_value[CKeyValue::MAX_VALUE];

	char* m_team;

	errno_t m_err;

	FILE* m_fMaps;

	int m_entityCount;
	int m_keyValueCount;
	int m_mapSizeX;
	int m_mapSizeY;
	int m_mapSizeZ;
	int m_maxEntityCount;
	int m_sectorSize;

	UINT m_height;
	UINT m_primSize;
	UINT m_width;

	CServerEnvironment();
	CServerEnvironment(CErrorLog* errorLog, const char* scriptName);
	~CServerEnvironment();
};