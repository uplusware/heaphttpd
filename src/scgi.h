/*
	Copyright (c) openheap, uplusware
	uplusware@gmail.com
*/

#ifndef _SCGI_H_
#define _SCGI_H_

#include "cgi.h"

#define SCGI_VERSION_1 1

class SimpleCGI : public cgi_base
{
public:
	SimpleCGI(const char* ipaddr, unsigned short port);
	SimpleCGI(const char* sock_file);
	virtual ~SimpleCGI();
    
    int SendRequest(map<string, string> &params_map, const char* postdata, unsigned int postdata_len);
    int RecvResponse(vector<char> &appout);
};

#endif /* _SCGI_H_ */