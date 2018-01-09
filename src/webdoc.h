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

class doc
{
public:
    doc()
    { 
    }
    
    virtual ~doc()
    {    
    }
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
	virtual ~web_api(){}

	virtual void Response() = 0;
protected:
    const char* encode_URI(const char* src, string& dst)
	{
		NIU_URLFORMAT_ENCODE((const unsigned char *) src, dst);
		return dst.c_str();
	}
	
	const char* decode_URI(const char* src, string& dst)
	{
		NIU_URLFORMAT_DECODE((const unsigned char *) src, dst);
		return dst.c_str();
	}
	
	const char* escape_HTML(const char* src, string& dst)
	{
	    dst = src;
		Replace(dst, "&", "&amp;");
		
		Replace(dst, "\r", "");
		Replace(dst, "\n", "<BR>");
		Replace(dst, " ", "&nbsp;");
		Replace(dst, "\t", "&nbsp;&nbsp;&nbsp;&nbsp;");
		
		Replace(dst, "<", "&lt;");
		Replace(dst, ">", "&gt;");
		Replace(dst, "'", "&apos;");
		Replace(dst, "\"", "&quot;");
		return dst.c_str();
	}
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
	virtual ~web_doc(){}

	virtual void Response();
};

#endif /* _WEBAPI_H_ */
