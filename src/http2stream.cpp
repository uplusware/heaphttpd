#include "http2stream.h"
#include "debug.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////
//http2_stream
http2_stream::http2_stream(int epoll_fd, map<int, backend_session*>* backend_list, uint_32 stream_ind, uint_32 local_window_size, uint_32 peer_window_size, CHttp2* phttp2, time_t connection_first_request_time, time_t connection_keep_alive_timeout, unsigned int connection_keep_alive_request_tickets,
        http_tunneling* tunneling, fastcgi* php_fpm_instance, map<string, fastcgi*>* fastcgi_instances, map<string, scgi*>* scgi_instances, ServiceObjMap* srvobj, int sockfd,
        const char* servername, unsigned short serverport,
	    const char* clientip, X509* client_cert, memory_cache* ch,
		const char* work_path, vector<string>* default_webpages, vector<http_extension_t>* ext_list, vector<http_extension_t>* reverse_ext_list, const char* php_mode, 
        cgi_socket_t fpm_socktype, const char* fpm_sockfile, 
        const char* fpm_addr, unsigned short fpm_port, const char* phpcgi_path,
        map<string, cgi_cfg_t>* cgi_list,
        const char* private_path, AUTH_SCHEME wwwauth_scheme, AUTH_SCHEME proxyauth_scheme,
		SSL* ssl)
{
    m_backend_list = backend_list;
    m_connection_first_request_time = connection_first_request_time;
    m_connection_keep_alive_timeout = connection_keep_alive_timeout;
    m_connection_keep_alive_request_tickets = connection_keep_alive_request_tickets;
    
    m_http_tunneling = tunneling;
    m_php_fpm_instance = php_fpm_instance;
    m_fastcgi_instances = fastcgi_instances;
    m_scgi_instances = scgi_instances;
    
    m_path = "";
    m_method = "";
    m_authority = "";
    m_scheme = "";
    
    m_stream_ind = stream_ind;
    m_srvobj = srvobj;
    m_sockfd = sockfd;
    m_epoll_fd = epoll_fd;
    m_servername = servername;
    m_serverport = serverport;
    m_clientip = clientip;
    m_client_cert = client_cert;
    m_ch = ch;
    m_work_path = work_path;
    m_ext_list = ext_list;
    m_reverse_ext_list = reverse_ext_list;
    m_php_mode = php_mode;
    m_fpm_socktype = fpm_socktype;
    m_fpm_sockfile = fpm_sockfile;
    m_fpm_addr = fpm_addr;
    m_fpm_port = fpm_port;
    m_phpcgi_path = phpcgi_path;
    
    m_cgi_list = cgi_list;
    m_default_webpages = default_webpages;
    m_private_path = private_path;
    m_wwwauth_scheme = wwwauth_scheme;
    m_proxyauth_scheme = proxyauth_scheme;
    m_ssl = ssl;
    
    m_http2 = phttp2;
    
    m_http1 = new CHttp(m_epoll_fd, m_backend_list, m_connection_first_request_time, m_connection_keep_alive_timeout, m_connection_keep_alive_request_tickets,
                            m_http_tunneling, m_php_fpm_instance, m_fastcgi_instances, m_scgi_instances,
                            m_srvobj,
                            m_sockfd,
                            m_servername.c_str(),
                            m_serverport,
                            m_clientip.c_str(),
                            m_client_cert,
                            m_ch,
                            m_work_path.c_str(),
                            m_default_webpages,
                            m_ext_list,
                            m_reverse_ext_list,
                            m_php_mode.c_str(),
                            m_fpm_socktype,
                            m_fpm_sockfile.c_str(),
                            m_fpm_addr.c_str(),
                            m_fpm_port,
                            m_phpcgi_path.c_str(),
                            m_cgi_list,
                            m_private_path.c_str(),
                            m_wwwauth_scheme,
                            m_proxyauth_scheme,
                            m_ssl, m_http2, m_stream_ind);
    m_hpack = NULL;
    
    m_push_promise_trigger_header = "";
    
    m_dependency_stream = 0;
    m_priority_weight = 16;
    m_stream_state = stream_idle;
    
    m_peer_window_size = peer_window_size;
    m_local_window_size = local_window_size;
    
    m_last_used_time = time(NULL);
}

