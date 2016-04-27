/*
	Copyright (c) openheap, uplusware
	uplusware@gmail.com
*/

#ifndef _WEBAPI_H_
#define _WEBAPI_H_

#include "http.h"
#include "httpcomm.h"
#include "util/escape.h"

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

};

#endif /* _WEBAPI_H_ */
