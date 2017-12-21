/*
	Copyright (c) openheap, uplusware
	uplusware@gmail.com
*/
#include "tunneling.h"
#include "http_client.h"
#include "httpcomm.h"
#include "version.h"

http_tunneling::http_tunneling(int client_socked, SSL* client_ssl, HTTPTunneling type, memory_cache* cache)
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
}

http_tunneling::~http_tunneling()
{
    if(m_backend_sockfd > 0)
	{
        close(m_backend_sockfd);
		
	}
    m_backend_sockfd = -1;
}

int http_tunneling::client_send(const char* buf, int len)
{
	if(m_client_ssl)
		return SSLWrite(m_client_sockfd, m_client_ssl, buf, len, CHttpBase::m_connection_idle_timeout);
	else
		return _Send_( m_client_sockfd, buf, len, CHttpBase::m_connection_idle_timeout);
		
}


bool http_tunneling::connect_backend(const char* szAddr, unsigned short nPort, const char* http_url,
    const char* szAddrBackup1, unsigned short nPortBackup1, const char* http_url_backup1,
    const char* szAddrBackup2, unsigned short nPortBackup2, const char* http_url_backup2,
    BOOL request_no_cache)
{
    //try 1st one cache
	m_http_tunneling_url = http_url;    
	
    if(CHttpBase::m_enable_http_tunneling_cache && !request_no_cache && m_http_tunneling_url != "")
    {
        m_cache->rdlock_tunneling_cache();
        m_cache->_find_tunneling_(m_http_tunneling_url.c_str(), &m_tunneling_cache_instance);
        m_cache->unlock_tunneling_cache();
        
        if(m_tunneling_cache_instance)
        {
            return true;
        }
        else
        {
            // try 2nd one cache
            m_http_tunneling_url = http_url_backup1;
            m_cache->rdlock_tunneling_cache();
            m_cache->_find_tunneling_(m_http_tunneling_url.c_str(), &m_tunneling_cache_instance);
            m_cache->unlock_tunneling_cache();
            
            if(m_tunneling_cache_instance)
            {
                return true;
            }
            else
            {
                // try 3rd one cache
                m_http_tunneling_url = http_url_backup2;
                m_cache->rdlock_tunneling_cache();
                m_cache->_find_tunneling_(m_http_tunneling_url.c_str(), &m_tunneling_cache_instance);
                m_cache->unlock_tunneling_cache();
                
                if(m_tunneling_cache_instance)
                {
                    return true;
                }
            }
        }
    }
    
	if((m_address == szAddr && m_port == nPort && m_backend_sockfd > 0)
        || (m_address == szAddrBackup1 && nPortBackup1 == nPort && m_backend_sockfd > 0)
        || (m_address == szAddrBackup2 && nPortBackup2 == nPort && m_backend_sockfd > 0))
		return true;
    
    for(int t = 0; t < 3; t++)
    {
        //connect to the new address:port
        if(t == 0)
        {
            if(*szAddr == '\0' || nPort == 0)
                continue;
            m_address = szAddr;
            m_port = nPort;
            m_http_tunneling_url = http_url;
        }
        else if(t == 1)
        {
            if(*szAddrBackup1 == '\0' || nPortBackup1 == 0)
                continue;
            m_address = szAddrBackup1;
            m_port = nPortBackup1;
            m_http_tunneling_url = http_url_backup1;
        }
        else if(t == 2)
        {
            if(*szAddrBackup2 == '\0'|| nPortBackup2 == 0)
                continue;
            m_address = szAddrBackup2;
            m_port = nPortBackup2;
            m_http_tunneling_url = http_url_backup2;
        }
        
        time_t t1 = time(NULL);
        unsigned short backhost_port = m_port;
        
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
            int rval = getaddrinfo(m_address.c_str(), NULL, &hints, &servinfo);
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
            continue;
        
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
            fprintf(stderr, "couldn't find ip for %s\n", m_address.c_str());
            continue; //to to next backup
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
        sprintf(szPort, "%u", backhost_port);
        
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
            continue;
        
        bool connected = false;
        
        for (rp = server_addr; rp != NULL; rp = rp->ai_next)
        {
            m_backend_sockfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
            if (m_backend_sockfd == -1)
                continue;
           
            int flags = fcntl(m_backend_sockfd, F_GETFL, 0); 
            fcntl(m_backend_sockfd, F_SETFL, flags | O_NONBLOCK);

            fd_set mask_r, mask_w; 
            struct timeval timeout; 
        
            timeout.tv_sec = CHttpBase::m_connection_sync_timeout; 
            timeout.tv_usec = 0;
            
            int s = connect(m_backend_sockfd, rp->ai_addr, rp->ai_addrlen);
            if(s == 0 || (s < 0 && errno == EINPROGRESS))
            {

                FD_ZERO(&mask_r);
                FD_ZERO(&mask_w);
            
                FD_SET(m_backend_sockfd, &mask_r);
                FD_SET(m_backend_sockfd, &mask_w);
                int ret_val = select(m_backend_sockfd + 1, &mask_r, &mask_w, NULL, &timeout);
                if(ret_val > 0)
                {
                    connected = true;
                    break;  /* Success */
                }
                else
                {
                    if(m_backend_sockfd > 0)
                        close(m_backend_sockfd);
                    m_backend_sockfd = -1;
                    continue;
                }  
            }
            else
            {
                if(m_backend_sockfd > 0)
                    close(m_backend_sockfd);
                m_backend_sockfd = -1;
                continue;
            }
        }

        freeaddrinfo(server_addr);           /* No longer needed */
        
        if(!connected)
        {            
            if(m_backend_sockfd > 0)
                close(m_backend_sockfd);
            m_backend_sockfd = -1;
                    
            continue;
        }
        
        //connected!
        return true;
    }
    
    // have try 3 one, and couldn't connect to any backend server.
    return false;
}

