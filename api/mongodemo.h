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
#include "heapapi.h"
#include "stgengine.h"

class ApiMongoDemo : public doc
{
public:
	ApiMongoDemo(CHttp* session, const char * html_path)  : doc(session, html_path)
	{}
	
	virtual ~ApiMongoDemo() {}
	
	virtual void Response();	
};

extern "C" void* api_mongodemo_response(CHttp* m_session, const char * html_path);

#endif /* _API_LOGIN_H_ */
