/*
	Copyright (c) openheap, uplusware
	uplusware@gmail.com
*/

#include "tunneling.h"
#include "http_client.h"

http_tunneling::http_tunneling(int client_socked, const char* szAddr, unsigned short nPort, HTTPTunneling type)
{
    m_address = szAddr;
    m_port = nPort;
    m_client_sockfd = client_socked;
    m_backend_sockfd = -1;
    m_type = type;
}

http_tunneling::~http_tunneling()
{
    if(m_backend_sockfd > 0)
        close(m_backend_sockfd);
    m_backend_sockfd = -1;
}

bool http_tunneling::connect_backend()
{
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
    
    if (getaddrinfo(m_address.c_str(), NULL, &hints, &servinfo) != 0)
    {
        return false;
    }
    
    bool found = false;
    curr = servinfo; 
    while (curr && curr->ai_canonname)
    {  
        if(servinfo->ai_family == AF_INET6)
        {
            sa6 = (struct sockaddr_in6 *)curr->ai_addr;  
            inet_ntop(AF_INET6, (void*)&sa6->sin6_addr, backhost_ip, sizeof (backhost_ip));
            found = TRUE;
        }
        else if(servinfo->ai_family == AF_INET)
        {
            sa = (struct sockaddr_in *)curr->ai_addr;  
            inet_ntop(AF_INET, (void*)&sa->sin_addr, backhost_ip, sizeof (backhost_ip));
            found = TRUE;
        }
        curr = curr->ai_next;
    }     

    freeaddrinfo(servinfo);

    if(found == false)
    {
        fprintf(stderr, "couldn't find ip for %s\n", m_address.c_str());
        return false;
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
    if (getaddrinfo((backhost_ip && backhost_ip[0] != '\0') ? backhost_ip : NULL, szPort, &hints, &server_addr) != 0)
    {
       
        string strError = backhost_ip;
        strError += ":";
        strError += szPort;
        strError += " ";
        strError += strerror(errno);
        
        fprintf(stderr, "%s\n", strError.c_str());
        return false;
    }
    
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
    
        timeout.tv_sec = MAX_SOCKET_TIMEOUT; 
	    timeout.tv_usec = 0;
        
        int s = connect(m_backend_sockfd, rp->ai_addr, rp->ai_addrlen);
        if(s == 0 || (s == -1 && errno == EINPROGRESS))
        {
            FD_ZERO(&mask_r);
            FD_ZERO(&mask_w);
        
            FD_SET(m_backend_sockfd, &mask_r);
            FD_SET(m_backend_sockfd, &mask_w);
            
            if(select(m_backend_sockfd + 1, &mask_r, &mask_w, NULL, &timeout) == 1)
            {
                connected = true;
                break;  /* Success */
            }
            else
            {
                close(m_backend_sockfd);
                continue;
            }  
        }
    }

    freeaddrinfo(server_addr);           /* No longer needed */
    
    if(!connected)
    {
        string strError = backhost_ip;
        strError += ":";
        strError += szPort;
        strError += " ";
        strError += strerror(errno);
        
        fprintf(stderr, "%s\n", strError.c_str());
        return false;
    }
    return true;
}

bool http_tunneling::send_request(const char* hbuf, int hlen, const char* dbuf, int dlen)
{
    if(m_type == HTTP_Tunneling_Without_CONNECT)
    {
        if(hbuf && hlen > 0 && _Send_(m_backend_sockfd, hbuf, hlen) < 0)
            return false;
        if(dbuf && dlen > 0 && _Send_(m_backend_sockfd, dbuf, dlen) < 0)
            return false;
    }
    return true;
}

bool http_tunneling::recv_relay_reply()
{
    if(m_type == HTTP_Tunneling_Without_CONNECT)
    {
        int http_header_length = -1;
        int http_content_length = -1;
            
        http_client the_client(m_client_sockfd);
        string str_header;
        int received_len = 0;
        char response_buf[4096];
        int next_recv_len = 4095;
        fd_set mask_r; 
        struct timeval timeout; 
        FD_ZERO(&mask_r);
        while(1)
        {
            timeout.tv_sec = MAX_SOCKET_TIMEOUT; 
            timeout.tv_usec = 0;

            FD_SET(m_backend_sockfd, &mask_r);
            int ros = select(m_backend_sockfd + 1, &mask_r, NULL, NULL, &timeout);
            if( ros <= 0)
            {
                break; // quit from the loop since error or timeout
            }
            
            int len = recv(m_backend_sockfd, response_buf, 4095 /*next_recv_len > 4095 ? 4095 : next_recv_len*/, 0);
            if(len == 0)
            {
				close(m_client_sockfd);
                return false; 
            }
            else if(len < 0)
            {
                if( errno == EAGAIN)
                    continue;
				close(m_client_sockfd);
                return false;
            }
            response_buf[len] = '\0';
            
            if(!the_client.processing(response_buf, len, next_recv_len))
                break;
        }
    }
    return true;
}

void http_tunneling::relay_processing()
{
    char recv_buf[4096];
    fd_set mask_r; 
    struct timeval timeout; 
    FD_ZERO(&mask_r);
    while(m_type == HTTP_Tunneling_With_CONNECT)
    {
        timeout.tv_sec = MAX_SOCKET_TIMEOUT; 
        timeout.tv_usec = 0;

        FD_SET(m_backend_sockfd, &mask_r);
        FD_SET(m_client_sockfd, &mask_r);
		
        int ros = select(m_backend_sockfd > m_client_sockfd ? m_backend_sockfd + 1 : m_backend_sockfd + 1,
            &mask_r, NULL, NULL, &timeout);
        if(ros > 0)
        {
            if(FD_ISSET(m_client_sockfd, &mask_r))
            {
                //recv from the client
                int len = recv(m_client_sockfd, recv_buf, 4095, 0);//SSL_read(m_client_ssl, recv_buf, 4095);
                if(len == 0)
                {
					close(m_backend_sockfd);
                    break;
                }
                else if(len < 0)
                {
                    if( errno != EAGAIN)
                    {
						close(m_backend_sockfd);
                        break;
                    }
                }
                else if(len > 0)
                {
                    recv_buf[len] = '\0';
                    if(_Send_(m_backend_sockfd, recv_buf, len) < 0)
                        break;
                }
            }
            if(FD_ISSET(m_backend_sockfd,&mask_r))
            {
                //recv from the backend
                int len = recv(m_backend_sockfd, recv_buf, 4095, 0);//SSL_read(m_backend_ssl, recv_buf, 4095);
                if(len == 0)
                {
					close(m_client_sockfd);
                    break;
                }
                else if(len < 0)
                {
                    if( errno != EAGAIN)
                    {
						close(m_client_sockfd);
                        break;
                    }
                }
                else if(len > 0)
                {
                    recv_buf[len] = '\0';
                    if(_Send_(m_client_sockfd, recv_buf, len) < 0)
                        break;
                }
            }
        }
        else if(ros < 0)
        {
			close(m_client_sockfd);
			close(m_client_sockfd);
            break;
        }
    }
}
