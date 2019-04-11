/*
	Copyright (c) openheap, uplusware
	uplusware@gmail.com
*/
#include "tunneling.h"
#include "httpcomm.h"
#include "version.h"
#include "util/security.h"

http_tunneling::http_tunneling(int epoll_fd, map<int, backend_session*>* backend_list, int client_socked, SSL* client_ssl, HTTPTunneling type, BOOL request_no_cache, memory_cache* cache)
{
    m_address = "";
    m_port = 0;
    m_client_sockfd = client_socked;
    m_client_ssl = client_ssl;
    m_backend_sockfd = -1;
    m_type = type;
    m_cache = cache;
    m_http_tunneling_url = "";
    m_tunneling_cache_instance = NULL;
    
    m_backend_ssl = NULL;
    
    m_client_send_buf = (char*)malloc(4095 + 1);
    m_client_send_buf[4095] = '\0';
    m_client_send_buf_len = 4095;
    m_client_send_buf_used_len = 0;
    
    m_backend_send_buf = (char*)malloc(4095 + 1);
    m_backend_send_buf[4095] = '\0';
    m_backend_send_buf_len = 4095;
    m_backend_send_buf_used_len = 0;
    m_client = NULL;
    m_backend_list = backend_list;
    m_epoll_fd = epoll_fd;
    
    
    m_tunneling_state = TunnlingNew;
    
    m_request_no_cache = request_no_cache;
    
    m_async_backend_recv_buf = NULL;
    m_async_backend_recv_data_len = 0;
    m_async_backend_recv_buf_size = 0;
        
    m_is_http2 = FALSE;
        
}

http_tunneling::~http_tunneling()
{
    if(m_client_send_buf)
        free(m_client_send_buf);
    m_client_send_buf = NULL;
    
    if(m_backend_send_buf)
        free(m_backend_send_buf);
    m_backend_send_buf = NULL;
   
    if(m_async_backend_recv_buf)
        free(m_async_backend_recv_buf);
    m_async_backend_recv_buf = NULL;
    m_async_backend_recv_data_len = 0;
    m_async_backend_recv_buf_size = 0;
    
    backend_close();
	
	if(m_client)
		delete m_client;

}

void http_tunneling::set_http_session_data(CHttp* http_session, CHttpResponseHdr* session_response_header, const char* req_header, int req_header_len, const char* req_data, int req_data_len,
    const char* backend_addr, unsigned short backend_port, const char* http_url,
    const char* backend_addr_backup1, unsigned short backend_port_backup1, const char* http_url_backup1,
    const char* backend_addr_backup2, unsigned short backend_port_backup2, const char* http_url_backup2)
{
    m_http = http_session;
    m_req_header = req_header;
    m_req_header_len = req_header_len;
    m_req_data = req_data;
    m_req_data_len = req_data_len;
    m_session_response_header = session_response_header;
    
    
    m_backend_addr = backend_addr;
    m_backend_port = backend_port;
    m_http_url = http_url;
    m_backend_addr_backup1 = backend_addr_backup1;
    m_backend_port_backup1 = backend_port_backup1;
    m_http_url_backup1 = http_url_backup1,
    m_backend_addr_backup2 = backend_addr_backup2;
    m_backend_port_backup2 = backend_port_backup2;
    m_http_url_backup2 = http_url_backup2;
}

int http_tunneling::async_processing()
{
     return processing();
}

