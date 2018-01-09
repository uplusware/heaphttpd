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

class ApiMySQLDemo : public web_api
{
public:
	ApiMySQLDemo(http_request* request, http_response *response) : web_api(request, response)
	{}
	
	virtual ~ApiMySQLDemo() {}
	
	virtual void Response();	
};

extern "C" void* api_mysqldemo_response(http_request* request, http_response *response);

#endif /* _API_LOGIN_H_ */
