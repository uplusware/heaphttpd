/*
	Copyright (c) openheap, uplusware
	uplusware@gmail.com
*/

#ifndef _API_LOGIN_H_
#define _API_LOGIN_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "http.h"
#include "webdoc.h"
#include "niuapi.h"
#include "stgengine.h"

class ApiMySQLDemo : public doc
{
public:
	ApiMySQLDemo(CHttp* session, const char * html_path)  : doc(session, html_path)
	{}
	
	virtual ~ApiMySQLDemo() {}
	
	virtual void Response();	
};

extern "C" void* api_mysqldemo_response(CHttp* m_session, const char * html_path);

#endif /* _API_LOGIN_H_ */
