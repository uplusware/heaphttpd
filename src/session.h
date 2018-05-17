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
#include "tunneling.h"

class Session_Group;

typedef enum
{
	stHTTP = 1,
} Service_Type;

class Session
{
protected:
	int m_sockfd;
    int m_epoll_fd;
    SSL * m_ssl;
	string m_clientip;
	BOOL m_https;
	BOOL m_http2;
    ServiceObjMap * m_srvobj;
    X509* m_client_cert;
    http_tunneling* m_http_tunneling;
    
    AUTH_SCHEME m_wwwauth_scheme;
    AUTH_SCHEME m_proxyauth_scheme;
    
    IHttp * m_http_protocol_instance;
    unsigned int m_connection_keep_alive_tickets;
    time_t m_first_connection_request_time;
    
    Session_Group* m_group;
    
    void CreateProtocol();
    
public:
	Session(Session_Group* group, int epoll_fd, ServiceObjMap* srvobj, int sockfd, SSL* ssl, const char* clientip, X509* client_cert, BOOL https, BOOL http2, memory_cache* ch);
	virtual ~Session();
	
    
	void Processing();
    
    Http_Connection AsyncProcessing();
    
    int AsyncRecv()
    {
        if(m_http_protocol_instance)
            return m_http_protocol_instance->AsyncRecv();
        else
            return -1;
    }
    
    int AsyncSend()
    {
        if(m_http_protocol_instance)
            return m_http_protocol_instance->AsyncSend();
        else
            return -1;
    }
    
    int get_sock_fd() { return m_sockfd; }
	memory_cache* m_cache;
};
#endif /* _SESSION_H_*/