int http_tunneling::processing()
{
    if(m_tunneling_state == TunnlingConnecting)
    {            
        if(!connect_backend(m_backend_addr.c_str(), m_backend_port, m_http_url.c_str(), m_request_no_cache)) //connected    
        {
             if(!connect_backend(m_backend_addr_backup1.c_str(), m_backend_port_backup1, m_http_url_backup1.c_str(), m_request_no_cache)) //connected
             {
                 if(!connect_backend(m_backend_addr_backup2.c_str(), m_backend_port_backup2, m_http_url_backup2.c_str(), m_request_no_cache)) //connected
                 {
                    m_tunneling_state = TunnlingError;
                 }
             }
        }
    }
    
    if(m_tunneling_state == TunnlingConnectingWaiting)
    {
        fd_set backend_mask_r, backend_mask_w; 
        struct timeval timeout; 
        timeout.tv_sec = 0; 
        timeout.tv_usec = 0;
        FD_ZERO(&backend_mask_r);
        FD_ZERO(&backend_mask_w);
        FD_SET(m_backend_sockfd, &backend_mask_r);
        FD_SET(m_backend_sockfd, &backend_mask_w);
        
        int ret_val = select(m_backend_sockfd + 1, &backend_mask_r, &backend_mask_w, NULL, &timeout);
        FD_CLR(m_backend_sockfd, &backend_mask_r);
        FD_CLR(m_backend_sockfd, &backend_mask_w);
        
        if(ret_val > 0)
        {
            m_tunneling_state = TunnlingConnected;
        }
    }
    
    
    if(m_tunneling_state == TunnlingConnected)
    {
        if(m_type == HTTP_Tunneling_Without_CONNECT_SSL)
        {
            string ca_crt_root;
            ca_crt_root = CHttpBase::m_ca_client_base_dir;
            ca_crt_root += "/";
            ca_crt_root += m_address;
            ca_crt_root += "/ca.crt";

            string ca_crt_client;
            ca_crt_client = CHttpBase::m_ca_client_base_dir;
            ca_crt_client += "/";
            ca_crt_client += m_address;
            ca_crt_client += "/client.crt";

            string ca_key_client;
            ca_key_client = CHttpBase::m_ca_client_base_dir;
            ca_key_client += "/";
            ca_key_client += m_address;
            ca_key_client += "/client.key";

            string ca_password_file;
            ca_password_file = CHttpBase::m_ca_client_base_dir;
            ca_password_file += "/";
            ca_password_file += m_address;
            ca_password_file += "/client.pwd";

            struct stat file_stat;
            if(stat(ca_crt_root.c_str(), &file_stat) == 0
                && stat(ca_crt_client.c_str(), &file_stat) == 0 && stat(ca_key_client.c_str(), &file_stat) == 0
                && stat(ca_password_file.c_str(), &file_stat) == 0)
            {
                string ca_password;
                string strPwdEncoded;
                ifstream ca_password_fd(ca_password_file.c_str(), ios_base::binary);
                if(ca_password_fd.is_open())
                {
                    getline(ca_password_fd, strPwdEncoded);
                    ca_password_fd.close();
                    strtrim(strPwdEncoded);
                }
                
                Security::Decrypt(strPwdEncoded.c_str(), strPwdEncoded.length(), ca_password);

                if(connect_ssl(m_backend_sockfd,
                    ca_crt_root.c_str(),
                    ca_crt_client.c_str(), ca_password.c_str(),
                    ca_key_client.c_str(),
                    &m_backend_ssl, &m_backend_ssl_ctx, CHttpBase::m_connection_sync_timeout, &m_is_http2) == FALSE)
                {
                    backend_close();
                    return false;
                }
            }
            else
            {
                if(connect_ssl(m_backend_sockfd, NULL, NULL, NULL, NULL, &m_backend_ssl, &m_backend_ssl_ctx, CHttpBase::m_connection_sync_timeout, &m_is_http2) == FALSE)
                {
                    backend_close();
                    return false;
                }
            }
        }
        else if(m_type == HTTP_Tunneling_With_CONNECT)
        {
            const char* connection_established = "HTTP/1.1 200 Connection Established\r\nProxy-Agent: " VERSION_STRING "\r\n\r\n";
            client_send(connection_established, strlen(connection_established));
        }
        
        m_tunneling_state = TunnlingEstablished;
    }
    
    if(m_tunneling_state >= TunnlingEstablished && m_tunneling_state < TunnlingComplete)
    {    
        if(m_type == HTTP_Tunneling_With_CONNECT)
        {	
            relay_processing();
        }
        else if(m_type == HTTP_Tunneling_Without_CONNECT || m_type == HTTP_Tunneling_Without_CONNECT_SSL)
        {
            if(m_tunneling_state == TunnlingSendingReqToBackend)
            {
               send_request_to_backend(m_req_header, m_req_header_len, m_req_data, m_req_data_len);
            }
            
            if(m_tunneling_state >= TunnlingRelayingData1 && m_tunneling_state <= TunnlingRelayingDataComplete)
            {
                if(m_tunneling_cache_instance)
                {
                    acquire_cache_relay_to_client(m_session_response_header);
                }
                else
                {
                    recv_response_from_backend_relay_to_client(m_session_response_header);
                }
            }
        }
    }
    
    if(m_tunneling_state >= TunnlingComplete)
    {
        if(m_client)
            delete m_client;
        m_client = NULL;
    }
    return 0;
}

int http_tunneling::append_backend_session()
{
    if(m_epoll_fd != -1 && m_backend_list && m_backend_sockfd > 0)
    {
        struct epoll_event event; 
        event.data.fd = m_backend_sockfd;  
        event.events = EPOLLIN | EPOLLOUT | EPOLLHUP | EPOLLERR; 
        int epoll_r = epoll_ctl(m_epoll_fd, EPOLL_CTL_ADD, m_backend_sockfd, &event);
        
        if (epoll_r == -1)  
        {  
            fprintf(stderr, "%s %u# epoll_ctl: %s\n", __FILE__, __LINE__, strerror(errno));
            return -1;
        }
        
        m_backend_list->insert(map<int, backend_session*>::value_type(m_backend_sockfd, this));
    }
    return 0;

}

int http_tunneling::remove_backend_session()
{
    if(m_epoll_fd != -1 && m_backend_list && m_backend_sockfd > 0)
    {
        map<int, backend_session*>::iterator iter = m_backend_list->find(m_backend_sockfd);
        if(iter != m_backend_list->end())
        {           
            m_backend_list->erase(iter);
            
            int epoll_r = epoll_ctl(m_epoll_fd, EPOLL_CTL_DEL, m_backend_sockfd, NULL);
            
            if (epoll_r == -1)  
            {  
                fprintf(stderr, "%s %u# epoll_ctl: %s\n", __FILE__, __LINE__, strerror(errno));
                return -1;
            }
        
        }
        

    }
    return 0;
}

int http_tunneling::backend_session_writeable(bool writeable)
{
    if(m_epoll_fd != -1 && m_backend_list && m_backend_sockfd > 0)
    {
        map<int, backend_session*>::iterator iter = m_backend_list->find(m_backend_sockfd);
        if(iter != m_backend_list->end())
        {
            struct epoll_event event; 
            event.data.fd = m_backend_sockfd;  
            event.events = writeable ? (EPOLLIN | EPOLLOUT | EPOLLHUP | EPOLLERR) : (EPOLLIN | EPOLLHUP | EPOLLERR); 
            int epoll_r = epoll_ctl(m_epoll_fd, EPOLL_CTL_MOD, m_backend_sockfd, &event);
            
            if (epoll_r == -1)  
            {  
                fprintf(stderr, "%s %u# epoll_ctl: %s\n", __FILE__, __LINE__, strerror(errno));
                return -1;
            }
        }
    }
    return 0;
}


int http_tunneling::client_session_writeable(bool writeable)
{
    if(m_epoll_fd != -1 && m_client_sockfd > 0)
    {
        struct epoll_event event; 
        event.data.fd = m_client_sockfd;  
        event.events = writeable ? (EPOLLIN | EPOLLOUT | EPOLLHUP | EPOLLERR) : (EPOLLIN | EPOLLHUP | EPOLLERR); 
        int epoll_r = epoll_ctl(m_epoll_fd, EPOLL_CTL_MOD, m_client_sockfd, &event);
        
        if (epoll_r == -1)  
        {  
            fprintf(stderr, "%s %u# epoll_ctl: %s\n", __FILE__, __LINE__, strerror(errno));
            return -1;
        }
    }
    return 0;
}