http2_stream::~http2_stream()
{
    m_stream_state = stream_closed;
    
    if(m_http1)
        delete m_http1;
    if(m_hpack)
        delete m_hpack;
    m_http1 = NULL;
    m_hpack = NULL;
}

void http2_stream::BuildPushPromiseResponse(http2_stream* host_stream, const char* path)
{
    m_push_promise_trigger_header = "GET ";
    m_push_promise_trigger_header += path;
    m_push_promise_trigger_header += " HTTP/1.1\r\n";
    
    if(strcmp(host_stream->GetHttp1()->GetRequestField("Accept"), "") != 0)
    {
        m_push_promise_trigger_header += "Accept: ";
        m_push_promise_trigger_header += host_stream->GetHttp1()->GetRequestField("Accept");
        m_push_promise_trigger_header += "\r\n";
    }
    if(strcmp(host_stream->GetHttp1()->GetRequestField("Accept-Encoding"), "") != 0)
    {
        m_push_promise_trigger_header += "Accept-Encoding: ";
        m_push_promise_trigger_header += host_stream->GetHttp1()->GetRequestField("Accept-Encoding");
        m_push_promise_trigger_header += "\r\n";
    }
    if(strcmp(host_stream->GetHttp1()->GetRequestField("Accept-Language"), "") != 0)
    {
        m_push_promise_trigger_header += "Accept-Language: ";
        m_push_promise_trigger_header += host_stream->GetHttp1()->GetRequestField("Accept-Language");
        m_push_promise_trigger_header += "\r\n";
    }
    if(strcmp(host_stream->GetHttp1()->GetRequestField("User-Agent"), "") != 0)
    {
        m_push_promise_trigger_header += "User-Agent: ";
        m_push_promise_trigger_header += host_stream->GetHttp1()->GetRequestField("User-Agent");
        m_push_promise_trigger_header += "\r\n";
    }
    m_push_promise_trigger_header += "\r\n";
}

void http2_stream::SendPushPromiseResponse()
{
    if(m_push_promise_trigger_header != "")
        http1_parse(m_push_promise_trigger_header.c_str());
    m_push_promise_trigger_header = "";
}

int http2_stream::hpack_parse(HTTP2_Header_Field* field, int len)
{
    if(!m_hpack)
        m_hpack = new hpack();
    return m_hpack->parse(field, len);
}

Http_Connection http2_stream::http1_parse(const char* text)
{
    return m_http1->LineParse(text);
}

CHttp* http2_stream::GetHttp1()
{
    return m_http1;
}

hpack* http2_stream::GetHpack()
{
    return m_hpack;
}

Http_Method http2_stream::GetMethod()
{
    return m_http1->GetMethod();
}

BOOL http2_stream::PushPostData(const char* buf, int len)
{
    m_http1->SetHttpStatus(httpReqData);
    return m_http1->PushPostData(buf, len);
}

void http2_stream::Response()
{
    m_http1->SetHttpStatus(httpAuthentication);
    m_http1->ResponseReply();
}

void http2_stream::ClearHpack()
{
    if(m_hpack)
        delete m_hpack;
    m_hpack = NULL;
}

void http2_stream::SetPriorityWeight(uint_32 weight)
{
    m_priority_weight = weight;
}

void http2_stream::SetStreamState(stream_state_e state)
{
    m_stream_state = state;
}

stream_state_e http2_stream::GetStreamState()
{
    return m_stream_state;
}

void http2_stream::SetDependencyStream(uint_32 dependency_stream)
{
    if(dependency_stream == 0)
        m_dependency_stream = dependency_stream;
    else
    {
        http2_stream* p_dependency_stream = m_http2->get_stream_instance(dependency_stream);       
        if(p_dependency_stream)
        {
            m_dependency_stream = dependency_stream;
#ifdef _http2_debug_
            printf("    Set stream[%u] depends on stream[%u]\n", m_stream_ind, m_dependency_stream);
#endif /* _http2_debug_ */ 
        }
        else
        {
            m_dependency_stream = 0;
        }
    }
   
}

