/*
        Copyright (c) openheap, uplusware
        uplusware@gmail.com
*/

#ifndef _WSGI_H_
#define _WSGI_H_

#include "cgi.h"

#define SCGI_VERSION_1 1

class wsgi : public cgi_base {
 public:
  wsgi(const char* ipaddr, unsigned short port);
  wsgi(const char* sock_file);
  virtual ~wsgi();
};

#endif /* _WSGI_H_ */