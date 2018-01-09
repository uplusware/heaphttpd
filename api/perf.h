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

class ApiPerf : public web_api
{
public:
	ApiPerf(http_request* request, http_response *response) : web_api(request, response)
	{}
	
	virtual ~ApiPerf() {}
	
	virtual void Response();	
};

extern "C" void* api_perf_response(http_request* request, http_response *response);

#endif /* _SAMPLE_H_ */
