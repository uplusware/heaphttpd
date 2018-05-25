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
#include "cache.h"
#include "base.h"
#include "util/general.h"
#include "httpcomm.h"
#include "backend_session.h"

enum HTTPTunneling
{
    HTTP_Tunneling_None = 0,
    HTTP_Tunneling_Without_CONNECT,
    HTTP_Tunneling_Without_CONNECT_SSL,
    HTTP_Tunneling_With_CONNECT
};

class CHttp;

#include "http.h"

class http_client;

#include "http_client.h"

#include <arpa/inet.h>
 
using namespace std;

enum Tunneling_State
{
	TunnlingNew = 0,
	TunnlingConnecting = TunnlingNew,
    TunnlingConnectingWaiting,
	TunnlingConnected,
	TunnlingEstablished,
    TunnlingSendingReqToBackend = TunnlingEstablished,
    TunnlingSentReqToBackend,
    TunnlingRelayingData1 = TunnlingSentReqToBackend,
    TunnlingRelayingData2,
    TunnlingRelayingData3,
    TunnlingRelayingDataComplete,
	TunnlingComplete,
    TunnlingError = TunnlingComplete
};

#define BUFFER_DESCR_BUF_LEN (4096*3)

typedef struct
{
    char* buf;
    int buf_len;
    int r_pos;
    int w_pos;
} Buffer_Descr;

class http_tunneling : public backend_session
{
public:
    http_tunneling(int epoll_fd, map<int, backend_session*>* backend_list, int client_socked, SSL* client_ssl, HTTPTunneling type, BOOL request_no_cache, memory_cache* cache);
        
    virtual ~http_tunneling();
    
    virtual int processing();
    virtual int async_processing();
    virtual int backend_async_recv();
    virtual int backend_async_flush();
    
    int client_send(const char* buf, int len);
    int client_flush();
    
    int backend_send(const char* buf, int len);
    int backend_flush();
    
    void backend_close();
    
    bool connect_backend(const char* szAddr, unsigned short nPort, const char* http_url,
        const char* szAddrBackup1, unsigned short nPortBackup1, const char* http_url_backup1,
        const char* szAddrBackup2, unsigned short nPortBackup2, const char* http_url_backup2,
        BOOL request_no_cache);
    
    bool send_request_to_backend(const char* hbuf, int hlen, const char* dbuf, int dlen);
    bool recv_response_from_backend_relay_to_client(CHttpResponseHdr* session_response_header);
    bool acquire_cache_relay_to_client(CHttpResponseHdr* session_response_header);
    void relay_processing();
    
    void force_tunneling_close();
    
    Tunneling_State get_tunneling_state() { return m_tunneling_state; }
    void set_tunneling_state(Tunneling_State state) { m_tunneling_state = state; }
    int backend_recv(char* buf, int len);
    
    int append_backend_session();
    int remove_backend_session();
    int backend_session_writeable(bool writeable);
    int client_session_writeable(bool writeable);
    
    void set_http_session_data(CHttp* http_session, CHttpResponseHdr* session_response_header, const char* req_header, int req_header_len, const char* req_data, int req_data_len,
        const char* backend_addr, unsigned short backend_port, const char* http_url,
        const char* backend_addr_backup1, unsigned short backend_port_backup1, const char* http_url_backup1,
        const char* backend_addr_backup2, unsigned short backend_port_backup2, const char* http_url_backup2);
    
    int get_backend_sockfd() { return m_backend_sockfd; }
    
    CHttp * get_http() { return m_http; }
    
    bool connect_backend(const char* backend_addr, unsigned short backend_port, const char* http_url, BOOL request_no_cache);
    
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
    memory_cache* m_cache;
    string m_http_tunneling_url;
    tunneling_cache* m_tunneling_cache_instance;
        
    char* m_client_send_buf;
    int m_client_send_buf_len;
    int m_client_send_buf_used_len;
    
    char* m_backend_send_buf;
    int m_backend_send_buf_len;
    int m_backend_send_buf_used_len;
	
        
    char* m_async_backend_recv_buf;
    int m_async_backend_recv_data_len;
    int m_async_backend_recv_buf_size;
        
	http_client * m_client;
    map<int, backend_session*>* m_backend_list;
    int m_epoll_fd;
    
    Tunneling_State m_tunneling_state;
    
    const char* m_req_header;
    int m_req_header_len;
    const char* m_req_data;
    int m_req_data_len;
    
    string m_backend_addr;
    unsigned short m_backend_port;
    string m_http_url;
    string m_backend_addr_backup1;
    unsigned short m_backend_port_backup1;
    string m_http_url_backup1;
    string m_backend_addr_backup2;
    unsigned short m_backend_port_backup2;
    string m_http_url_backup2;
    BOOL m_request_no_cache;
    CHttpResponseHdr* m_session_response_header;
    CHttp * m_http;
};
#endif /* _TUNNELING_H_ */