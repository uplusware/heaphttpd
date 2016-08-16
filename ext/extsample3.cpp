/*
	Copyright (c) openheap, uplusware
	uplusware@gmail.com
*/

#include <stdio.h>
#include "extsample.h"

void* ext_request(CHttp* m_session, const char * action)
{
    printf("%s %s %s\n", __FILE__, __FUNCTION__, action);
}

void* ext_response(CHttp* m_session, const char * action, Htdoc* doc)
{
    printf("%s %s %s\n", __FILE__, __FUNCTION__, action);
}

void* ext_finish(CHttp* m_session, const char * action, Htdoc* doc)
{
    printf("%s %s %s\n", __FILE__, __FUNCTION__, action);
}