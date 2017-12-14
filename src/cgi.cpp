/*
	Copyright (c) openheap, uplusware
	uplusware@gmail.com
*/
#include "cgi.h"
#include "base.h"

cgi_base::cgi_base(const char* ipaddr, unsigned short port)
{
	m_sockfd = -1;
	m_strIP = ipaddr;
	m_nPort = port;
    m_sockType = INET_SOCK;
}

cgi_base::cgi_base(const char* sock_file)
{
	m_sockfd = -1;
	m_strSockfile = sock_file;
    m_sockType = UNIX_SOCK;
}

cgi_base::cgi_base(int sockfd)
{
	m_sockfd = sockfd;
    m_sockType = UNIX_SOCK;
}

cgi_base::~cgi_base()
{
	if(m_sockfd >= 0)
		close(m_sockfd);
}

int cgi_base::Connect()
{
	string err_msg;
	int err_code = 1;
	int err_code_len = sizeof(err_code);
	int res;
	struct sockaddr_in6 ser_addr;
    struct sockaddr_un ser_unix;

	fd_set mask_r, mask_w; 
	struct timeval timeout; 
	
	m_sockfd = socket(m_sockType == INET_SOCK ? AF_INET6 : AF_UNIX, SOCK_STREAM, 0);
	if(m_sockfd < 0)
	{
		err_msg = "system error\r\n";
		return -1;
	}
	
	int flags = fcntl(m_sockfd, F_GETFL, 0); 
	fcntl(m_sockfd, F_SETFL, flags | O_NONBLOCK); 
	
    if(m_sockType == INET_SOCK)
    {
	    struct addrinfo hints;
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
        sprintf(szPort, "%u", m_nPort);
        
        /*printf("fast-cgi: %s %s\n", m_strIP.c_str(), szPort);*/
        int s = getaddrinfo(m_strIP != "" ? m_strIP.c_str() : NULL, szPort, &hints, &server_addr);
        if (s != 0)
        {
           fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
           return FALSE;
        }
        
        for (rp = server_addr; rp != NULL; rp = rp->ai_next)
        {
            m_sockfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
            if (m_sockfd == -1)
               continue;

            int flags = fcntl(m_sockfd, F_GETFL, 0); 
            fcntl(m_sockfd, F_SETFL, flags | O_NONBLOCK);

            timeout.tv_sec = 10; 
            timeout.tv_usec = 0;

            connect(m_sockfd, rp->ai_addr, rp->ai_addrlen);
            break;
        }
         
        if (rp == NULL)
        {
            err_msg = "system error\r\n";
		    return -1;
        }

        freeaddrinfo(server_addr);           /* No longer needed */
    }
    else
    {
        char local_sockfile[256];
        sprintf(local_sockfile, "/tmp/heaphttpd/fastcgi.sock.%05d.%05d", getpid(), gettid());

        //printf("local sock file: %s\n", local_sockfile);        
        memset(&ser_unix, 0, sizeof(ser_unix));
	    ser_unix.sun_family = AF_UNIX;
	    strcpy(ser_unix.sun_path, local_sockfile);
        unlink(ser_unix.sun_path);

        int size = offsetof(struct sockaddr_un, sun_path) + strlen(ser_unix.sun_path);
	    if (bind(m_sockfd, (struct sockaddr *)&ser_unix, size) < 0)
        {
		   err_msg = "system error\r\n";
		   return -1;
	    }
        
        memset(&ser_unix, 0, sizeof(ser_unix));
    	ser_unix.sun_family = AF_UNIX;
    	strcpy(ser_unix.sun_path, m_strSockfile.c_str());
    	size = offsetof(struct sockaddr_un, sun_path) + strlen(ser_unix.sun_path);

        connect(m_sockfd, (struct sockaddr*)&ser_unix, size);
    }
    

	timeout.tv_sec = 10; 
	timeout.tv_usec = 0;      

	FD_ZERO(&mask_r);
	FD_ZERO(&mask_w);
	
	FD_SET(m_sockfd, &mask_r);
	FD_SET(m_sockfd, &mask_w);
	res = select(m_sockfd + 1, &mask_r, &mask_w, NULL, &timeout);
	
	if( res != 1) 
	{
		close(m_sockfd);
		m_sockfd = -1;
		err_msg = "can not connect the ";
		err_msg += m_sockType == INET_SOCK ? m_strIP : m_strSockfile;
		err_msg += ".\r\n";
		
		return -1;
	}
	getsockopt(m_sockfd,SOL_SOCKET,SO_ERROR,(char*)&err_code,(socklen_t*)&err_code_len);
	if(err_code !=0)
	{
		close(m_sockfd);
		m_sockfd = -1;
		err_msg = "system error\r\n";
		return -1;
	}
	
	return 0;
}

int cgi_base::Send(const char* buf, unsigned long long len)
{
	return m_sockfd >= 0 ? _Send_(m_sockfd, (char*)buf, len, CHttpBase::m_connection_idle_timeout) : -1;
}

int cgi_base::Recv(const char* buf, unsigned long long len)
{
	return m_sockfd >= 0 ? _Recv_(m_sockfd, (char*)buf, len, CHttpBase::m_connection_idle_timeout) : -1;
}

// classic cgi mode
//forkcgi

forkcgi::forkcgi(int sockfd)
    : cgi_base(sockfd)
{
    
}

forkcgi::~forkcgi()
{
    
}

int forkcgi::SendData(const char* data, int len)
{
	return Send(data, len);
}

int forkcgi::ReadAppData(vector<char>& binaryResponse, BOOL& continue_recv)
{
    continue_recv = FALSE;
    char szBuf[4096];
    while(1)
    {
        int rlen = Recv(szBuf, 4095);
        if(rlen > 0)
        {
            for(int x = 0; x < rlen; x++)
            {
                binaryResponse.push_back(szBuf[x]);
            }
            if(binaryResponse.size() >= 1024*64) /* 64k */
            {
                continue_recv = TRUE;
                break;
            }
        }
        else if(rlen <= 0)
            break;
    }
    
    return binaryResponse.size();
}