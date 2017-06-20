/*
	Copyright (c) openheap, uplusware
	uplusware@gmail.com
*/

#include <stdio.h>
#include "extsample.h"

void* ext_request(CHttp* http_session, const char * name, const char * description, const char * parameters, BOOL * skip)
{
    printf("%s %s %s %s\n", __FILE__, __FUNCTION__, name, description);
}

void* ext_response(CHttp* http_session, const char * name, const char * description, const char * parameters, Htdoc* doc)
{
    printf("%s %s %s %s\n", __FILE__, __FUNCTION__, name, description);
}

void* ext_finish(CHttp* http_session, const char * name, const char * description, const char * parameters, Htdoc* doc)
{
    printf("%s %s %s %s\n", __FILE__, __FUNCTION__, name, description);
}