void http_tunneling::backend_close()
{
#ifdef _WITH_ASYNC_
    remove_backend_session();
#endif /* _WITH_ASYNC_ */
    
    if(m_backend_sockfd > 0)
        close(m_backend_sockfd);
    m_backend_sockfd = -1;
}

void http_tunneling::force_tunneling_close()
{
#ifdef _WITH_ASYNC_
    remove_backend_session();
#endif /* _WITH_ASYNC_ */
    
    if(m_client_sockfd > 0)
        close(m_client_sockfd);
    
    if(m_backend_sockfd > 0)
        close(m_backend_sockfd);
    
     m_client_sockfd = -1;
     m_backend_sockfd = -1;
}

int http_tunneling::client_flush()
{
#ifdef _WITH_ASYNC_
    return m_http->AsyncHttpFlush();
#else
    if(m_client_send_buf_used_len > 0)
    {
        int sent_len = 0;
        
        if(m_client_ssl)
            sent_len = SSL_write(m_client_ssl, m_client_send_buf, m_client_send_buf_used_len);
        else
            sent_len = send(m_client_sockfd, m_client_send_buf, m_client_send_buf_used_len, 0);
        
        if(sent_len > 0)
        {
             memmove(m_client_send_buf, m_client_send_buf + sent_len, m_client_send_buf_used_len - sent_len);
             m_client_send_buf_used_len -= sent_len;
        }
        else if(sent_len < 0)
        {
            if(m_client_ssl)
            {
                int ret = SSL_get_error(m_client_ssl, sent_len);
                if(ret != SSL_ERROR_WANT_READ && ret != SSL_ERROR_WANT_WRITE)
                {
                    m_client_send_buf_used_len = 0;
                    return -1;
                }
            }
            else
            {
                if(errno != EAGAIN)
                {
                    m_client_send_buf_used_len = 0;
                    return -1;
                }
            }
            
        }
        else
        {
            m_client_send_buf_used_len = 0;
            return -1;
        }
    }
    
    return m_client_send_buf_used_len;
#endif /* _WITH_ASYNC_ */
}

int http_tunneling::backend_flush()
{
    if(m_backend_send_buf_used_len > 0)
    {
        int sent_len = 0;
        
        if(m_backend_ssl)
            sent_len = SSL_write(m_backend_ssl, m_backend_send_buf, m_backend_send_buf_used_len);
        else
            sent_len = send(m_backend_sockfd, m_backend_send_buf, m_backend_send_buf_used_len, 0);
        
        if(sent_len > 0)
        {
             memmove(m_backend_send_buf, m_backend_send_buf + sent_len, m_backend_send_buf_used_len - sent_len);
             m_backend_send_buf_used_len -= sent_len;
           
        }
        else if(sent_len < 0)
        {
            if(m_backend_ssl)
            {
                int ret = SSL_get_error(m_backend_ssl, sent_len);
                if(ret != SSL_ERROR_WANT_READ && ret != SSL_ERROR_WANT_WRITE)
                {
                    m_backend_send_buf_used_len = 0;
                    return -1;
                }
            }
            else
            {
                if(errno != EAGAIN)
                {
                    m_backend_send_buf_used_len = 0;
                    return -1;

                }
            }
        }
        else
        {
            m_backend_send_buf_used_len = 0;
            return -1;
        }
    }
#ifdef _WITH_ASYNC_    
    if(m_backend_send_buf_used_len > 0)
    {
        backend_session_writeable(true);
    }
    else
    {
        backend_session_writeable(false);
    }
#endif /* _WITH_ASYNC_ */
    return m_backend_send_buf_used_len;
}

int http_tunneling::client_send(const char* buf, int len)
{
#ifdef _WITH_ASYNC_
    return m_http->AsyncHttpSend(buf, len);
#else
    //Concatenate the send buffer
    if(m_client_send_buf_len - m_client_send_buf_used_len < len)
    {
        char* new_buf = (char*)malloc(m_client_send_buf_used_len + len + 1);
        new_buf[m_client_send_buf_used_len + len] = '\0';
        
        memcpy(new_buf, m_client_send_buf, m_client_send_buf_used_len);
        
        free(m_client_send_buf);
        
        m_client_send_buf = new_buf;
        m_client_send_buf_len = m_client_send_buf_used_len + len;
    }
    memcpy(m_client_send_buf + m_client_send_buf_used_len, buf, len);
    m_client_send_buf_used_len += len;
	
	int sent_len = 0;
	
	if(m_client_send_buf_used_len >= 4096)
	{
		if(m_client_ssl)
			sent_len = SSLWrite(m_client_sockfd, m_client_ssl, m_client_send_buf, m_client_send_buf_used_len, CHttpBase::m_connection_idle_timeout);
		else
			sent_len = _Send_( m_client_sockfd, m_client_send_buf, m_client_send_buf_used_len, CHttpBase::m_connection_idle_timeout);
		
		m_client_send_buf_used_len = 0;
	}
	else
	{
		if(m_client_ssl)
			sent_len = SSL_write(m_client_ssl, m_client_send_buf, m_client_send_buf_used_len);
		else
			sent_len = send( m_client_sockfd, m_client_send_buf, m_client_send_buf_used_len, 0);
        
		if(sent_len > 0)
		{
			 memmove(m_client_send_buf, m_client_send_buf + sent_len, m_client_send_buf_used_len - sent_len);
			 m_client_send_buf_used_len -= sent_len;        
		}
        else if(sent_len < 0)
        {
            if(m_client_ssl)
            {
                int ret = SSL_get_error(m_client_ssl, sent_len);
                if(ret != SSL_ERROR_WANT_READ && ret != SSL_ERROR_WANT_WRITE)
                {
                    m_client_send_buf_used_len = 0;
                }
            }
            else
            {
                if(errno != EAGAIN)
                {
                    m_client_send_buf_used_len = 0;
                }
            }
        }
        else
        {
            m_client_send_buf_used_len = 0;
        }
    }
    return sent_len;
#endif /* _WITH_ASYNC_ */
}

