/*
	Copyright (c) openheap, uplusware
	uplusware@gmail.com
*/

#ifndef _UWSGI_H_
#define _UWSGI_H_

#include "cgi.h"

#define SCGI_VERSION_1 1

class uwsgi : public cgi_base
{
public:
	uwsgi(const char* ipaddr, unsigned short port);
	uwsgi(const char* sock_file);
	virtual ~uwsgi();
};

#endif /* _UWSGI_H_ */