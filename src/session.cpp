/*
	Copyright (c) openheap, uplusware
	uplusware@gmail.com
*/

#include "session.h"
#include "session_group.h"

Session::Session(Session_Group* group, ServiceObjMap* srvobj, int sockfd, SSL* ssl, const char* clientip, X509* client_cert,
    BOOL https, BOOL http2, memory_cache* ch)
{
    m_group = group;
    m_srvobj = srvobj;
	m_sockfd = sockfd;
    m_epoll_fd = group ? group->get_epoll_fd() : -1;
    m_backend_list = group ? group->get_backend_list() : NULL;
    m_ssl = ssl;
	m_clientip = clientip;
	m_https = https;
	m_http2 = http2;
	m_client_cert = client_cert;
	m_cache = ch;
    m_http_tunneling = NULL;
    m_php_fpm_instance = NULL;
    
    m_wwwauth_scheme = asNone;
    m_proxyauth_scheme = asNone;
    
	if(strcasecmp(CHttpBase::m_www_authenticate.c_str(), "basic") == 0)
	{
		m_wwwauth_scheme = asBasic;
	}
	else if(strcasecmp(CHttpBase::m_www_authenticate.c_str(), "digest") == 0)
	{
		m_wwwauth_scheme = asDigest;
	}
	
    if(strcasecmp(CHttpBase::m_proxy_authenticate.c_str(), "basic") == 0)
	{
		m_proxyauth_scheme = asBasic;
	}
	else if(strcasecmp(CHttpBase::m_proxy_authenticate.c_str(), "digest") == 0)
	{
		m_proxyauth_scheme = asDigest;
	}
    
    m_http_protocol_instance = NULL;
    
    m_connection_keep_alive_tickets = CHttpBase::m_connection_keep_alive_max;
    m_first_connection_request_time = time(NULL);
    
    CreateProtocol();
}

Session::~Session()
{
    if(m_http_protocol_instance)
        delete m_http_protocol_instance;
    m_http_protocol_instance = NULL;
    
    if(m_http_tunneling)
        delete m_http_tunneling;
    m_http_tunneling = NULL;
    
    if(m_php_fpm_instance)
        delete m_php_fpm_instance;
    m_php_fpm_instance = NULL;
    
    map<string, fastcgi*>::iterator iter_instance;
    for(iter_instance = m_fastcgi_instances.begin(); iter_instance != m_fastcgi_instances.end(); iter_instance++)
    {
        delete iter_instance->second;
    }
    
}