int http_tunneling::backend_send(const char* buf, int len)
{
    //Concatenate the send buffer
    if(m_backend_send_buf_len - m_backend_send_buf_used_len < len)
    {
        char* new_buf = (char*)malloc(m_backend_send_buf_used_len + len + 1);
        new_buf[m_backend_send_buf_used_len + len] = '\0';
        
        memcpy(new_buf, m_backend_send_buf, m_backend_send_buf_used_len);
        
        free(m_backend_send_buf);
        
        m_backend_send_buf = new_buf;
        m_backend_send_buf_len = m_backend_send_buf_used_len + len;
    }
    memcpy(m_backend_send_buf + m_backend_send_buf_used_len, buf, len);
    m_backend_send_buf_used_len += len;

	int sent_len = 0;
#ifndef _WITH_ASYNC_	
	if(m_backend_send_buf_used_len >= 4096)
	{
		if(m_backend_ssl)
			sent_len = SSLWrite(m_backend_sockfd, m_backend_ssl, m_backend_send_buf, m_backend_send_buf_used_len, CHttpBase::m_connection_idle_timeout);
		else
			sent_len = _Send_( m_backend_sockfd, m_backend_send_buf, m_backend_send_buf_used_len, CHttpBase::m_connection_idle_timeout);
		
		m_backend_send_buf_used_len = 0;
	}
	else
#endif /* _WITH_ASYNC_ */
	{
		if(m_backend_ssl)
			sent_len = SSL_write(m_backend_ssl, m_backend_send_buf, m_backend_send_buf_used_len);
		else
			sent_len = send( m_backend_sockfd, m_backend_send_buf, m_backend_send_buf_used_len, 0);
		
		if(sent_len > 0)
		{
			 memmove(m_backend_send_buf, m_backend_send_buf + sent_len, m_backend_send_buf_used_len - sent_len);
			 m_backend_send_buf_used_len -= sent_len;        
		}
        else if(sent_len < 0)
        {
            if(m_backend_ssl)
            {
                int ret = SSL_get_error(m_backend_ssl, sent_len);
                if(ret != SSL_ERROR_WANT_READ && ret != SSL_ERROR_WANT_WRITE)
                {
                    m_backend_send_buf_used_len = 0;
                }
            }
            else
            {
                if(errno != EAGAIN)
                {
                    m_backend_send_buf_used_len = 0;
                }
            }
        }
    }
    
    return sent_len;
}

bool http_tunneling::connect_backend(const char* backend_addr, unsigned short backend_port, const char* http_url, BOOL request_no_cache)
{
    if(CHttpBase::m_enable_http_tunneling_cache && !request_no_cache && http_url && *http_url != '\0')
    {
        m_cache->rdlock_tunneling_cache();
        m_cache->_find_tunneling_(http_url, &m_tunneling_cache_instance);
        m_cache->unlock_tunneling_cache();
        
        if(m_tunneling_cache_instance)
        {
            m_tunneling_state = TunnlingConnected;
            return true;
        }
    }
    
	if(m_address == backend_addr && m_port == backend_port && m_backend_sockfd > 0)
    {
        m_tunneling_state = TunnlingConnected;
		return true;
    }
    
    //connect to the new address:port
    if(*backend_addr == '\0' || backend_port == 0)
        return false;
    
    /* Get the IP from the name */
    char backhost_ip[INET6_ADDRSTRLEN];
    struct addrinfo hints;      
    struct addrinfo *servinfo, *curr;  
    struct sockaddr_in *sa;
    struct sockaddr_in6 *sa6;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_flags = AI_CANONNAME; 
    
    bool success_getaddrinfo = false;
    for(;;)
    {
        int rval = getaddrinfo(backend_addr, NULL, &hints, &servinfo);
        if (rval != 0)
        {
            if(rval == EAI_AGAIN)
                continue;         
            break;
        }
        else
        {
            success_getaddrinfo = true;
            break;
        }
    }
    
    if(!success_getaddrinfo)
        return false;
    
    bool found_ip = false;
    curr = servinfo; 
    while (curr && curr->ai_canonname)
    {  
        if(servinfo->ai_family == AF_INET6)
        {
            sa6 = (struct sockaddr_in6 *)curr->ai_addr;  
            inet_ntop(AF_INET6, (void*)&sa6->sin6_addr, backhost_ip, sizeof (backhost_ip));
            found_ip = TRUE;
        }
        else if(servinfo->ai_family == AF_INET)
        {
            sa = (struct sockaddr_in *)curr->ai_addr;  
            inet_ntop(AF_INET, (void*)&sa->sin_addr, backhost_ip, sizeof (backhost_ip));
            found_ip = TRUE;
        }
        curr = curr->ai_next;
    }     

    freeaddrinfo(servinfo);
    
    if(found_ip == false)
    {
        fprintf(stderr, "couldn't find ip for %s\n", backend_addr);
        return false; //to to next backup
    }
    
    int res; 
    
    /* struct addrinfo hints; */
    struct addrinfo *server_addr, *rp;
    
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
    hints.ai_socktype = SOCK_STREAM; /* Datagram socket */
    hints.ai_flags = AI_PASSIVE;    /* For wildcard IP address */
    hints.ai_protocol = 0;          /* Any protocol */
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;
    
    char szPort[32];
    sprintf(szPort, "%u", backend_port);
    
    success_getaddrinfo = false;
    for(;;)
    {
        int rval = getaddrinfo((backhost_ip && backhost_ip[0] != '\0') ? backhost_ip : NULL, szPort, &hints, &server_addr);
        if (rval != 0)
        {  
           if(rval == EAI_AGAIN)
               continue;
            string strError = backhost_ip;
            strError += ":";
            strError += szPort;
            strError += " ";
            strError += strerror(errno);
            
            fprintf(stderr, "%s(line:%s): %s\n", __FILE__, __LINE__, strError.c_str());
            break;
        }
        else
        {
            success_getaddrinfo = true;
            break;
        }
    }
    
    if(!success_getaddrinfo)
        return false;
    
    bool connected = false;
    
    for (rp = server_addr; rp != NULL; rp = rp->ai_next)
    {
        m_backend_sockfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (m_backend_sockfd == -1)
            continue;
       
        int flags = fcntl(m_backend_sockfd, F_GETFL, 0); 
        fcntl(m_backend_sockfd, F_SETFL, flags | O_NONBLOCK);
        
        int s = connect(m_backend_sockfd, rp->ai_addr, rp->ai_addrlen);
        if(s == 0 || (s < 0 && errno == EINPROGRESS))
        {
#ifdef _WITH_ASYNC_
            append_backend_session();
            
            m_address = backend_addr;
            m_port = backend_port;
            m_http_tunneling_url = http_url;
            
            m_tunneling_state = TunnlingConnectingWaiting;
            
            return true;
#else
            fd_set backend_mask_r, backend_mask_w; 
            struct timeval timeout; 
            timeout.tv_sec = CHttpBase::m_connection_sync_timeout;  
            timeout.tv_usec = 0;
            
            FD_ZERO(&backend_mask_r);
            FD_ZERO(&backend_mask_w);
        
            FD_SET(m_backend_sockfd, &backend_mask_r);
            FD_SET(m_backend_sockfd, &backend_mask_w);
            int ret_val = select(m_backend_sockfd + 1, &backend_mask_r, &backend_mask_w, NULL, &timeout);
            FD_CLR(m_backend_sockfd, &backend_mask_r);
            FD_CLR(m_backend_sockfd, &backend_mask_w);
            
            if(ret_val > 0)
            {
                connected = true;
                break;  /* Success */
            }
            else
            {
                backend_close();
                continue;
            }
#endif /* _WITH_ASYNC_ */
        }
        else
        {
            backend_close();
            continue;
        }
    }

    freeaddrinfo(server_addr);           /* No longer needed */
    
    if(!connected)
    {            
        backend_close();         
        return false;
    }
    
    m_address = backend_addr;
    m_port = backend_port;
    m_http_tunneling_url = http_url;
    
    m_tunneling_state = TunnlingConnected;
    //connected!
    return true;
}

