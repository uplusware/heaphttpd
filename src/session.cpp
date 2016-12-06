/*
	Copyright (c) openheap, uplusware
	uplusware@gmail.com
*/

#include "session.h"

Session::Session(ServiceObjMap* srvobj, int sockfd, SSL* ssl, const char* clientip, X509* client_cert,
    BOOL https, BOOL http2, memory_cache* ch)
{
    m_srvobj = srvobj;
	m_sockfd = sockfd;
    m_ssl = ssl;
	m_clientip = clientip;
	m_https = https;
	m_http2 = http2;
	m_client_cert = client_cert;
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
        IHttp * pProtocol;
        try
        {
            if(!m_https)
            {
                pProtocol = new CHttp(m_srvobj, m_sockfd, CHttpBase::m_localhostname.c_str(), CHttpBase::m_httpport,
                    m_clientip.c_str(), m_client_cert, m_cache,
					CHttpBase::m_work_path.c_str(), &CHttpBase::m_ext_list, CHttpBase::m_php_mode.c_str(),
                    CHttpBase::m_fpm_socktype.c_str(), CHttpBase::m_fpm_sockfile.c_str(), 
                    CHttpBase::m_fpm_addr.c_str(), CHttpBase::m_fpm_port, CHttpBase::m_phpcgi_path.c_str(),
                    CHttpBase::m_fastcgi_name.c_str(), CHttpBase::m_fastcgi_pgm.c_str(), 
                    CHttpBase::m_fastcgi_socktype.c_str(), CHttpBase::m_fastcgi_sockfile.c_str(), 
                    CHttpBase::m_fastcgi_addr.c_str(), CHttpBase::m_fastcgi_port,
                    CHttpBase::m_private_path.c_str(), wwwauth_scheme);
            }
            else if(m_https)
            {
                if(m_http2)
                {
                    pProtocol = new CHttp2(m_srvobj, m_sockfd, CHttpBase::m_localhostname.c_str(), CHttpBase::m_httpsport,
                        m_clientip.c_str(), m_client_cert, m_cache,
                        CHttpBase::m_work_path.c_str(), &CHttpBase::m_ext_list, CHttpBase::m_php_mode.c_str(), 
                        CHttpBase::m_fpm_socktype.c_str(), CHttpBase::m_fpm_sockfile.c_str(), 
                        CHttpBase::m_fpm_addr.c_str(), CHttpBase::m_fpm_port, CHttpBase::m_phpcgi_path.c_str(),
                        CHttpBase::m_fastcgi_name.c_str(), CHttpBase::m_fastcgi_pgm.c_str(),  
                        CHttpBase::m_fastcgi_socktype.c_str(), CHttpBase::m_fastcgi_sockfile.c_str(), 
                        CHttpBase::m_fastcgi_addr.c_str(), CHttpBase::m_fastcgi_port,
                        CHttpBase::m_private_path.c_str(), wwwauth_scheme, m_ssl);
                }
                else
                {
                    pProtocol = new CHttp(m_srvobj, m_sockfd, CHttpBase::m_localhostname.c_str(), CHttpBase::m_httpsport,
                        m_clientip.c_str(), m_client_cert, m_cache,
                        CHttpBase::m_work_path.c_str(), &CHttpBase::m_ext_list, CHttpBase::m_php_mode.c_str(), 
                        CHttpBase::m_fpm_socktype.c_str(), CHttpBase::m_fpm_sockfile.c_str(), 
                        CHttpBase::m_fpm_addr.c_str(), CHttpBase::m_fpm_port, CHttpBase::m_phpcgi_path.c_str(),
                        CHttpBase::m_fastcgi_name.c_str(), CHttpBase::m_fastcgi_pgm.c_str(),  
                        CHttpBase::m_fastcgi_socktype.c_str(), CHttpBase::m_fastcgi_sockfile.c_str(), 
                        CHttpBase::m_fastcgi_addr.c_str(), CHttpBase::m_fastcgi_port,
                        CHttpBase::m_private_path.c_str(), wwwauth_scheme, m_ssl);
                }
            }
            else
            {
                throw(new string("wrong protocol type"));
            }
        }
        catch(string* e)
        {
            //printf("Catch exception: %s\n", e->c_str());
            delete e;
            shutdown(m_sockfd, SHUT_RDWR);
            m_sockfd = -1;
            return;
        }
        
        if(m_http2)
        {
            httpConn = pProtocol->Processing();
        }
        else
        {
            httpConn = pProtocol->Processing();
        }
        delete pProtocol;
    }
}

