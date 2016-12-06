#ifndef _HTTP2_STREAM_H_
#define _HTTP2_STREAM_H_

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
    http2_stream(uint_32 stream_ind, uint_32 local_window_size, uint_32 peer_window_size, CHttp2* phttp2, ServiceObjMap* srvobj, int sockfd,
        const char* servername, unsigned short serverport,
	    const char* clientip, X509* client_cert, memory_cache* ch,
		const char* work_path, vector<stExtension>* ext_list, const char* php_mode, 
        const char* fpm_socktype, const char* fpm_sockfile, 
        const char* fpm_addr, unsigned short fpm_port, const char* phpcgi_path,
        const char* fastcgi_name, const char* fastcgi_pgm, 
        const char* fastcgi_socktype, const char* fastcgi_sockfile,
        const char* fastcgi_addr, unsigned short fastcgi_port,
        const char* private_path, AUTH_SCHEME wwwauth_scheme = asNone,
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
        
    uint_32 m_stream_ind;
    ServiceObjMap * m_srvobj;
    int m_sockfd;
    string m_servername;
    unsigned short m_serverport;
    string m_clientip;
    X509* m_client_cert;
    memory_cache* m_ch;
    string m_work_path;
    vector<stExtension>* m_ext_list;
    string m_php_mode;
    string m_fpm_socktype;
    string m_fpm_sockfile;
    string m_fpm_addr;
    unsigned short m_fpm_port;
    string m_phpcgi_path;
    string m_fastcgi_name;
    string m_fastcgi_pgm;
    string m_fastcgi_socktype;
    string m_fastcgi_sockfile;
    string m_fastcgi_addr;
    unsigned short m_fastcgi_port;
    string m_private_path;
    AUTH_SCHEME m_wwwauth_scheme;
    SSL* m_ssl;
    
    string m_push_promise_trigger_header;
    
    uint_32 m_dependency_stream;
    uint_32 m_priority_weight;
    
    stream_state_e m_stream_state;
    
    int m_peer_window_size;
    int m_local_window_size;
    
    time_t m_last_used_time;
};

#endif /*_HTTP2_STREAM_H_*/