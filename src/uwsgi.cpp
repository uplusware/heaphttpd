/*
	Copyright (c) openheap, uplusware
	uplusware@gmail.com
*/
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h> 
#include <sys/types.h> 
#include <netdb.h>
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <arpa/inet.h>
#include <sys/un.h>
#include <sys/syscall.h>
#define gettid() syscall(__NR_gettid) 
#include "uwsgi.h"
#include "util/general.h"

uWSGI::uWSGI(const char* ipaddr, unsigned short port)
    : cgi_base(ipaddr, port)
{
    
}

uWSGI::uWSGI(const char* sock_file)
    : cgi_base(sock_file)
{
}

uWSGI::~uWSGI()
{
    
}