bool http_tunneling::send_request(const char* hbuf, int hlen, const char* dbuf, int dlen)
{
    
    if(m_type == HTTP_Tunneling_Without_CONNECT)
    {
        if(m_tunneling_cache_instance)
        {
            return true;
        }
        //skip since there's cache
    
        if(hbuf && hlen > 0 && _Send_(m_backend_sockfd, hbuf, hlen, CHttpBase::m_connection_idle_timeout) < 0)
            return false;
        if(dbuf && dlen > 0 && _Send_(m_backend_sockfd, dbuf, dlen, CHttpBase::m_connection_idle_timeout) < 0)
            return false;
    }
    return true;
}

bool http_tunneling::recv_relay_reply(CHttpResponseHdr* session_response_header)
{
    if(m_type == HTTP_Tunneling_Without_CONNECT)
    {
        if(m_tunneling_cache_instance)
        {
            TUNNELING_CACHE_DATA* tunneling_cache_data = m_tunneling_cache_instance->tunneling_rdlock();
            if(tunneling_cache_data)
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
                    strVia += "("VERSION_STRING")";                    
                }
                else
                {
                    strVia = tunneling_cache_data->via;
                    strVia += ", HTTP/1.1 ";
                    strVia += CHttpBase::m_localhostname.c_str();
                    strVia += "("VERSION_STRING")";
                }
                
                cache_header.SetField("Via", strVia.c_str());
                
                string str_header_crlf = cache_header.Text();
                str_header_crlf += "\r\n";
                
                if(client_send(str_header_crlf.c_str(), str_header_crlf.length()) < 0)
                {
                    m_tunneling_cache_instance->tunneling_unlock();
                    return false;
                }                
                if(tunneling_cache_data->buf && tunneling_cache_data->len > 0)
                {
                    if(client_send( tunneling_cache_data->buf, tunneling_cache_data->len) < 0)
                    {
                        m_tunneling_cache_instance->tunneling_unlock();
                        return false;
                    }
                }
            }
            m_tunneling_cache_instance->tunneling_unlock();
            return true;
        }
        //skip since there's cache
        
        int http_header_length = -1;
        int http_content_length = -1;
                    
        http_client the_client(m_client_sockfd, m_client_ssl, m_backend_sockfd, m_cache, m_http_tunneling_url.c_str());
        string str_header;
        int received_len = 0;
        
        auto_ptr<char> response_buf_obj(new char[4096]);
        char* response_buf = response_buf_obj.get();
        
        fd_set mask_r; 
        struct timeval timeout; 
        FD_ZERO(&mask_r);
        while(1)
        {
            timeout.tv_sec = CHttpBase::m_connection_idle_timeout; 
            timeout.tv_usec = 0;

            FD_SET(m_backend_sockfd, &mask_r);
            int ret_val = select(m_backend_sockfd + 1, &mask_r, NULL, NULL, &timeout);
            if( ret_val <= 0)
            {
                break; // quit from the loop since error or timeout
            }
            
            int len = recv(m_backend_sockfd, response_buf, 4095, 0);
            
            if(len == 0)
            {
				close(m_client_sockfd);
                close(m_backend_sockfd);
                m_backend_sockfd = -1;
                
                return false; 
            }
            else if(len < 0)
            {
                if( errno == EAGAIN)
                    continue;
				close(m_client_sockfd);
                close(m_backend_sockfd);
                m_backend_sockfd = -1;
                
                return false;
            }
            response_buf[len] = '\0';
            
            
            if(!the_client.processing(response_buf, len))
            {
                break;
            }
        }
                    
    }
    
    return true;
}

