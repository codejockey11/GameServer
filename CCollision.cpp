#include "CCollision.h"

/*
*/
CCollision::CCollision()
{
	memset(this, 0x00, sizeof(CCollision));
}

/*
*/
CCollision::CCollision(CFrametime* frametime)
{
	memset(this, 0x00, sizeof(CCollision));

	m_frametime = frametime;
}

/*
*/
CCollision::~CCollision()
{
}

/*
Ground Collision

B  E-F
|\  \|
A-C  D

BAC is N1
DFE is N2
*/
bool CCollision::GroundCollision()
{
	m_serverInfo->m_position += m_serverInfo->m_direction * (m_frametime->m_frametime * m_serverInfo->m_velocity);

	// gravity
	if (m_serverInfo->m_isFalling)
	{
		m_serverInfo->m_freefallVelocity -= m_frametime->m_frametime * 2.0f;

		m_serverInfo->m_position.p.y += m_serverInfo->m_freefallVelocity;
	}

	// Grab the collision list for the cube where the client is located
	int px = (int)(m_serverInfo->m_position.p.x + (m_environment->m_width / 2.0f)) / m_environment->m_gridUnits;
	int pz = (int)(m_serverInfo->m_position.p.z + (m_environment->m_height / 2.0f)) / m_environment->m_gridUnits;

	CLinkList<CTerrainCollision>* collisions = (CLinkList<CTerrainCollision>*)m_environment->m_collisions->GetElement(2, px, pz);

	// Return when an out-of-range cube is found
	if (collisions == nullptr)
	{
		return false;
	}

	// Return when the list is empty
	if (collisions->m_count == 0)
	{
		return false;
	}

	CVertex direction = { 0.0f, -1.0f, 0.0f };

	CLinkListNode<CTerrainCollision>* collision = collisions->m_list;

	while (collision->m_object)
	{
		CCollision::IntersectPlane(&collision->m_object->m_n1, &collision->m_object->m_a, &m_serverInfo->m_position, &direction);

		if (m_length <= 1.0f)
		{
			CVertex pointOnPlane(m_serverInfo->m_position.p.x,
				m_serverInfo->m_position.p.y - m_length,
				m_serverInfo->m_position.p.z);

			if (CCollision::RayTriangleIntersect(&pointOnPlane, &collision->m_object->m_b, &collision->m_object->m_a, &collision->m_object->m_c))
			{
				pointOnPlane.p.y += 1.0f;

				m_serverInfo->m_position.Set(&pointOnPlane);

				m_serverInfo->m_isFalling = false;

				m_serverInfo->m_freefallVelocity = 0.0f;

				return true;
			}
		}

		collision = collision->m_next;
	}

	m_serverInfo->m_isFalling = true;

	return false;
}

/*
*/
bool CCollision::IntersectPlane(CVertex* planeNormal, CVertex* planeOrigin, CVertex* rayOrigin, CVertex* rayDirection)
{
	// Assuming vectors planeNormal and rayDirection are normalized
	float denom = planeNormal->Dot(*rayDirection);

	// Determine if ray is parallel or pointing away from the plane
	if (denom > -0.0f)
	{
		return false;
	}

	CVertex p0l0 = *planeOrigin - *rayOrigin;

	// To keep translation don't normalize p0l0
	m_length = p0l0.Dot(*planeNormal) / denom;

	// Returning true when collision in front of starting point
	// No need to test return value unless it is important to the caller
	if (m_length >= 0.0f)
	{
		return true;
	}

	return false;
}

/*
*/
void CCollision::Movement(CServerInfo* serverInfo)
{
	m_serverInfo = serverInfo;

	switch (m_serverInfo->m_action)
	{
	case CNetwork::ClientActivity::E_CA_STOP:
	{
		if (m_serverInfo->m_velocity > 0)
		{
			m_serverInfo->m_velocity -= m_frametime->m_frametime * m_serverInfo->m_acceleration;
		}
		else
		{
			m_serverInfo->m_velocity += m_frametime->m_frametime * m_serverInfo->m_acceleration;
		}

		if ((m_serverInfo->m_velocity > -1.0f) && ((m_serverInfo->m_velocity < 1.0f)))
		{
			m_serverInfo->m_velocity = 0.0f;
		}

		break;
	}
	case CNetwork::ClientActivity::E_CA_FORWARD:
	{
		m_serverInfo->m_velocity += m_frametime->m_frametime * m_serverInfo->m_acceleration;

		break;
	}
	case CNetwork::ClientActivity::E_CA_BACKWARD:
	{
		m_serverInfo->m_velocity -= m_frametime->m_frametime * m_serverInfo->m_acceleration;

		break;
	}
	case CNetwork::ClientActivity::E_CA_STEP_LEFT:
	{
		// yaw pitch roll in radians
		float yaw = -90.0f * _PiDiv180;

		m_serverInfo->m_direction = m_serverInfo->m_direction.RotateAngleByAxis(yaw, CVertex(0.0f, 1.0f, 0.0f));

		break;
	}
	case CNetwork::ClientActivity::E_CA_STEP_RIGHT:
	{
		// yaw pitch roll in radians
		float yaw = 90.0f * _PiDiv180;

		m_serverInfo->m_direction = m_serverInfo->m_direction.RotateAngleByAxis(yaw, CVertex(0.0f, 1.0f, 0.0f));

		break;
	}
	}

	m_serverInfo->m_direction.Normalize();

	if (m_serverInfo->m_velocity > 8.0f)
	{
		m_serverInfo->m_velocity = 8.0f;
	}
	else if (m_serverInfo->m_velocity < -8.0f)
	{
		m_serverInfo->m_velocity = -8.0f;
	}

#ifndef _DEBUG
	// friction
	if ((m_serverInfo->m_velocity > -0.125f) && ((m_serverInfo->m_velocity < 0.125f)))
	{
		m_serverInfo->m_velocity = 0.0f;
	}
	else
	{
		if (m_serverInfo->m_velocity > 0.0f)
		{
			m_serverInfo->m_velocity -= (m_frametime->m_frametime * 2.0f);
		}
		else
		{
			m_serverInfo->m_velocity += (m_frametime->m_frametime * 2.0f);
		}
	}
#endif
	if (m_environment != nullptr)
	{
		CCollision::GroundCollision();
	}
}

/*
* blackpawn.com/texts/pointinpoly/default.html
* realtimecollisiondetection.net
* en.wikipedia.org/wiki/Barycentric_coordinate_system
* mathworld.wolfram.com/BarycentricCoordinates.html
*/
bool CCollision::RayTriangleIntersect(CVertex* P, CVertex* A, CVertex* B, CVertex* C)
{
	// Compute vectors
	CVertex v0 = *C - *A;
	CVertex v1 = *B - *A;
	CVertex v2 = *P - *A;

	// Compute dot products
	float dot00 = v0.Dot(v0);
	float dot01 = v0.Dot(v1);
	float dot02 = v0.Dot(v2);
	float dot11 = v1.Dot(v1);
	float dot12 = v1.Dot(v2);

	// Compute barycentric coordinates
	float invDenom = 1 / ((dot00 * dot11) - (dot01 * dot01));

	// uv of intersection
	m_u = ((dot11 * dot02) - (dot01 * dot12)) * invDenom;
	m_v = ((dot00 * dot12) - (dot01 * dot02)) * invDenom;

	// Check if point is in triangle
	if ((m_u >= 0) && (m_v >= 0) && ((m_u + m_v) <= 1.0f))
	{
		return true;
	}

	return false;
}