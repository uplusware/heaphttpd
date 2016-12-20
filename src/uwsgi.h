/*
	Copyright (c) openheap, uplusware
	uplusware@gmail.com
*/

#ifndef _SCGI_H_
#define _SCGI_H_

#include "cgi.h"

#define SCGI_VERSION_1 1

class uWSGI : public cgi_base
{
public:
	uWSGI(const char* ipaddr, unsigned short port);
	uWSGI(const char* sock_file);
	virtual ~uWSGI();
};

#endif /* _SCGI_H_ */