bool http_tunneling::send_request_to_backend(const char* hbuf, int hlen, const char* dbuf, int dlen)
{
    if(m_type == HTTP_Tunneling_Without_CONNECT || m_type == HTTP_Tunneling_Without_CONNECT_SSL)
    {
        if(m_tunneling_state == TunnlingSendingReqToBackend)
        {
            if(m_tunneling_cache_instance)
            {
                m_tunneling_state = TunnlingSentReqToBackend;
                return true;
            }
            
            //skip since there's cache
            //if(hlen > 0)
            //    printf("[%.*s]", hlen, hbuf);
            
            if(hbuf && hlen > 0 && backend_send(hbuf, hlen) < 0)
            {
                m_tunneling_state = TunnlingError;
                return false;
            }
            
            //if(dlen > 0)
            //    printf("[%.*s]", dlen, dbuf);
        
            if(dbuf && dlen > 0 && backend_send(dbuf, dlen) < 0)
            {
                
                m_tunneling_state = TunnlingError;
                return false;
            }
            
            m_tunneling_state = TunnlingSentReqToBackend;
        }
    }
    return true;
}

bool http_tunneling::acquire_cache_relay_to_client(CHttpResponseHdr* session_response_header)
{
    if(m_type == HTTP_Tunneling_Without_CONNECT || m_type == HTTP_Tunneling_Without_CONNECT_SSL)
    {
        if(m_tunneling_cache_instance)
        {
            TUNNELING_CACHE_DATA* tunneling_cache_data = m_tunneling_cache_instance->tunneling_rdlock();
            if(tunneling_cache_data)
            {
                if(m_tunneling_state == TunnlingRelayingData1)
                {
                    CHttpResponseHdr cache_header;
                    cache_header.SetStatusCode(SC200);
                    if(tunneling_cache_data->type != "")
                        cache_header.SetField("Content-Type", tunneling_cache_data->type.c_str());
                    if(tunneling_cache_data->cache != "")
                        cache_header.SetField("Cache-Control", tunneling_cache_data->cache.c_str());                
                    if(tunneling_cache_data->allow != "")
                        cache_header.SetField("Allow", tunneling_cache_data->allow.c_str());
                    if(tunneling_cache_data->encoding != "")
                        cache_header.SetField("Content-Encoding", tunneling_cache_data->encoding.c_str());
                    if(tunneling_cache_data->language != "")
                        cache_header.SetField("Content-Language", tunneling_cache_data->language.c_str());
                    if(tunneling_cache_data->etag != "")
                        cache_header.SetField("ETag", tunneling_cache_data->etag.c_str());
                    if(tunneling_cache_data->len >= 0)
                        cache_header.SetField("Content-Length", tunneling_cache_data->len);
                    if(tunneling_cache_data->expires != "")
                        cache_header.SetField("Expires", tunneling_cache_data->expires.c_str());
                    if(tunneling_cache_data->last_modified != "")
                        cache_header.SetField("Last-Modified", tunneling_cache_data->last_modified.c_str());
                    
                    cache_header.SetField("Connection", "Keep-Alive");
                    
                    string strVia;
                    
                    if(tunneling_cache_data->via == "")
                    {
                        strVia = "HTTP/1.1 ";
                        strVia += CHttpBase::m_localhostname.c_str();
                        strVia += "(" VERSION_STRING ")";                    
                    }
                    else
                    {
                        strVia = tunneling_cache_data->via;
                        strVia += ", HTTP/1.1 ";
                        strVia += CHttpBase::m_localhostname.c_str();
                        strVia += "(" VERSION_STRING ")";
                    }
                    
                    cache_header.SetField("Via", strVia.c_str());
                    
                    string str_header_crlf = cache_header.Text();
                    str_header_crlf += "\r\n";
                    
                    if(client_send(str_header_crlf.c_str(), str_header_crlf.length()) < 0)
                    {
                        m_tunneling_cache_instance->tunneling_unlock();
                        m_tunneling_state = TunnlingError;
                        return false;
                    }                
                    if(tunneling_cache_data->buf && tunneling_cache_data->len > 0)
                    {
                        if(client_send( tunneling_cache_data->buf, tunneling_cache_data->len) < 0)
                        {
                            m_tunneling_cache_instance->tunneling_unlock();
                            m_tunneling_state = TunnlingError;
                            return false;
                        }
                    }
                    
                    m_tunneling_state = TunnlingRelayingData2;
                }
                
                if(m_tunneling_state == TunnlingRelayingData2)
                {
                    if(client_flush() == 0)
                    {
                        m_tunneling_state = TunnlingRelayingData3;
                    }
                }
            }
            m_tunneling_cache_instance->tunneling_unlock();
            
            if(m_tunneling_state == TunnlingRelayingData3)
            {
#ifdef _WITH_ASYNC_
                if(client_flush() == 0)
                {
                    m_tunneling_state = TunnlingComplete;
                }
#else
                //flush the buffer
                fd_set mask_w; 
                fd_set mask_e; 
                struct timeval timeout; 
                while(m_client_send_buf_used_len > 0)
                {
                    FD_ZERO(&mask_w);
                    FD_ZERO(&mask_e);
                    
                    timeout.tv_sec = CHttpBase::m_connection_idle_timeout; 
                    timeout.tv_usec = 0;
                    
                    if(m_client_send_buf_used_len > 0)
                    {
                        FD_SET(m_client_sockfd, &mask_w);
                    }
                    
                    FD_SET(m_client_sockfd, &mask_e);
                    
                    int ret_val = select(m_client_send_buf_used_len + 1, NULL, &mask_w, &mask_e, &timeout);
                    
                    if( ret_val <= 0)
                    {
                        break; // quit from the loop since error or timeout
                    }
                    
                    if(FD_ISSET(m_client_sockfd, &mask_w))
                    {
                        client_flush();
                    }
                    
                    if(FD_ISSET(m_client_sockfd, &mask_e))
                    {
                        m_client_send_buf_used_len = 0;
                        close(m_client_sockfd);
                        m_client_sockfd = -1;
                    }
                }
                
                m_tunneling_state = TunnlingComplete;
#endif /* _WITH_ASYNC_ */
            }
            return true;
        }
    }
    
    return true;
}

