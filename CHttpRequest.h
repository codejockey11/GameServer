#pragma once

#include "framework.h"

#include "CHttpFields.h"
#include "CLinkList.h"
#include "CString.h"

class CHttpRequest
{
public:

	CLinkList<CString>* m_buffers;
	CString* m_buffer;

	CHttpRequest();
	~CHttpRequest();

	void FormRequest(const char* url, CLinkList<CHttpFields>* fields);
	void UrlRequest(const char* url);

private:

	CURL* m_curl;
	CURLcode m_res;
};