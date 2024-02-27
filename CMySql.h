#pragma once

#include "framework.h"

class CMySql
{
public:

	MYSQL* m_connection;
	MYSQL_RES* m_result;
	MYSQL_ROW m_rowset;
	
	int m_fieldCount;
	
	CMySql();
	~CMySql();
	
	bool PerformQuery(const char* query);

	int GetFieldCount();

	MYSQL_ROW FetchRow();
};

