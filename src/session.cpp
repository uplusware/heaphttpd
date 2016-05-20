/*
	Copyright (c) openheap, uplusware
	uplusware@gmail.com
*/

#include "session.h"

Session::Session(ServiceObjMap* srvobj, int sockfd, SSL* ssl, const char* clientip, Service_Type st, memory_cache* ch)
{
    m_srvobj = srvobj;
	m_sockfd = sockfd;
    m_ssl = ssl;
	m_clientip = clientip;
	m_st = st;

	m_cache = ch;
}

Session::~Session()
{

}

void Session::Process()
{

	AUTH_SCHEME wwwauth_scheme = asNone;
	if(strcasecmp(CHttpBase::m_www_authenticate.c_str(), "basic") == 0)
	{
		wwwauth_scheme = asBasic;
	}
	else if(strcasecmp(CHttpBase::m_www_authenticate.c_str(), "digest") == 0)
	{
		wwwauth_scheme = asDigest;
	}
	
    Http_Connection httpConn = httpKeepAlive;
    
    while(httpConn != httpClose)
    {
        CHttp * pProtocol;
        try
        {
            if(m_st == stHTTP)
            {
                pProtocol = new CHttp(m_srvobj, m_sockfd, CHttpBase::m_localhostname.c_str(), CHttpBase::m_httpport,
                    m_clientip.c_str(), m_cache,
					CHttpBase::m_work_path.c_str(), CHttpBase::m_php_mode.c_str(),
                    CHttpBase::m_fpm_socktype.c_str(), CHttpBase::m_fpm_sockfile.c_str(), 
                    CHttpBase::m_fpm_addr.c_str(), CHttpBase::m_fpm_port, CHttpBase::m_phpcgi_path.c_str(),
                    CHttpBase::m_fastcgi_name.c_str(), CHttpBase::m_fastcgi_pgm.c_str(), 
                    CHttpBase::m_fastcgi_socktype.c_str(), CHttpBase::m_fastcgi_sockfile.c_str(), 
                    CHttpBase::m_fastcgi_addr.c_str(), CHttpBase::m_fastcgi_port,
                    CHttpBase::m_private_path.c_str(), CHttpBase::m_global_uid, wwwauth_scheme);
                    CHttpBase::m_global_uid++;
            }
            else if(m_st == stHTTPS)
            {
                pProtocol = new CHttp(m_srvobj, m_sockfd, CHttpBase::m_localhostname.c_str(), CHttpBase::m_httpsport,
                    m_clientip.c_str(), m_cache,
					CHttpBase::m_work_path.c_str(), CHttpBase::m_php_mode.c_str(), 
                    CHttpBase::m_fpm_socktype.c_str(), CHttpBase::m_fpm_sockfile.c_str(), 
                    CHttpBase::m_fpm_addr.c_str(), CHttpBase::m_fpm_port, CHttpBase::m_phpcgi_path.c_str(),
                    CHttpBase::m_fastcgi_name.c_str(), CHttpBase::m_fastcgi_pgm.c_str(),  
                    CHttpBase::m_fastcgi_socktype.c_str(), CHttpBase::m_fastcgi_sockfile.c_str(), 
                    CHttpBase::m_fastcgi_addr.c_str(), CHttpBase::m_fastcgi_port,
                    CHttpBase::m_private_path.c_str(), CHttpBase::m_global_uid, wwwauth_scheme, m_ssl);
                    CHttpBase::m_global_uid++;
            }
            else
            {
                throw("wrong protocol type");
            }
        }
        catch(string* e)
        {
            //printf("catched exception: %s\n", e->c_str());
            delete e;
            close(m_sockfd);
            m_sockfd = -1;
            return;
        }
        char szmsg[4096];
        int result;
        while(1)
        {
            result = pProtocol->ProtRecv(szmsg, 4095);
            if(result <= 0)
            {
                httpConn = httpClose; // socket is broken. close the keep-alive connection
                break;
            }
            else
            {
                szmsg[result] = '\0';
                httpConn = pProtocol->LineParse(szmsg);
                if(httpConn != httpContinue) // Session finished or keep-alive connection closed.
                    break;
            }
        }
        delete pProtocol;
    }
}