bool http_tunneling::recv_response_from_backend_relay_to_client(CHttpResponseHdr* session_response_header)
{
    if(m_type == HTTP_Tunneling_Without_CONNECT || m_type == HTTP_Tunneling_Without_CONNECT_SSL)
    {
#ifdef _WITH_ASYNC_
        if(m_tunneling_state == TunnlingRelayingData1)
        {
            if(!m_client)
            {
                m_client = new http_client(m_cache, m_http_tunneling_url.c_str(), this);
            }
            
            std::unique_ptr<char> response_buf_obj(new char[4096]);
            char* response_buf = response_buf_obj.get();
            
            int len = 0;
            do
            {
                len = backend_recv(response_buf, 4095);
                if(len > 0)
                {
                    response_buf[len] = '\0';
                    
                    if(!m_client->processing(response_buf, len))
                    { 
                        m_tunneling_state = TunnlingRelayingDataComplete;
                        break;
                    }
                }
                else if(len < 0)
                {
                    m_tunneling_state = TunnlingError;
                }
                client_flush();
            } while(len > 0);
        }
        
        if(m_tunneling_state == TunnlingRelayingDataComplete)
        {
            m_tunneling_state = TunnlingComplete;
        }
#else
        if(m_tunneling_state == TunnlingRelayingData1)
        {
            if(!m_client)
                m_client = new http_client(m_cache, m_http_tunneling_url.c_str(), this);
            
            std::unique_ptr<char> response_buf_obj(new char[4096]);
            char* response_buf = response_buf_obj.get();
            
            fd_set mask_r; 
            fd_set mask_w; 
            fd_set mask_e; 
            struct timeval timeout; 
            
            BOOL tunneling_ongoing = TRUE;
            
            int tunneling_max_sockfd = m_backend_sockfd > m_client_sockfd ? m_backend_sockfd : m_client_sockfd;
            
            while(m_backend_send_buf_used_len > 0 || m_client_send_buf_used_len > 0 || tunneling_ongoing)
            {
                FD_ZERO(&mask_r);
                FD_ZERO(&mask_w);
                FD_ZERO(&mask_e);
                
                timeout.tv_sec = CHttpBase::m_connection_idle_timeout; 
                timeout.tv_usec = 0;

                FD_SET(m_backend_sockfd, &mask_r);
                
                int write_fd_size = 0;
                if(m_backend_send_buf_used_len > 0)
                {
                    write_fd_size++;
                    FD_SET(m_backend_sockfd, &mask_w);
                }
                if(m_client_send_buf_used_len > 0)
                {
                    write_fd_size++;
                    FD_SET(m_client_sockfd, &mask_w);
                }
                
                FD_SET(m_client_sockfd, &mask_e);
                FD_SET(m_backend_sockfd, &mask_e);
                
                int ret_val = select(tunneling_max_sockfd + 1, &mask_r, write_fd_size > 0 ? &mask_w : NULL, &mask_e, &timeout);
                
                if( ret_val <= 0)
                {
                    break; // quit from the loop since error or timeout
                }
                
                if(FD_ISSET(m_backend_sockfd, &mask_r))
                {
                    int len = recv(m_backend_sockfd, response_buf, 4095, 0);
                    
                    if(len > 0)
                    {
                        response_buf[len] = '\0';
                        
                        if(!m_client->processing(response_buf, len))
                        {
                            delete m_client;
                            m_client = NULL;
                            
                            tunneling_ongoing = FALSE;
                        }
                    }
                    else if(len < 0)
                    {
                        if( errno == EAGAIN)
                            continue;
                        
                        tunneling_ongoing = FALSE;
                    }
                }
                
                if(FD_ISSET(m_backend_sockfd, &mask_w))
                {
                    backend_flush();
                }
                
                if(FD_ISSET(m_client_sockfd, &mask_w))
                {
                    client_flush();
                }
                
                if(FD_ISSET(m_backend_sockfd, &mask_e))
                {
                    m_backend_send_buf_used_len = 0;
                    backend_close();
                    tunneling_ongoing = FALSE;
                }
                
                if(FD_ISSET(m_client_sockfd, &mask_e))
                {
                    m_client_send_buf_used_len = 0;
                    close(m_client_sockfd);
                    m_client_sockfd = -1;
                    tunneling_ongoing = FALSE;
                }
            }
            m_tunneling_state = TunnlingComplete;
        }
#endif /* _WITH_ASYNC_ */
    }
    
    return true;
}

