/*
	Copyright (c) openheap, uplusware
	uplusware@gmail.com
*/

#ifndef _WEBAPI_H_
#define _WEBAPI_H_

#include "http.h"
#include "httpcomm.h"
#include "heapapi.h"

class doc
{
protected:
	CHttp * m_session;
    string m_work_path;
	
public:
	doc(CHttp* session, const char* work_path)
	{
		m_session = session;
        m_work_path = work_path;
	}
	virtual ~doc(){}

	virtual void Response();
	
protected:
    const char* encodeURI(const char* src, string& dst)
	{
		NIU_URLFORMAT_ENCODE((const unsigned char *) src, dst);
		return dst.c_str();
	}
	
	const char* decodeURI(const char* src, string& dst)
	{
		NIU_URLFORMAT_DECODE((const unsigned char *) src, dst);
		return dst.c_str();
	}
	
	const char* escapeHTML(const char* src, string& dst)
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

#endif /* _WEBAPI_H_ */
