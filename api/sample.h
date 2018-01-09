/*
	Copyright (c) openheap, uplusware
	uplusware@gmail.com
*/

#ifndef _SAMPLE_H_
#define _SAMPLE_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "http.h"
#include "webdoc.h"
#include "heapapi.h"

class ApiSample : public web_api
{
public:
	ApiSample(http_request* request, http_response *response) : web_api(request, response)
	{

    }
	
	virtual ~ApiSample() {}
	
	virtual void Response();	
};

extern "C" void* api_sample_response(http_request* request, http_response *response);

#endif /* _SAMPLE_H_ */
