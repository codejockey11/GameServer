#include "CHttpFields.h"

/*
*/
CHttpFields::CHttpFields()
{
	memset(this, 0x00, sizeof(CHttpFields));
}

/*
*/
CHttpFields::CHttpFields(const char* name, const char* data)
{
	memset(this, 0x00, sizeof(CHttpFields));

	m_name = new CString(name);
	m_data = new CString(data);
}

/*
*/
CHttpFields::~CHttpFields()
{
	delete m_data;
	delete m_name;
}
