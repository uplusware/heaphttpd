/*
        Copyright (c) openheap, uplusware
        uplusware@gmail.com
*/
#include "uwsgi.h"

uwsgi::uwsgi(const char* ipaddr, unsigned short port)
    : cgi_base(ipaddr, port) {}

uwsgi::uwsgi(const char* sock_file) : cgi_base(sock_file) {}

uwsgi::~uwsgi() {}