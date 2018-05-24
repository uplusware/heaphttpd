#ifndef _HTTP2_STREAM_H_
#define _HTTP2_STREAM_H_
#include "backend_session.h"
#include "http.h"
#include "http2.h"
#include "hpack.h"

class CHttp;
class CHttp2;

enum stream_state_e
{
        stream_idle = 0,
        stream_open,
        stream_reserved_local,
        stream_reserved_remote,
        stream_half_closed_remote,
        stream_half_closed_local,
        stream_closed
};
class http2_stream
{
public:
    http2_stream(int epoll_fd, map<int, backend_session*>* backend_list, uint_32 stream_ind, uint_32 local_window_size, uint_32 peer_window_size, CHttp2* phttp2, time_t connection_first_request_time, time_t connection_keep_alive_timeout, unsigned int connection_keep_alive_request_tickets, http_tunneling* tunneling, ServiceObjMap* srvobj, int sockfd,
        const char* servername, unsigned short serverport,
	    const char* clientip, X509* client_cert, memory_cache* ch,
		const char* work_path, vector<string>* default_webpages, vector<http_extension_t>* ext_list, vector<http_extension_t>* reverse_ext_list, const char* php_mode, 
        cgi_socket_t fpm_socktype, const char* fpm_sockfile, 
        const char* fpm_addr, unsigned short fpm_port, const char* phpcgi_path,
        map<string, cgi_cfg_t>* cgi_list,
        const char* private_path, AUTH_SCHEME wwwauth_scheme = asNone, AUTH_SCHEME proxyauth_scheme = asNone,
		SSL* ssl = NULL);
        
    virtual ~http2_stream();
    
    int hpack_parse(HTTP2_Header_Field* field, int len);
    Http_Connection http1_parse(const char* text);
    
    CHttp* GetHttp1();
    hpack* GetHpack();
    Http_Method GetMethod();
    void PushPostData(const char* buf, int len);
    void Response();
    
    void ClearHpack();
    
    void BuildPushPromiseResponse(http2_stream* host_stream, const char* path);
    void SendPushPromiseResponse();
    
    void SetPriorityWeight(uint_32 weight);
    void SetStreamState(stream_state_e state);
    stream_state_e GetStreamState();
    
    void SetDependencyStream(uint_32 dependency_stream);
    uint_32 GetDependencyStream();
    
    void RefreshLastUsedTime();
    time_t GetLastUsedTime();
    
    void IncreasePeerWindowSize(uint_32 window_size);
    void DecreasePeerWindowSize(uint_32 window_size);

    void IncreaseLocalWindowSize(uint_32 window_size);
    void DecreaseLocalWindowSize(uint_32 window_size);
    
    int GetPeerWindowSize();
    int GetLocalWindowSize();

    string m_path;
    string m_method;
    string m_authority;
    string m_scheme;
    string m_status;
    
private:
    CHttp2* m_http2;
    CHttp* m_http1;
    hpack* m_hpack;
    
    int m_epoll_fd;
    
    uint_32 m_stream_ind;
    ServiceObjMap * m_srvobj;
    int m_sockfd;
    string m_servername;
    unsigned short m_serverport;
    string m_clientip;
    X509* m_client_cert;
    memory_cache* m_ch;
    string m_work_path;
    vector<http_extension_t>* m_ext_list;
    vector<http_extension_t>* m_reverse_ext_list;
    vector<string>* m_default_webpages;
    string m_php_mode;
    cgi_socket_t m_fpm_socktype;
    string m_fpm_sockfile;
    string m_fpm_addr;
    unsigned short m_fpm_port;
    string m_phpcgi_path;

    map<string, cgi_cfg_t>* m_cgi_list;
    string m_private_path;
    AUTH_SCHEME m_wwwauth_scheme;
    AUTH_SCHEME m_proxyauth_scheme;
    SSL* m_ssl;
    
    string m_push_promise_trigger_header;
    
    uint_32 m_dependency_stream;
    uint_32 m_priority_weight;
    
    stream_state_e m_stream_state;
    
    int m_peer_window_size;
    int m_local_window_size;
    
    time_t m_last_used_time;
    
    time_t m_connection_first_request_time;
    time_t m_connection_keep_alive_timeout;
    unsigned int m_connection_keep_alive_request_tickets;
    
    http_tunneling* m_http_tunneling;
    map<int, backend_session*>* m_backend_list;
    
};

#endif /*_HTTP2_STREAM_H_*/