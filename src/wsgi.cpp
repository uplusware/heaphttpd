/*
	Copyright (c) openheap, uplusware
	uplusware@gmail.com
*/

#include "wsgi.h"

wsgi::wsgi(const char* ipaddr, unsigned short port)
    : cgi_base(ipaddr, port)
{
    
}

wsgi::wsgi(const char* sock_file)
    : cgi_base(sock_file)
{
}

wsgi::~wsgi()
{
    
}