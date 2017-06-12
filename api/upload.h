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

class ApiUpload : public doc
{
public:
	ApiUpload(CHttp* session, const char * html_path)  : doc(session, html_path)
	{}
	
	virtual ~ApiUpload() {}
	
	virtual void Response();	
};

extern "C" void* api_upload_response(CHttp* m_session, const char * html_path);

#endif /* _SAMPLE_H_ */