void http_tunneling::relay_processing()
{
#ifdef _WITH_ASYNC_
    char client_recv_buf[4096];
    
    int client_recv_len = m_http->AsyncHttpRecv(client_recv_buf, 4095);
    
    if(client_recv_len > 0)
    {
        backend_send(client_recv_buf, client_recv_len);
    }
    else if(client_recv_len < 0)
    {
       m_tunneling_state = TunnlingError;
    }
    
    char backend_recv_buf[4096];
    int backend_recv_len = backend_recv(backend_recv_buf, 4095);
    
    if(backend_recv_len > 0)
    {
        client_send(backend_recv_buf, backend_recv_len);
    }
    else if(backend_recv_len < 0)
    {            
        m_tunneling_state = TunnlingError;
    }
#else
    Buffer_Descr buf_descr_frm_client, buf_descr_frm_backend;
    
    buf_descr_frm_client.buf = (char*)malloc(BUFFER_DESCR_BUF_LEN*2 + 1); //dup for implement ring buffer
    buf_descr_frm_client.buf_len = BUFFER_DESCR_BUF_LEN;
    buf_descr_frm_client.r_pos = 0;
    buf_descr_frm_client.w_pos = 0;
    
    buf_descr_frm_backend.buf = (char*)malloc(BUFFER_DESCR_BUF_LEN*2 + 1); //dup for implement ring buffer
    buf_descr_frm_backend.buf_len = BUFFER_DESCR_BUF_LEN;
    buf_descr_frm_backend.r_pos = 0;
    buf_descr_frm_backend.w_pos = 0;
    
    char recv_buf[4096];
    fd_set mask_r, mask_w, mask_e; 
    struct timeval timeout; 
    FD_ZERO(&mask_r);
    FD_ZERO(&mask_w);
    FD_ZERO(&mask_e);
    while(m_type == HTTP_Tunneling_With_CONNECT)
    {
        timeout.tv_sec = CHttpBase::m_connection_idle_timeout; 
        timeout.tv_usec = 0;

        FD_SET(m_backend_sockfd, &mask_r);
        FD_SET(m_client_sockfd, &mask_r);
		
        FD_SET(m_backend_sockfd, &mask_e);
        FD_SET(m_client_sockfd, &mask_e);
        
        int ret_val = select(m_backend_sockfd > m_client_sockfd ? m_backend_sockfd + 1 : m_backend_sockfd + 1,
            &mask_r, &mask_w, &mask_e, &timeout);
        if(ret_val > 0)
        {
            if(FD_ISSET(m_client_sockfd, &mask_r))
            {
                //recv from the client
                int wanna_recv_len = buf_descr_frm_client.buf_len - (buf_descr_frm_client.w_pos - buf_descr_frm_client.r_pos);
                
                if(wanna_recv_len > 0 && wanna_recv_len <= buf_descr_frm_client.buf_len)
                {
                    int len = recv(m_client_sockfd, buf_descr_frm_client.buf + buf_descr_frm_client.w_pos%buf_descr_frm_client.buf_len, wanna_recv_len, 0);
                    
                    if(len > buf_descr_frm_client.buf_len - buf_descr_frm_client.w_pos%buf_descr_frm_client.buf_len) //dup the data
                    {
                        memcpy(buf_descr_frm_client.buf, buf_descr_frm_client.buf + buf_descr_frm_client.buf_len,
                            len - (buf_descr_frm_client.buf_len - buf_descr_frm_client.w_pos%buf_descr_frm_client.buf_len));
                    }
                    
                    if(len == 0)
                    {
                        close(m_client_sockfd);
                        backend_close();
                        break;
                    }
                    else if(len < 0)
                    {
                        if( errno != EAGAIN)
                        {
                            close(m_client_sockfd);
                            backend_close();
                            break;
                        }
                    }
                    else if(len > 0)
                    {
                        buf_descr_frm_client.w_pos += len;
                        FD_SET(m_backend_sockfd, &mask_w);
                    }
                }
            }
            
            //send
            if(FD_ISSET(m_backend_sockfd, &mask_w))
            {
                int wanna_send_len = buf_descr_frm_client.w_pos - buf_descr_frm_client.r_pos;
                
                if(wanna_send_len > 0 && wanna_send_len <= buf_descr_frm_client.buf_len)
                {
                    int len = send(m_backend_sockfd, buf_descr_frm_client.buf + buf_descr_frm_client.r_pos%buf_descr_frm_client.buf_len, wanna_send_len, 0);
                    
                    if(len == 0)
                    {
                        close(m_client_sockfd);
                        backend_close();
                        break;
                    }
                    else if(len < 0)
                    {
                        if( errno != EAGAIN)
                        {
                            close(m_client_sockfd);
                            backend_close();
                            break;
                        }
                    }
                    else if(len > 0)
                    {
                        buf_descr_frm_client.r_pos += len;
                        if(buf_descr_frm_client.r_pos == buf_descr_frm_client.w_pos)
                            FD_CLR(m_backend_sockfd, &mask_w);
                    }
                }
                else if(wanna_send_len == 0)
                {
                    FD_CLR(m_backend_sockfd, &mask_w);
                }
            }
            
            if(FD_ISSET(m_backend_sockfd,&mask_r))
            {
                //recv from the backend
                int wanna_recv_len = buf_descr_frm_backend.buf_len - (buf_descr_frm_backend.w_pos - buf_descr_frm_backend.r_pos);
                
                if(wanna_recv_len > 0 && wanna_recv_len <= buf_descr_frm_backend.buf_len)
                {
                    int len = recv(m_backend_sockfd, buf_descr_frm_backend.buf + buf_descr_frm_backend.w_pos%buf_descr_frm_backend.buf_len, wanna_recv_len, 0);
                    
                    if(len > buf_descr_frm_backend.buf_len - buf_descr_frm_backend.w_pos%buf_descr_frm_backend.buf_len) //dup the data
                    {
                        memcpy(buf_descr_frm_backend.buf, buf_descr_frm_backend.buf + buf_descr_frm_backend.buf_len,
                            len - (buf_descr_frm_backend.buf_len - buf_descr_frm_backend.w_pos%buf_descr_frm_backend.buf_len));
                    }
                    
                    if(len == 0)
                    {
                        close(m_client_sockfd);
                        backend_close();
                        break;
                    }
                    else if(len < 0)
                    {
                        if( errno != EAGAIN)
                        {
                            close(m_client_sockfd);
                            backend_close();
                            break;
                        }
                    }
                    else if(len > 0)
                    {
                        buf_descr_frm_backend.w_pos += len;
                        FD_SET(m_client_sockfd, &mask_w);
                    }
                }
            }
            
            //send
            if(FD_ISSET(m_client_sockfd, &mask_w))
            {
                int wanna_send_len = buf_descr_frm_backend.w_pos - buf_descr_frm_backend.r_pos;
                
                if(wanna_send_len > 0 && wanna_send_len <= buf_descr_frm_backend.buf_len)
                {
                    int len = send(m_client_sockfd, buf_descr_frm_backend.buf + buf_descr_frm_backend.r_pos%buf_descr_frm_backend.buf_len, wanna_send_len, 0);
                    
                    if(len == 0)
                    {
                        close(m_client_sockfd);
                        backend_close();
                        break;
                    }
                    else if(len < 0)
                    {
                        if( errno != EAGAIN)
                        {
                            close(m_client_sockfd);
                            backend_close();
                            break;
                        }
                    }
                    else if(len > 0)
                    {
                        buf_descr_frm_backend.r_pos += len;
                        if(buf_descr_frm_backend.r_pos == buf_descr_frm_backend.w_pos)
                            FD_CLR(m_client_sockfd, &mask_w);
                    }
                }
                else if(wanna_send_len == 0)
                {
                    FD_CLR(m_client_sockfd, &mask_w);
                }
            }
            
            if(FD_ISSET(m_client_sockfd, &mask_e))
            {
                close(m_client_sockfd);
                backend_close();
                break;
            }
            
            if(FD_ISSET(m_backend_sockfd, &mask_e))
            {
                close(m_client_sockfd);
                backend_close();
                break;
            }
        }
        else
        {
			close(m_client_sockfd);
			backend_close();
            break;
        }
    }
    
    free(buf_descr_frm_client.buf);
    free(buf_descr_frm_backend.buf);
#endif /* _WITH_ASYNC_ */
}

