/*
	Copyright (c) openheap, uplusware
	uplusware@gmail.com
*/

#ifndef _SESSION_H_
#define _SESSION_H_
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/select.h>
#include "base.h"
#include "http.h"
#include "cache.h"
#include "serviceobjmap.h"
#include "util/trace.h"

typedef enum
{
	stHTTP = 1,
} Service_Type;

class Session
{
protected:
	int m_sockfd;
    SSL * m_ssl;
	string m_clientip;
	BOOL m_https;
	BOOL m_http2;
    ServiceObjMap * m_srvobj;
    X509* m_client_cert;
public:
	Session(ServiceObjMap* srvobj, int sockfd, SSL* ssl, const char* clientip, X509* client_cert, BOOL https, BOOL http2, memory_cache* ch);
	virtual ~Session();
	
	void Process();

	memory_cache* m_cache;
};
#endif /* _SESSION_H_*/

