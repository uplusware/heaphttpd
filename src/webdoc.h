/*
	Copyright (c) openheap, uplusware
	uplusware@gmail.com
*/

#ifndef _WEBAPI_H_
#define _WEBAPI_H_

#include "http.h"
#include "httpcomm.h"
#include "heapapi.h"
#include "httprequest.h"
#include "httpresponse.h"
#include "httputility.h"

class doc
{
public:
    doc() {}    
    virtual ~doc() {}
    virtual void Response() = 0;
};

class web_api : public doc
{
protected:
	http_request * m_request;
    http_response * m_response;
	
public:
	web_api(http_request* request, http_response *response)
	{
		m_request = request;
        m_response = response;
	}
	virtual ~web_api() {}

	virtual void Response() = 0;
};

class web_doc : public doc
{
protected:
	CHttp * m_session;
    string m_work_path;
	
public:
	web_doc(CHttp* session, const char* work_path)
	{
		m_session = session;
        m_work_path = work_path;
	}
	virtual ~web_doc() {}

	virtual void Response();
};

#endif /* _WEBAPI_H_ */
