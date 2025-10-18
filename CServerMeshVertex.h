#pragma once

#include "framework.h"

class CServerMeshVertex
{
public:

	XMFLOAT3 m_p[3];

	XMFLOAT3 m_n;

	CServerMeshVertex();
	~CServerMeshVertex();
};