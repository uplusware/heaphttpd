/*
	Copyright (c) openheap, uplusware
	uplusware@gmail.com
*/

#ifndef _TUNNELING_H_
#define _TUNNELING_H_

#include <sys/epoll.h>  
#include <arpa/inet.h>
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <netdb.h>

#include "base.h"
#include "util/general.h"

#include <arpa/inet.h>
 
using namespace std;

enum HTTPTunneling
{
    HTTP_Tunneling_None = 0,
    HTTP_Tunneling_Without_CONNECT,
    HTTP_Tunneling_With_CONNECT
};

class http_tunneling
{
public:
    http_tunneling(int client_socked, const char* szAddr, unsigned short nPort, HTTPTunneling type);
    virtual ~http_tunneling();
    
    bool connect_backend();
    
    bool send_request(const char* hbuf, int hlen, const char* dbuf, int dlen);
    bool recv_relay_reply();
    
    void relay_processing();
protected:
    string m_address;
    unsigned short m_port;
    int m_backend_sockfd;
    int m_client_sockfd;
    
    SSL* m_backend_ssl;
	SSL_CTX* m_backend_ssl_ctx;
    
    SSL* m_client_ssl;
	SSL_CTX* m_client_ssl_ctx;
    
    HTTPTunneling m_type;
};
#endif /* _TUNNELING_H_ */