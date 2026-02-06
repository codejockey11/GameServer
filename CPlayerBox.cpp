#include "CPlayerBox.h"

/*
*/
CPlayerBox::CPlayerBox()
{
	memset(this, 0x00, sizeof(CPlayerBox));

	m_height = 6.0f + (3.0f / 12.0f);
	m_width = 2.0f;

	// unit vector
	// 1 foot
	// 30.48 centimeters
	//
	// 88.0f feet per second is 60 mph = 1.46666
	//
	// Walking 3 mph
	// Jogging 5 mph
	// Running 6 mph
	// Manatee 16 mph
	// Gallop 25 - 30
	// Quarter Horses 45 - 50
	//
	// m_maxVelocity in units

	// Manatee mph
	m_maxVelocity = 1.46666f * 6.0f;

	// map testing at 60mph
	m_maxFreeflight = 1.46666f * 60.0f;

	m_acceleration = 512.0f;

	// terminal velocity is 120 fps
	m_terminalVelocity = 120.0f;

	// 9.81 meters per second
	// 32.18504 feet per second
	m_freefallAcceleration = 32.18504f;

	// speed at 10 feet
	m_injurySpeed = 25.0f;

	// max distance for baricentric
	m_maxCollision = m_height + 4.0f;

	m_n[0] = CVec3f(0.0f, 1.0f, 0.0f);
	m_n[1] = CVec3f(0.0f, -1.0f, 0.0f);

	CVec3f start = CVec3f(0.0f, 0.0f, 1.0f);

	float degrees = 360.0f / (float)CPlayerBox::E_MAX_SIDES - 2.0f;

	for (int32_t i = 2; i < CPlayerBox::E_MAX_SIDES; i++)
	{
		start.Normalize();

		m_n[i] = start;

		start = start.RotateAngleByAxis(degrees * DEG2RAD, &m_n[0]);
	}

	CPlayerBox::Reset();
}

/*
*/
CPlayerBox::~CPlayerBox()
{

}

/*
*/
void CPlayerBox::Reset()
{
	for (int32_t i = 0; i < CPlayerBox::E_MAX_SIDES; i++)
	{
		m_dist[i] = 99999.99f;
	}
}