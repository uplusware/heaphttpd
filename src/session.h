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
#include "serviceobj.h"
#include "util/trace.h"

typedef enum
{
	stHTTP = 1,
	stHTTPS,
} Service_Type;

class Session
{
protected:
	int m_sockfd;
    SSL * m_ssl;
	string m_clientip;
	Service_Type m_st;
	BOOL m_http2;
    ServiceObjMap * m_srvobj;
    X509* m_client_cert;
public:
	Session(ServiceObjMap* srvobj, int sockfd, SSL* ssl, const char* clientip, X509* client_cert, Service_Type st, BOOL http2, memory_cache* ch);
	virtual ~Session();
	
	void Process();

	memory_cache* m_cache;
};
#endif /* _SESSION_H_*/