int http_tunneling::backend_recv(char* buf, int len)
{
    int min_len = 0;
	if(m_async_backend_recv_buf && m_async_backend_recv_data_len > 0)
    {
        min_len = len < m_async_backend_recv_data_len ? len : m_async_backend_recv_data_len;
        memcpy(buf, m_async_backend_recv_buf, min_len);
        if(min_len > 0)
        {
            memmove(m_async_backend_recv_buf, m_async_backend_recv_buf + min_len, m_async_backend_recv_data_len - min_len);
            m_async_backend_recv_data_len -= min_len;
        }
    }
    
    return min_len;
}

int http_tunneling::backend_async_recv()
{
    char buf[1024];

    int len = m_backend_ssl ? SSL_read(m_backend_ssl, buf, 1024) : recv(m_backend_sockfd, buf, 1024, 0);
    if(len > 0)
    {
        
        m_async_backend_recv_buf_size = m_async_backend_recv_data_len + len;
        char* new_buf = (char*)malloc(m_async_backend_recv_buf_size);
        
        if(m_async_backend_recv_buf)
        {
            memcpy(new_buf, m_async_backend_recv_buf, m_async_backend_recv_data_len);
            free(m_async_backend_recv_buf);
        }
        memcpy(new_buf + m_async_backend_recv_data_len, buf, len);
        m_async_backend_recv_data_len += len;
        m_async_backend_recv_buf = new_buf;
    }
    else if(len == 0)
    {
        return -1;
    }
    else
    {
        if(m_backend_ssl)
        {
            int ret = SSL_get_error(m_backend_ssl, len);
            if(ret == SSL_ERROR_WANT_READ || ret == SSL_ERROR_WANT_WRITE)
                len = 0;
        }
        else
        {
            if( errno == EAGAIN)
                len = 0;
        }
    }
    
    return len;
}

int http_tunneling::backend_async_flush()
{
        return backend_flush();
}