void http_tunneling::relay_processing()
{
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
                        close(m_backend_sockfd);
                        m_backend_sockfd = -1;
                        break;
                    }
                    else if(len < 0)
                    {
                        if( errno != EAGAIN)
                        {
                            close(m_client_sockfd);
                            close(m_backend_sockfd);
                            m_backend_sockfd = -1;
                            break;
                        }
                    }
                    else if(len > 0)
                    {
                        //printf("RECV(CLT): %d\n", len);
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
                        close(m_backend_sockfd);
                        m_backend_sockfd = -1;
                        break;
                    }
                    else if(len < 0)
                    {
                        if( errno != EAGAIN)
                        {
                            close(m_client_sockfd);
                            close(m_backend_sockfd);
                            m_backend_sockfd = -1;
                            break;
                        }
                    }
                    else if(len > 0)
                    {
                        //printf("SEND(SRV): %d\n", len);
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
                        close(m_backend_sockfd);
                        m_backend_sockfd = -1;
                        break;
                    }
                    else if(len < 0)
                    {
                        if( errno != EAGAIN)
                        {
                            close(m_client_sockfd);
                            close(m_backend_sockfd);
                            m_backend_sockfd = -1;
                            break;
                        }
                    }
                    else if(len > 0)
                    {
                        //printf("RECV(SRV): %d\n", len);
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
                        close(m_backend_sockfd);
                        m_backend_sockfd = -1;
                        break;
                    }
                    else if(len < 0)
                    {
                        if( errno != EAGAIN)
                        {
                            close(m_client_sockfd);
                            close(m_backend_sockfd);
                            m_backend_sockfd = -1;
                            break;
                        }
                    }
                    else if(len > 0)
                    {
                        //printf("SEND(CLT): %d\n", len);
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
                close(m_backend_sockfd);
                m_backend_sockfd = -1;
                break;
            }
            
            if(FD_ISSET(m_backend_sockfd, &mask_e))
            {
                close(m_client_sockfd);
                close(m_backend_sockfd);
                m_backend_sockfd = -1;
                break;
            }
        }
        else
        {
			close(m_client_sockfd);
			close(m_backend_sockfd);
            m_backend_sockfd = -1;
            break;
        }
    }
    
    free(buf_descr_frm_client.buf);
    free(buf_descr_frm_backend.buf);
}
