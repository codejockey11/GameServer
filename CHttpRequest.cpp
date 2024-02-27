#include "CHttpRequest.h"

/*
*/
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* obj)
{
	CHttpRequest* h = (CHttpRequest*)obj;

	size_t buffer_size = size * nmemb;

	h->m_buffer = new CString((char*)contents, (int)nmemb);

	h->m_buffers->Add(h->m_buffer, "");

	return size * nmemb;
}

/*
*/
CHttpRequest::CHttpRequest()
{
	memset(this, 0x00, sizeof(CHttpRequest));

	m_buffers = new CLinkList<CString>();
}

/*
*/
CHttpRequest::~CHttpRequest()
{
	delete m_buffers;
}

/*
*/
void CHttpRequest::FormRequest(const char* url, CLinkList<CHttpFields>* fields)
{
	m_curl = curl_easy_init();

	if (m_curl)
	{
		curl_mime* form = curl_mime_init(m_curl);

		if (fields)
		{
			CLinkListNode<CHttpFields>* httpFields = fields->m_list;

			while (httpFields->m_object)
			{
				curl_mimepart* field = curl_mime_addpart(form);

				curl_mime_name(field, httpFields->m_object->m_name->GetText());
				curl_mime_data(field, httpFields->m_object->m_data->GetText(), httpFields->m_object->m_data->GetLength());

				httpFields = httpFields->m_next;
			}
		}

		// initialize custom header list stating that Expect: 100-continue is not wanted
		struct curl_slist* headerlist = {};

		headerlist = curl_slist_append(headerlist, "Expect:");

		curl_easy_setopt(m_curl, CURLOPT_URL, url);
		curl_easy_setopt(m_curl, CURLOPT_USERAGENT, "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/108.0.0.0 Safari/537.36");
		curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, WriteCallback);
		curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, this);

		/* only disable 100-continue header if explicitly requested */
		curl_easy_setopt(m_curl, CURLOPT_HTTPHEADER, headerlist);
		curl_easy_setopt(m_curl, CURLOPT_MIMEPOST, form);

		m_res = curl_easy_perform(m_curl);

		curl_easy_cleanup(m_curl);

		curl_mime_free(form);
	}
}

/*
*/
void CHttpRequest::UrlRequest(const char* url)
{
	m_curl = curl_easy_init();

	if (m_curl)
	{
		curl_easy_setopt(m_curl, CURLOPT_URL, url);
		curl_easy_setopt(m_curl, CURLOPT_USERAGENT, "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/108.0.0.0 Safari/537.36");
		curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, WriteCallback);
		curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, this);

		m_res = curl_easy_perform(m_curl);

		curl_easy_cleanup(m_curl);
	}
}