uint_32 http2_stream::GetDependencyStream()
{
    return m_dependency_stream;
}

void http2_stream::RefreshLastUsedTime()
{
    m_last_used_time = time(NULL);
#ifdef _http2_debug_
    printf("  Refresh stream(%u) as %u\n", m_stream_ind, m_last_used_time);
#endif /* _http2_debug_ */     
}

void http2_stream::IncreasePeerWindowSize(uint_32 window_size)
{
    if(m_dependency_stream == 0)
    {
        m_peer_window_size += window_size;
#ifdef _http2_debug_
        printf("    Current Stream[%u] Peer Windows Size %d\n", m_stream_ind, m_peer_window_size);
#endif /* _http2_debug_ */ 
    }
    else
    {
        http2_stream* p_dependency_stream = m_http2->get_stream_instance(m_dependency_stream);
        if(p_dependency_stream)
            p_dependency_stream->IncreasePeerWindowSize(window_size);
    }
}

void http2_stream::DecreasePeerWindowSize(uint_32 window_size)
{
    if(m_dependency_stream == 0)
    {
        m_peer_window_size -= window_size;
    #ifdef _http2_debug_
        printf("    Current Stream[%u] Peer Windows Size %d\n", m_stream_ind, m_peer_window_size);
    #endif /* _http2_debug_ */ 
    }
    else
    {
        http2_stream* p_dependency_stream = m_http2->get_stream_instance(m_dependency_stream);      
        if(p_dependency_stream)
        {
            p_dependency_stream->DecreasePeerWindowSize(window_size);
        }
    }
}

void http2_stream::IncreaseLocalWindowSize(uint_32 window_size)
{
    if(m_dependency_stream == 0)
    {
        m_local_window_size += window_size;
    #ifdef _http2_debug_
        printf("    Current Stream[%u] Local Windows Size %d\n", m_stream_ind, m_local_window_size);
    #endif /* _http2_debug_ */ 
    }
    else
    {
        http2_stream* p_dependency_stream = m_http2->get_stream_instance(m_dependency_stream);        
        if(p_dependency_stream)
            p_dependency_stream->IncreaseLocalWindowSize(window_size);
    }
}

void http2_stream::DecreaseLocalWindowSize(uint_32 window_size)
{
    if(m_dependency_stream == 0)
    {
        m_local_window_size -= window_size;
    #ifdef _http2_debug_
        printf("    Current Stream[%u] Local Windows Size %d\n", m_stream_ind, m_local_window_size);
    #endif /* _http2_debug_ */ 
        
        if(m_local_window_size <= 0)
        {
            m_http2->send_window_update(m_stream_ind, m_http2->get_initial_local_window_size());
        }
    }
    else
    {
        http2_stream* p_dependency_stream = m_http2->get_stream_instance(m_dependency_stream);       
        if(p_dependency_stream)
        {
            p_dependency_stream->DecreaseLocalWindowSize(window_size);
        }
    }
}

int http2_stream::GetPeerWindowSize()
{
    if(m_dependency_stream == 0)
    {
        return m_peer_window_size;
    }
    else
    {
        http2_stream* p_dependency_stream = m_http2->get_stream_instance(m_dependency_stream);
        if(p_dependency_stream)
            return p_dependency_stream->GetPeerWindowSize();
    }
}

int http2_stream::GetLocalWindowSize()
{
    if(m_dependency_stream == 0)
    {
        return m_local_window_size;
    }
    else
    {
        http2_stream* p_dependency_stream = m_http2->get_stream_instance(m_dependency_stream);
        if(p_dependency_stream)
            return p_dependency_stream->GetLocalWindowSize();
    }
}

time_t http2_stream::GetLastUsedTime()
{
    return m_last_used_time;
}

vector<string>* http2_stream::GetHttp2PushPromiseList()
{
    return m_http1 ? m_http1->GetHttp2PushPromiseList() : NULL;
}