/*
	Copyright (c) openheap, uplusware
	uplusware@gmail.com
*/

#include <stdio.h>
#include "extsample.h"

void* ext_request(CHttp* http_session, const char * name, const char * description, const char * parameters, BOOL * skip)
{
    fprintf(stderr, "%s %s %s %s\n", __FILE__, __FUNCTION__, name, description);
    return NULL;
}

void* ext_response(CHttp* http_session, const char * name, const char * description, const char * parameters, Htdoc* doc)
{
    fprintf(stderr, "%s %s %s %s\n", __FILE__, __FUNCTION__, name, description);
    return NULL;
}

void* ext_finish(CHttp* http_session, const char * name, const char * description, const char * parameters, Htdoc* doc)
{
    fprintf(stderr, "%s %s %s %s\n", __FILE__, __FUNCTION__, name, description);
    return NULL;
}