void Session::CreateProtocol()
{
    if(!m_http_protocol_instance)
    {
        if(!m_https)
        {
            m_http_protocol_instance = new CHttp(m_epoll_fd, m_backend_list, m_first_connection_request_time, CHttpBase::m_connection_keep_alive_timeout, m_connection_keep_alive_tickets,
                m_http_tunneling, m_php_fpm_instance, &m_fastcgi_instances,
                m_srvobj, m_sockfd, CHttpBase::m_localhostname.c_str(), CHttpBase::m_httpport,
                m_clientip.c_str(), m_client_cert, m_cache,
                CHttpBase::m_work_path.c_str(), &CHttpBase::m_default_webpages, &CHttpBase::m_ext_list, &CHttpBase::m_reverse_ext_list, CHttpBase::m_php_mode.c_str(),
                CHttpBase::m_fpm_socktype, CHttpBase::m_fpm_sockfile.c_str(), 
                CHttpBase::m_fpm_addr.c_str(), CHttpBase::m_fpm_port, CHttpBase::m_phpcgi_path.c_str(),
                &CHttpBase::m_cgi_list,
                CHttpBase::m_private_path.c_str(), m_wwwauth_scheme, m_proxyauth_scheme);
        }
        else if(m_https)
        {
            if(m_http2)
            {
                m_http_protocol_instance = new CHttp2(m_epoll_fd, m_backend_list, m_first_connection_request_time, CHttpBase::m_connection_keep_alive_timeout, m_connection_keep_alive_tickets,
                    m_http_tunneling, m_php_fpm_instance, &m_fastcgi_instances, m_srvobj, m_sockfd, CHttpBase::m_localhostname.c_str(), CHttpBase::m_httpsport,
                    m_clientip.c_str(), m_client_cert, m_cache,
                    CHttpBase::m_work_path.c_str(), &CHttpBase::m_default_webpages, &CHttpBase::m_ext_list, &CHttpBase::m_reverse_ext_list, CHttpBase::m_php_mode.c_str(), 
                    CHttpBase::m_fpm_socktype, CHttpBase::m_fpm_sockfile.c_str(), 
                    CHttpBase::m_fpm_addr.c_str(), CHttpBase::m_fpm_port, CHttpBase::m_phpcgi_path.c_str(),
                    &CHttpBase::m_cgi_list,
                    CHttpBase::m_private_path.c_str(), m_wwwauth_scheme, m_proxyauth_scheme, m_ssl);
            }
            else
            {
                m_http_protocol_instance = new CHttp(m_epoll_fd, m_backend_list, m_first_connection_request_time, CHttpBase::m_connection_keep_alive_timeout, m_connection_keep_alive_tickets,
                    m_http_tunneling, m_php_fpm_instance, &m_fastcgi_instances, m_srvobj, m_sockfd, CHttpBase::m_localhostname.c_str(), CHttpBase::m_httpsport,
                    m_clientip.c_str(), m_client_cert, m_cache,
                    CHttpBase::m_work_path.c_str(), &CHttpBase::m_default_webpages, &CHttpBase::m_ext_list, &CHttpBase::m_reverse_ext_list, CHttpBase::m_php_mode.c_str(), 
                    CHttpBase::m_fpm_socktype, CHttpBase::m_fpm_sockfile.c_str(), 
                    CHttpBase::m_fpm_addr.c_str(), CHttpBase::m_fpm_port, CHttpBase::m_phpcgi_path.c_str(),
                    &CHttpBase::m_cgi_list,
                    CHttpBase::m_private_path.c_str(), m_wwwauth_scheme, m_proxyauth_scheme, m_ssl);
            }
        }
    }
}

Http_Connection Session::AsyncProcessing()
{
    Http_Connection httpConn = httpKeepAlive;
    
    CreateProtocol();
    
    if(m_http_protocol_instance)
    {
        httpConn = m_http_protocol_instance->AsyncProcessing();
        
        if(httpConn == httpClose || httpConn == httpKeepAlive)
        {
            m_http_tunneling  = m_http_protocol_instance->GetHttpTunneling();
            m_php_fpm_instance = m_http_protocol_instance->GetPhpFpm();
            
            if(httpConn == httpKeepAlive || httpConn == httpClose)
            {
                delete m_http_protocol_instance;
                m_http_protocol_instance = NULL;
            }
            
            if(m_connection_keep_alive_tickets > 0)
            {
                m_connection_keep_alive_tickets--;
            }
            
            if(m_connection_keep_alive_tickets == 0 || (time(NULL) - m_first_connection_request_time) > CHttpBase::m_connection_keep_alive_timeout)
            {
                httpConn = httpClose;
            }
        }
    }
    else
    {
        httpConn = httpClose;
    }
    return httpConn;
}

void Session::Processing()
{
    Http_Connection httpConn = httpKeepAlive;
    
    while(httpConn != httpClose)
    {
        CreateProtocol();
        
        if(m_http_protocol_instance)
        {
            httpConn = m_http_protocol_instance->Processing();
            
            m_http_tunneling  = m_http_protocol_instance->GetHttpTunneling();
            m_php_fpm_instance = m_http_protocol_instance->GetPhpFpm();
            
            delete m_http_protocol_instance;
            m_http_protocol_instance = NULL;
            
            if(m_connection_keep_alive_tickets > 0)
            {
                m_connection_keep_alive_tickets--;
            }
            
            if(m_connection_keep_alive_tickets == 0 || (time(NULL) - m_first_connection_request_time) > CHttpBase::m_connection_keep_alive_timeout)
            {
                httpConn = httpClose;
            }
        }
        else
        {
            httpConn = httpClose;
        }
    }
}

