/*
	Copyright (c) openheap, uplusware
	uplusware@gmail.com
*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>

#include "util/base64.h"
#include "htdoc.h"
#include "fstring.h"
#include "http.h"
#include "http2.h"
#include "http2comm.h"
#include "util/general.h"
#include "heapapi.h"
#include "serviceobjmap.h"
#include "version.h"

#define MAX_APPLICATION_X_WWW_FORM_URLENCODED_LEN (1024*1024*4)
#define MAX_MULTIPART_FORM_DATA_LEN (1024*1024*4)

const char* HTTP_METHOD_NAME[] = { "OPTIONS", "GET", "HEAD", "POST", "PUT", "DELETE", "TRACE", "CONNECT" };

CHttp::CHttp(int epoll_fd, time_t connection_first_request_time, time_t connection_keep_alive_timeout, unsigned int connection_keep_alive_request_tickets, http_tunneling* tunneling, ServiceObjMap * srvobj, int sockfd, const char* servername, unsigned short serverport,
    const char* clientip, X509* client_cert, memory_cache* ch,
	const char* work_path, vector<string>* default_webpages, vector<http_extension_t>* ext_list, vector<http_extension_t>* reverse_ext_list, const char* php_mode, 
    cgi_socket_t fpm_socktype, const char* fpm_sockfile,
    const char* fpm_addr, unsigned short fpm_port, const char* phpcgi_path,
    map<string, cgi_cfg_t>* cgi_list,
	const char* private_path, AUTH_SCHEME wwwauth_scheme, AUTH_SCHEME proxyauth_scheme, 
	SSL* ssl, CHttp2* phttp2, uint_32 http2_stream_ind)
{	
    m_epoll_fd = epoll_fd;
    m_srvobj = srvobj;
    m_protocol_upgrade = FALSE;
    m_upgrade_protocol = "";
    m_web_socket_handshake = Websocket_None;

    m_connection_first_request_time = connection_first_request_time;
    m_connection_keep_alive_timeout = connection_keep_alive_timeout;
    m_connection_keep_alive_request_tickets = connection_keep_alive_request_tickets;
    
    if(m_connection_keep_alive_request_tickets == 1 || m_connection_keep_alive_timeout == 0 || (time(NULL) - m_connection_first_request_time) >=m_connection_keep_alive_timeout)
    {
        m_enabled_keep_alive = FALSE;
        m_keep_alive = FALSE;
    }
    else
    {
        m_keep_alive = TRUE; /* HTTP/1.1 Keep-Alive is enabled as default */
        m_enabled_keep_alive = TRUE; 
    }
    
    if(!m_enabled_keep_alive)
    {
        m_response_header.SetField("Connection", "keep-alive, close");
    }
    
	m_passed_wwwauth = FALSE;
    m_passed_proxyauth = FALSE;
	m_wwwauth_scheme = wwwauth_scheme;
    m_proxyauth_scheme = proxyauth_scheme;
	m_cache = ch;

    m_ext_list = ext_list;
    m_reverse_ext_list = reverse_ext_list;
    m_default_webpages = default_webpages;
    
    m_client_cert = client_cert;
    
	m_sockfd = sockfd;
	m_clientip = clientip;
	m_servername = servername;
	m_serverport = serverport;
	
	char szPort[64];
	sprintf(szPort, "%d", m_serverport);
	m_cgi.SetMeta("SERVER_NAME", m_servername.c_str());
	m_cgi.SetMeta("SERVER_PORT", szPort);
	m_cgi.SetMeta("REMOTE_ADDR", m_clientip.c_str());
	
    m_line_text = "";
	m_lsockfd = NULL;
	m_lssl = NULL;
	
	m_content_length = 0;
	
    int ssl_rc = -1;
	m_ssl = ssl;

	m_postdata_ex = NULL;
	m_formdata = NULL;
	m_querystring = "";
	m_postdata = "";
	m_http_method = hmGet;

    m_http_tunneling_connection = HTTP_Tunneling_None;
    m_http_tunneling = tunneling;
    
    m_work_path = work_path;
	m_php_mode = php_mode;
    m_fpm_socktype = fpm_socktype;
    m_fpm_sockfile = fpm_sockfile;
	m_fpm_addr = fpm_addr;
	m_fpm_port = fpm_port;
    m_phpcgi_path = phpcgi_path;
    
    m_cgi_list = cgi_list;

    m_private_path = private_path;
    
    m_lsockfd = NULL;
    m_lssl = NULL;
    
	if(m_ssl)
	{
		int flags = fcntl(m_sockfd, F_GETFL, 0); 
		fcntl(m_sockfd, F_SETFL, flags | O_NONBLOCK);
		
		m_lssl = new linessl(m_sockfd, m_ssl);
	}
	else
	{
		int flags = fcntl(m_sockfd, F_GETFL, 0); 
		fcntl(m_sockfd, F_SETFL, flags | O_NONBLOCK); 

		m_lsockfd = new linesock(m_sockfd);
	}

	m_content_type = application_x_www_form_urlencoded;
    
    m_http2 = phttp2;
    m_http2_stream_ind = http2_stream_ind;
    
    m_request_no_cache = FALSE;
    
    m_http_state = httpReqHeader;
    
    m_tunneling_ext_handled = FALSE;
    m_backend_connected = FALSE;
    m_tunneling_connection_established = FALSE;
}

CHttp::~CHttp()
{
	if(m_keep_alive != TRUE || m_enabled_keep_alive != TRUE) //close the socket when non-keep-alive or non-websocket
	{
		if(m_sockfd > 0)
		{
			shutdown(m_sockfd, SHUT_RDWR);
			m_sockfd = -1;
		}
	}
	
    if(m_lssl)
        delete m_lssl;
    
    if(m_lsockfd)
        delete m_lsockfd;
    
	if(m_postdata_ex)
		delete m_postdata_ex;

	if(m_formdata)
		delete m_formdata;
}

int CHttp::HttpSend(const char* buf, int len)
{
#ifdef _WITH_ASYNC_
    return AsyncHttpSend(buf, len);
#else
	if(m_ssl)
		return SSLWrite(m_sockfd, m_ssl, buf, len, CHttpBase::m_connection_idle_timeout);
	else
		return _Send_( m_sockfd, buf, len, CHttpBase::m_connection_idle_timeout);
#endif /* _WITH_ASYNC_ */
		
}

int CHttp::HttpRecv(char* buf, int len)
{
#ifdef _WITH_ASYNC_
    return AsyncHttpRecv(buf, len);
#else
	if(m_ssl)
	{
		return m_lssl->drecv(buf, len);
	}
	else
		return m_lsockfd->drecv(buf, len);	
#endif /* _WITH_ASYNC_ */
}

int CHttp::AsyncHttpSend(const char* buf, int len)
{
	if(len > 0)
    {
        m_async_send_buf_size = m_async_send_data_len + len;
        char* new_buf = (char*)malloc(m_async_send_buf_size);
        
        if(m_async_send_buf)
        {
            memcpy(new_buf, m_async_send_buf, m_async_send_data_len);
            free(m_async_send_buf);
        }
        memcpy(new_buf + m_async_send_data_len, buf, len);
        m_async_send_data_len += len;
        m_async_send_buf = new_buf;
        
        //printf("AsyncHttpSend %d %d\n", m_async_send_data_len, len);
        
        AsyncSend();
            
    }
    
    return len;
		
}

int CHttp::AsyncHttpRecv(char* buf, int len)
{
    int min_len = 0;
	if(m_async_recv_buf && m_async_recv_data_len > 0)
    {
        min_len = len < m_async_recv_data_len ? len : m_async_recv_data_len;
        memcpy(buf, m_async_recv_buf, min_len);
        if(min_len > 0)
        {
            memmove(m_async_recv_buf, m_async_recv_buf + min_len, m_async_recv_data_len - min_len);
            m_async_recv_data_len -= min_len;
        }
    }
    
    return min_len;
}

int CHttp::AsyncSend()
{
    int len = 0;
    if(m_async_send_buf && m_async_send_data_len > 0)
    {
        len = m_ssl ? SSL_write(m_ssl, m_async_send_buf, m_async_send_data_len) : send(m_sockfd, m_async_send_buf, m_async_send_data_len, 0);
        
        //printf("AsyncSend len %d %d\n", len, m_async_send_data_len);
        
        if(len > 0)
        {
            memmove(m_async_send_buf, m_async_send_buf + len, m_async_send_data_len - len);
            m_async_send_data_len -= len;
            
            struct epoll_event event; 
            event.data.fd = m_sockfd;  
            event.events = m_async_send_data_len == 0 ? (EPOLLIN | EPOLLHUP | EPOLLERR) : EPOLLIN | EPOLLOUT| EPOLLHUP | EPOLLERR;
            epoll_ctl (m_epoll_fd, EPOLL_CTL_MOD, m_sockfd, &event);
        }
        else
        {
            if(m_ssl)
            {
                int ret = SSL_get_error(m_ssl, len);
                if(ret == SSL_ERROR_WANT_READ || ret == SSL_ERROR_WANT_WRITE)
                    len = 0;
            }
            else
            {
                if( errno == EAGAIN)
                    len = 0;
            }
        }
    }
    
    return len;
		
}

int CHttp::AsyncRecv()
{
    char buf[1024];

    int len = m_ssl ? SSL_read(m_ssl, buf, 1024) : recv(m_sockfd, buf, 1024, 0);
    
    if(len > 0)
    {
        m_async_recv_buf_size = m_async_recv_data_len + len;
        char* new_buf = (char*)malloc(m_async_recv_buf_size);
        
        if(m_async_recv_buf)
        {
            memcpy(new_buf, m_async_recv_buf, m_async_recv_data_len);
            free(m_async_recv_buf);
        }
        memcpy(new_buf + m_async_recv_data_len, buf, len);
        m_async_recv_data_len += len;
        m_async_recv_buf = new_buf;
    }
    else
    {
        if(m_ssl)
        {
            int ret = SSL_get_error(m_ssl, len);
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

int CHttp::ProtRecv(char* buf, int len, int alive_timeout)
{
    if(m_ssl)
        return m_lssl->lrecv(buf, len, alive_timeout);
    else
        return m_lsockfd->lrecv(buf, len, alive_timeout);
}


int CHttp::AsyncProtRecv(char* buf, int len)
{
    char* p = NULL;
    unsigned int recv_num = 0;

    int left;
    int right;
    p = m_async_recv_data_len > 0 ? (char*)memchr(m_async_recv_buf, '\n', m_async_recv_data_len) : NULL;
    if(p != NULL)
    {
        left = p - m_async_recv_buf + 1;
        right = m_async_recv_data_len - left;
    
        if(len >= left)
        {
            memcpy(buf, m_async_recv_buf, left);
            memmove(m_async_recv_buf, p + 1, right);
            m_async_recv_data_len = right;
            buf[left] = '\0';
            return left;
        }
        else
        {
            memcpy(buf, m_async_recv_buf, len);
            memmove(m_async_recv_buf, m_async_recv_buf + len, m_async_recv_data_len - len);
            m_async_recv_data_len = m_async_recv_data_len - len;
            return len;
        }
    }
    else
    {
        if(len >= m_async_recv_data_len)
        {
            memcpy(buf, m_async_recv_buf, m_async_recv_data_len);
            recv_num = m_async_recv_data_len;
            m_async_recv_data_len = 0;
        }
        else
        {
            memcpy(buf, m_async_recv_buf, len);
            memmove(m_async_recv_buf, m_async_recv_buf + len, m_async_recv_data_len - len);
            m_async_recv_data_len = m_async_recv_data_len - len;
            return len;
        }
    }
    return recv_num;
}

Http_Connection CHttp::Processing()
{
    Http_Connection httpConn = httpKeepAlive;
    int result;
    char sz_http_data[4096];
    while(1)
    {
        result = ProtRecv(sz_http_data, 4095, CHttpBase::m_connection_keep_alive_timeout);
        if(result <= 0)
        {
            httpConn = httpClose; // socket is broken. close the keep-alive connection
            break;
        }
        else
        {
            sz_http_data[result] = '\0';
            httpConn = LineParse((const char*)sz_http_data);
            if(httpConn != httpContinue) // Session finished or keep-alive connection closed.
                break;
        }
    }
    
    return httpConn;
}

Http_Connection CHttp::AsyncProcessing()
{
    Http_Connection httpConn = httpKeepAlive;
    if(m_http_state == httpReqHeader)
    {
        char sz_http_data[4096];
        int result = AsyncProtRecv(sz_http_data, 4095);
        if(result <= 0)
        {
            httpConn = httpClose; // socket is broken. close the keep-alive connection
        }
        else
        {
            sz_http_data[result] = '\0';
            httpConn = LineParse((const char*)sz_http_data);
        }
    }
    else if(m_http_state == httpReqData)
    {
        httpConn = DataParse();
    }
    else if(m_http_state == httpResponse)
    {
        httpConn = ResponseReply();
    }
    else if(m_http_state == httpComplete)
    {
        httpConn = httpClose;
    }
    
    return httpConn;
}

void CHttp::SetCookie(const char* szName, const char* szValue,
    int nMaxAge, const char* szExpires,
    const char* szPath, const char* szDomain, 
    BOOL bSecure, BOOL bHttpOnly)
{
    string strEncodedValue;
    NIU_URLFORMAT_ENCODE((const unsigned char*)szValue, strEncodedValue);
    string strCookie;
    Cookie ck(szName, strEncodedValue.c_str(), nMaxAge, szExpires, szPath, szDomain, bSecure, bHttpOnly);   
    ck.toString(strCookie);
    
    map<string, string>::iterator iter = m_set_cookies.find(szName);
    if(iter != m_set_cookies.end())
        m_set_cookies.erase(iter);
    m_set_cookies.insert(map<string, string>::value_type(szName, strCookie.c_str()));
}

void CHttp::SetSessionVar(const char* szName, const char* szValue)
{
    const char* psuid;
    char szuid_seed[1024];
    char szuid[33];
    if(m_session_var_uid == "")
    {
        srandom(time(NULL));
        sprintf(szuid_seed, "%08x-%08x-%p-%08x", time(NULL), getpid(), this, random());
        
        unsigned char HA[16];
        MD5_CTX_OBJ Md5Ctx;
        Md5Ctx.MD5Init();
        Md5Ctx.MD5Update((unsigned char*) szuid_seed, strlen(szuid_seed));
        Md5Ctx.MD5Final(HA);
        sprintf(szuid, "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x", 
            HA[0], HA[1], HA[2], HA[3], HA[4], HA[5], HA[6], HA[7], 
            HA[8], HA[9], HA[10], HA[11], HA[12], HA[13], HA[14], HA[15]);
        psuid = szuid;
        
        m_session_var_uid = szuid;
    }
    else
    {
        psuid = m_session_var_uid.c_str();
    }
    string strEncodedValue;
    NIU_URLFORMAT_ENCODE((const unsigned char*)szValue, strEncodedValue);
    m_cache->push_session_var(psuid, szName, strEncodedValue.c_str());
}

int CHttp::GetSessionVar(const char* szName, string& strValue)
{
    if(m_session_var_uid != "")
    {
        string strEncodedValue;
        int ret = m_cache->get_session_var(m_session_var_uid.c_str(), szName, strEncodedValue);
        NIU_URLFORMAT_DECODE((const unsigned char*)strEncodedValue.c_str(), strValue);
        return ret;
    }
    else
        return -1;
}

void CHttp::SetServerVar(const char* szName, const char* szValue)
{
    string strEncodedValue;
    NIU_URLFORMAT_ENCODE((const unsigned char*)szValue, strEncodedValue);
    m_cache->push_server_var(szName, strEncodedValue.c_str());
}

const char* CHttp::GetPostDataVar(const char* key)
{
    if(_POST_VARS_.find(key) != _POST_VARS_.end())
        return _POST_VARS_[key].c_str();
    else
        return NULL;
}

const char* CHttp::GetQueryStringVar(const char* key)
{
    if(_GET_VARS_.find(key) != _GET_VARS_.end())
        return _GET_VARS_[key].c_str();
    else
        return NULL;
}

const char* CHttp::GetCookieVar(const char* key)
{
    if(_COOKIE_VARS_.find(key) != _COOKIE_VARS_.end())
        return _COOKIE_VARS_[key].c_str();
    else
        return NULL;
}

int CHttp::GetServerVar(const char* szName, string& strValue)
{
    string strEncodedValue;
    int ret = m_cache->get_server_var(szName, strEncodedValue);
    NIU_URLFORMAT_DECODE((const unsigned char*)strEncodedValue.c_str(), strValue);
    return ret;
}

int CHttp::SendHeader(const char* buf, int len)
{
    string strHeader = buf;
    
	map<string, string>::iterator cookie_it;
	for(cookie_it = m_set_cookies.begin(); cookie_it != m_set_cookies.end(); ++cookie_it)
	{
	    if(cookie_it->second != "")
		{
			strHeader += "Set-Cookie: ";
			strHeader += cookie_it->second;
			strHeader += "\r\n";
		}
	}
	
	if(m_session_var_uid != "")
	{
	    strHeader += "Set-Cookie: __heaphttpd_session__=";
		strHeader += m_session_var_uid;
		strHeader += "\r\n";
	}
	
    strHeader += "\r\n";
    if(m_http2)
    {
        m_http2->TransHttp1SendHttp2Header(m_http2_stream_ind, strHeader.c_str(), strHeader.length());
    }
    else
    {
        if(strstr(strHeader.c_str(), "\r\nContent-Length:") == NULL)
        {
            m_keep_alive = FALSE;
        }
        return HttpSend(strHeader.c_str(), strHeader.length());
    }
}

int CHttp::SendContent(const char* buf, int len)
{   
    if(m_http2)
    {
        return m_http2->TransHttp1SendHttp2Content(m_http2_stream_ind, buf, len);
    }
    else
        return HttpSend(buf, len);
}
void CHttp::ParseMethod(string & strtext)
{
    int buf_len = strtext.length();
    
    char* sz_method = (char*)malloc(buf_len + 1);
    memset(sz_method, 0, buf_len + 1);
    
    sscanf(strtext.c_str(), "%[A-Z \t]", sz_method);
    
    if(strncasecmp(sz_method, "CONNECT ", 8) == 0)
    {
        m_http_tunneling_connection = HTTP_Tunneling_With_CONNECT;
        
        const char* p_temp = strtext.c_str() + strlen(sz_method);
        
        char* sz_host = (char*)malloc(buf_len + 1);
        
        memset(sz_host, 0, buf_len + 1);
        sscanf(p_temp, "%[^ ]", sz_host);        
        
        m_http_tunneling_backend_port = 443;
        
        char* p = strstr(sz_host, ":");
        if(p)
        {
            *p = '\0';
            m_http_tunneling_backend_address = sz_host;
            m_http_tunneling_backend_port = atoi(p + 1);
            strtrim(m_http_tunneling_backend_address);
        }
        else
        {
            m_http_tunneling_backend_address = sz_host;
            strtrim(m_http_tunneling_backend_address);
        }
        
        free(sz_host);
            
    }
    else
    {
        if(CHttpBase::m_enable_http_reverse_proxy)
        {
            //reverse extension hook
            for(int x = 0; x < m_reverse_ext_list->size(); x++)
            {
                void* (*reverse_delivery)(const char *, const char *, const char *, const char*, const char*, string *, int *);
                reverse_delivery = (void*(*)(const char *, const char *, const char *, const char*, const char*, string *, int *))dlsym((*m_reverse_ext_list)[x].handle, "reverse_delivery");
                const char* errmsg;
                if((errmsg = dlerror()) == NULL)
                {
                    int skip;
                    string new_method_url;
                    reverse_delivery((*m_reverse_ext_list)[x].name.c_str(), (*m_reverse_ext_list)[x].description.c_str(), (*m_reverse_ext_list)[x].parameters.c_str(),
                        m_clientip.c_str(), strtext.c_str(), &new_method_url, &skip);
                    
                    strtext = new_method_url.c_str();
                    if(skip != 0)
                    {
                        break;
                    }
                }
            }
        }
            
        const char* p_temp = strtext.c_str() + strlen(sz_method);
        if(strncasecmp(p_temp, "http://", 7) == 0 || strncasecmp(p_temp, "https://", 8) == 0)
        {
            bool isCached = FALSE;
            if(p_temp[4] == 's')
                m_http_tunneling_connection = HTTP_Tunneling_Without_CONNECT_SSL;
            else
                m_http_tunneling_connection = HTTP_Tunneling_Without_CONNECT;
            
            if(CHttpBase::m_enable_http_reverse_proxy)
            {
                char* sz_url = (char*)malloc(buf_len + 1);
                char* sz_host = (char*)malloc(buf_len + 1);
                char* sz_relatived = (char*)malloc(buf_len + 1);
                memset(sz_host, 0, buf_len + 1);
                memset(sz_relatived, 0, buf_len + 1);
                memset(sz_url, 0, buf_len + 1);
                
                sscanf(p_temp, "%[^ ]", sz_url);

                sscanf(p_temp + 7, "%[^/|]", sz_host);
                
                int first_host_strlen = strlen(sz_host);
                
                strcpy(sz_relatived, p_temp + 7 + first_host_strlen);
                
                m_http_tunneling_backend_port = 80;
                
                char* p = strstr(sz_host, ":");
                if(p)
                {
                    *p = '\0';
                    m_http_tunneling_backend_address = sz_host;
                    m_http_tunneling_backend_port = atoi(p + 1);
                    strtrim(m_http_tunneling_backend_address);
                }
                else
                {
                    m_http_tunneling_backend_address = sz_host;
                    strtrim(m_http_tunneling_backend_address);
                }
                 
                if(*(p_temp + 7 + first_host_strlen) == '|')
                {
                    memset(sz_host, 0, buf_len + 1);
                    memset(sz_relatived, 0, buf_len + 1);
                    memset(sz_url, 0, buf_len + 1);

                    sscanf(p_temp + 7 + first_host_strlen + 1, "%[^/|]", sz_host);
                
                    int second_host_strlen = strlen(sz_host);
                    
                    strcpy(sz_relatived, p_temp + 7 + first_host_strlen + 1 + second_host_strlen);
                    
                    m_http_tunneling_backend_port_backup1 = 80;
                    
                    char* p = strstr(sz_host, ":");
                    if(p)
                    {
                        *p = '\0';
                        m_http_tunneling_backend_address_backup1 = sz_host;
                        m_http_tunneling_backend_port_backup1 = atoi(p + 1);
                        strtrim(m_http_tunneling_backend_address_backup1);
                    }
                    else
                    {
                        m_http_tunneling_backend_address_backup1 = sz_host;
                        strtrim(m_http_tunneling_backend_address_backup1);
                    }
                    
                    if(*(p_temp + 7 + first_host_strlen + 1 + second_host_strlen) == '|')
                    {
                        memset(sz_host, 0, buf_len + 1);
                        memset(sz_relatived, 0, buf_len + 1);
                        memset(sz_url, 0, buf_len + 1);

                        sscanf(p_temp + 7 + first_host_strlen + 1 +  second_host_strlen + 1, "%[^/|]", sz_host);
                    
                        int third_host_strlen = strlen(sz_host);
                        
                        strcpy(sz_relatived, p_temp + 7 + first_host_strlen + 1 + second_host_strlen + 1 + third_host_strlen);
                        
                        m_http_tunneling_backend_port_backup2 = 80;
                        
                        char* p = strstr(sz_host, ":");
                        if(p)
                        {
                            *p = '\0';
                            m_http_tunneling_backend_address_backup2 = sz_host;
                            m_http_tunneling_backend_port_backup2 = atoi(p + 1);
                            strtrim(m_http_tunneling_backend_address_backup2);
                        }
                        else
                        {
                            m_http_tunneling_backend_address_backup2 = sz_host;
                            strtrim(m_http_tunneling_backend_address_backup2);
                        }
                        
                    }
                    
                }
                
                
                strtext = sz_method;
                strtext += sz_relatived;
                
                char tmp_szport[32];
                
                sprintf(tmp_szport, "%d", m_http_tunneling_backend_port);
                if(p_temp[4] == 's')
                    m_http_tunneling_url = "https://";
                else
                    m_http_tunneling_url = "http://";
                m_http_tunneling_url += m_http_tunneling_backend_address;
                m_http_tunneling_url += ":";
                m_http_tunneling_url += tmp_szport;
                m_http_tunneling_url += sz_relatived;
                
                
                if(m_http_tunneling_backend_address_backup1 != "")
                {
                    sprintf(tmp_szport, "%d", m_http_tunneling_backend_port_backup1);
                    if(p_temp[4] == 's')
                        m_http_tunneling_url_backup1 = "https://";
                    else
                        m_http_tunneling_url_backup1 = "http://";
                    m_http_tunneling_url_backup1 += m_http_tunneling_backend_address_backup1;
                    m_http_tunneling_url_backup1 += ":";
                    m_http_tunneling_url_backup1 += tmp_szport;
                    m_http_tunneling_url_backup1 += sz_relatived;
                }
                if(m_http_tunneling_backend_address_backup2 != "")
                {
                    sprintf(tmp_szport, "%d", m_http_tunneling_backend_port_backup2);
                    if(p_temp[4] == 's')
                        m_http_tunneling_url_backup2 = "https://";
                    else
                        m_http_tunneling_url_backup2 = "http://";
                    m_http_tunneling_url_backup2 += m_http_tunneling_backend_address_backup2;
                    m_http_tunneling_url_backup2 += ":";
                    m_http_tunneling_url_backup2 += tmp_szport;
                    m_http_tunneling_url_backup2 += sz_relatived;
                }
                
                free(sz_relatived);
                free(sz_host);
                free(sz_url);
            }
            else
            {
                char* sz_url = (char*)malloc(buf_len + 1);
                char* sz_host = (char*)malloc(buf_len + 1);
                char* sz_relatived = (char*)malloc(buf_len + 1);
                memset(sz_host, 0, buf_len + 1);
                memset(sz_relatived, 0, buf_len + 1);
                memset(sz_url, 0, buf_len + 1);
                
                sscanf(p_temp, "%[^ ]", sz_url);
                
                
                m_http_tunneling_url = sz_url;
                
                strtrim(m_http_tunneling_url);

                sscanf(p_temp + 7, "%[^/]", sz_host);
                
                strcpy(sz_relatived, p_temp + 7 + strlen(sz_host));
                
                m_http_tunneling_backend_port = 80;
                
                char* p = strstr(sz_host, ":");
                if(p)
                {
                    *p = '\0';
                    m_http_tunneling_backend_address = sz_host;
                    m_http_tunneling_backend_port = atoi(p + 1);
                    strtrim(m_http_tunneling_backend_address);
                }
                else
                {
                    m_http_tunneling_backend_address = sz_host;
                    strtrim(m_http_tunneling_backend_address);
                }
                strtext = sz_method;
                strtext += sz_relatived;
                
                free(sz_relatived);
                free(sz_host);
                free(sz_url);
            }
        }
        else
        {
            char* sz_resource = (char*)malloc(buf_len + 1);
            char* sz_querystring = (char*)malloc(buf_len + 1);
            memset(sz_resource, 0, buf_len + 1);
            memset(sz_querystring, 0, buf_len + 1);
            
            sscanf(strtext.c_str(), "%*[^/]%[^? \t\r\n]?%[^ \t\r\n]", sz_resource, sz_querystring);
                
            NIU_URLFORMAT_DECODE((const unsigned char *)sz_resource, m_resource);
    
            m_querystring = sz_querystring;
            free(sz_resource);
            free(sz_querystring);
            
            m_uri = m_resource;
            if(m_querystring != "")
            {
                m_uri += "?";
                m_uri += m_querystring;
            }
            
            m_cgi.SetMeta("REQUEST_URI", m_uri.c_str());
            m_cgi.SetMeta("QUERY_STRING", m_querystring.c_str());
        }
    }
    free(sz_method);
}

void CHttp::PushPostData(const char* buf, int len)
{
    if(m_content_type == application_x_www_form_urlencoded)
    {
        if(m_postdata.length() > MAX_APPLICATION_X_WWW_FORM_URLENCODED_LEN)
            return;
            
        char* rbuf = (char*)malloc(len + 1);
        memcpy(rbuf, buf, len);
        rbuf[len] = '\0';
        m_postdata += rbuf;
        free(rbuf);
    }
    else if(m_content_type == multipart_form_data)
    {
        if(!m_postdata_ex)
            m_postdata_ex = new fbuffer(m_private_path.c_str());
        m_postdata_ex->bufcat(buf, len);
    }
}

void CHttp::RecvPostData()
{
    if(m_content_length > 0)
    {
        if(m_content_type == application_x_www_form_urlencoded)
        {
            if(m_content_length < MAX_APPLICATION_X_WWW_FORM_URLENCODED_LEN)
            {
                char* post_data = (char*)malloc(m_content_length + 1);
                int nlen = HttpRecv(post_data, m_content_length);
                if( nlen > 0)
                {
                    post_data[nlen] = '\0';
                    m_postdata += post_data;
                }
                free(post_data);
            }
        }
        else if(m_content_type == multipart_form_data)
        {
            if(!m_postdata_ex)
                m_postdata_ex = new fbuffer(m_private_path.c_str());
        
            int nRecv = 0;
            while(1)
            {
                if(nRecv == m_content_length)
                    break;
                char rbuf[1449];
                int rlen = HttpRecv(rbuf, (m_content_length - nRecv) > 1448 ? 1448 : ( m_content_length - nRecv));
                if(rlen > 0)
                {
                    nRecv += rlen;
                    m_postdata_ex->bufcat(rbuf, rlen);
                }
                else
                {
                    break;
                }
            }
        }
    }
    m_http_state = httpResponse;
}

void CHttp::AsyncRecvPostData()
{
    if(m_content_length > 0)
    {
        if(m_content_type == application_x_www_form_urlencoded)
        {
            if(m_content_length < MAX_APPLICATION_X_WWW_FORM_URLENCODED_LEN)
            {
                char* post_data = (char*)malloc(m_content_length + 1);
                int nlen = AsyncHttpRecv(post_data, m_content_length - m_postdata.length());
                if( nlen > 0)
                {
                    post_data[nlen] = '\0';
                    m_postdata += post_data;
                }
                free(post_data);
                
                if(m_postdata.length() == m_content_length)
                {
                    m_http_state = httpResponse;
                }
            }
        }
        else if(m_content_type == multipart_form_data)
        {
            if(!m_postdata_ex)
                m_postdata_ex = new fbuffer(m_private_path.c_str());
            
            char rbuf[1449];
                int rlen = AsyncHttpRecv(rbuf, (m_content_length - m_postdata_ex->length()) > 1448 ? 1448 : ( m_content_length - m_postdata_ex->length()));
            if(rlen > 0)
            {
                m_postdata_ex->bufcat(rbuf, rlen);
            }
            else
            {
                 m_http_state = httpResponse;
            }
            
            if(m_postdata_ex->length() == m_content_length)
            {
                m_http_state = httpResponse;
            }
        }
    }
    else if(m_content_length == 0)
    {
        m_http_state = httpResponse;
    }
}

void CHttp::Tunneling()
{
    if(!m_tunneling_ext_handled)
    {
        //4rd extension hook
        bool session_is_continuing = true;
        
        for(int x = 0; x < m_ext_list->size(); x++)
        {
            void* (*ext_tunneling)(CHttp*, const char*, const char*, const char*, const char*, unsigned short, http_ext_tunneling_continuing*);
            ext_tunneling = (void*(*)(CHttp*, const char*, const char*, const char*, const char*, unsigned short, http_ext_tunneling_continuing*))dlsym((*m_ext_list)[x].handle, "ext_tunneling");
            const char* errmsg;
            if((errmsg = dlerror()) == NULL)
            {
                http_ext_tunneling_continuing ext_is_continuing = http_ext_tunneling_continuing_yes;
                ext_tunneling(this, (*m_ext_list)[x].name.c_str(), (*m_ext_list)[x].description.c_str(), (*m_ext_list)[x].parameters.c_str(),
                    m_http_tunneling_backend_address.c_str(), m_http_tunneling_backend_port, &ext_is_continuing);
                
                if(ext_is_continuing == http_ext_tunneling_continuing_no)
                {
                    session_is_continuing = false;
                }
            }
        }
        
        m_tunneling_ext_handled = TRUE;
        
        if(!session_is_continuing)
        {
            CHttpResponseHdr header(m_response_header.GetMap());
            header.SetStatusCode(SC405);

            header.SetField("Content-Type", "text/html");
            header.SetField("Content-Length", header.GetDefaultHTMLLength());
            SendHeader(header.Text(), header.Length());
            SendContent(header.GetDefaultHTML(), header.GetDefaultHTMLLength());
            
            return;
        }
    }
    
    if(!m_http_tunneling)
        m_http_tunneling = new http_tunneling(m_sockfd, m_ssl, m_http_tunneling_connection, m_cache);
    
    if(m_backend_connected || m_http_tunneling->connect_backend(m_http_tunneling_backend_address.c_str(), m_http_tunneling_backend_port, m_http_tunneling_url.c_str(),
        m_http_tunneling_backend_address_backup1.c_str(), m_http_tunneling_backend_port_backup1, m_http_tunneling_url_backup1.c_str(),
        m_http_tunneling_backend_address_backup2.c_str(), m_http_tunneling_backend_port_backup2, m_http_tunneling_url_backup2.c_str(),
        m_request_no_cache)) //connected
    {
        m_backend_connected = TRUE;
        
        if(m_http_tunneling_connection == HTTP_Tunneling_With_CONNECT)
        {
            if(!m_tunneling_connection_established)
            {
                const char* connect_resp = "HTTP/1.1 200 Connection Established\r\nProxy-Agent: "VERSION_STRING"\r\n\r\n";
                HttpSend(connect_resp, strlen(connect_resp));
                
                m_tunneling_connection_established = TRUE;
            }
            m_http_tunneling->relay_processing();
        }
        else if(m_http_tunneling_connection == HTTP_Tunneling_Without_CONNECT || m_http_tunneling_connection == HTTP_Tunneling_Without_CONNECT_SSL)
        {
            const char* pRequestHeader = m_header_content.c_str();
            int nRequestHeaderLen = m_header_content.length();
            
            const char* pRequestData = NULL;
            int nRequestDataLen = 0;
            if(m_content_type == multipart_form_data && m_postdata_ex)
            {
                pRequestData = m_postdata_ex->c_buffer();
                nRequestDataLen = m_postdata_ex->length();
            }
            else
            {
                pRequestData = m_postdata.c_str();
                nRequestDataLen = m_postdata.length();
            }
            
            if(m_http_tunneling->send_request_to_backend(pRequestHeader, nRequestHeaderLen, pRequestData, nRequestDataLen))
            {
                m_http_tunneling->recv_response_from_backend_relay_to_client(&m_response_header);
            }
        }
    }
}

void CHttp::Response()
{
    NIU_POST_GET_VARS(m_querystring.c_str(), _GET_VARS_);
    NIU_POST_GET_VARS(m_postdata.c_str(), _POST_VARS_);
    NIU_COOKIE_VARS(m_cookie.c_str(), _COOKIE_VARS_);
    
    if(_COOKIE_VARS_.size() > 0)
    {
        /* Wouldn't save cookie in server side */
        /* m_cache->reload_cookies(); */
        map<string, string>::iterator iter_c;
        for(iter_c = _COOKIE_VARS_.begin(); iter_c != _COOKIE_VARS_.end(); iter_c++)
        {
            if(iter_c->first == "__heaphttpd_session__")
            {
                m_session_var_uid = iter_c->second;
                break;
            }
            /* Wouldn't save cookie in server side */
            /* m_cache->access_cookie(iter_c->first.c_str()); */
        }
    }
    
    if(m_content_type == multipart_form_data)
    {
        m_cgi.SetData(m_postdata_ex->c_buffer(), m_postdata_ex->length());
        char szLen[64];
        sprintf(szLen, "%d", m_postdata.length());
        m_cgi.SetMeta("CONTENT_LENGTH", szLen);
        
        m_formdata = new formdata(m_postdata_ex->c_buffer(), m_postdata_ex->length(), m_boundary.c_str());
    }
    else
    {
        char szLen[64];
        sprintf(szLen, "%d", m_postdata.length());
        m_cgi.SetMeta("CONTENT_LENGTH", szLen);
        m_cgi.SetData(m_postdata.c_str(), m_postdata.length());
    }

    //1st extension hook
    BOOL skipSession = FALSE;
    for(int x = 0; x < m_ext_list->size(); x++)
    {
        void* (*ext_request)(CHttp*, const char*, const char*, const char*, BOOL*);
        ext_request = (void*(*)(CHttp*, const char*, const char*, const char*, BOOL*))dlsym((*m_ext_list)[x].handle, "ext_request");
        const char* errmsg;
        if((errmsg = dlerror()) == NULL)
        {
            BOOL skipAction = FALSE;
            ext_request(this, (*m_ext_list)[x].name.c_str(), (*m_ext_list)[x].description.c_str(), (*m_ext_list)[x].parameters.c_str(), &skipAction);
            skipSession = skipAction ? skipAction : skipSession;
        }
    }

    if(!skipSession)
    {
        Htdoc *doc = new Htdoc(this, m_work_path.c_str(), m_default_webpages, m_php_mode.c_str(), 
            m_fpm_socktype, m_fpm_sockfile.c_str(), 
            m_fpm_addr.c_str(), m_fpm_port, m_phpcgi_path.c_str(),
            m_cgi_list);

        //2nd extension hook
        for(int x = 0; x < m_ext_list->size(); x++)
        {
            void* (*ext_response)(CHttp*, const char*, const char*, const char*, Htdoc*);
            ext_response = (void*(*)(CHttp*, const char*, const char*, const char*, Htdoc* doc))dlsym((*m_ext_list)[x].handle, "ext_response");
            const char* errmsg;
            if((errmsg = dlerror()) == NULL)
            {
                ext_response(this, (*m_ext_list)[x].name.c_str(), (*m_ext_list)[x].description.c_str(), (*m_ext_list)[x].parameters.c_str(), doc);
            }
        }
        doc->Response();
        
        if(m_http2)
        {
            m_http2->SendHttp2EmptyContent(m_http2_stream_ind);
            m_http2->SendHttp2PushPromiseResponse();
        }
        
        //3rd extension hook
        for(int x = 0; x < m_ext_list->size(); x++)
        {
            void* (*ext_finish)(CHttp*, const char*, const char*, const char*, Htdoc*);
            ext_finish = (void*(*)(CHttp*, const char*, const char*, const char*, Htdoc* doc))dlsym((*m_ext_list)[x].handle, "ext_finish");
            const char* errmsg;
            if((errmsg = dlerror()) == NULL)
            {
                ext_finish(this, (*m_ext_list)[x].name.c_str(), (*m_ext_list)[x].description.c_str(), (*m_ext_list)[x].parameters.c_str(), doc);
            }
        }

        delete doc;
    }
}

Http_Connection CHttp::ResponseReply()
{
    if(m_http_state == httpResponse)
    {
        //printf("ResponseReply\n");
        //Authentication
        if(m_http_tunneling_connection == HTTP_Tunneling_None)
        {
            if((GetWWWAuthScheme() == asBasic || GetWWWAuthScheme() == asDigest)
                && !IsPassedWWWAuth())
            {
                CHttpResponseHdr header(m_response_header.GetMap());
                string strRealm = "User or Administrator";
                header.SetStatusCode(SC401);
                
                unsigned char md5key[17];
                sprintf((char*)md5key, "%016lx", pthread_self());
                
                unsigned char szMD5Realm[16];
                char szHexMD5Realm[33];
                HMAC_MD5((unsigned char*)strRealm.c_str(), strRealm.length(), md5key, 16, szMD5Realm);
                sprintf(szHexMD5Realm, "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x", 
                    szMD5Realm[0], szMD5Realm[1], szMD5Realm[2], szMD5Realm[3],
                    szMD5Realm[4], szMD5Realm[5], szMD5Realm[6], szMD5Realm[7],
                    szMD5Realm[8], szMD5Realm[9], szMD5Realm[10], szMD5Realm[11],
                    szMD5Realm[12], szMD5Realm[13], szMD5Realm[14], szMD5Realm[15]);
                        
                string strVal;
                if(GetWWWAuthScheme() == asBasic)
                {
                    strVal = "Basic realm=\"";
                    strVal += strRealm;
                    strVal += "\"";
                }
                else if(GetWWWAuthScheme() == asDigest)
                {
                    struct timeval tval;
                    struct timezone tzone;
                    gettimeofday(&tval, &tzone);
                    char szNonce[35];
                    srandom(time(NULL));
                    unsigned long long thisp64 = (unsigned long long)this;
                    thisp64 <<= 32;
                    thisp64 >>= 32;
                    unsigned long thisp32 = (unsigned long)thisp64;
                    
                    sprintf(szNonce, "%08x%016lx%08x%02x", tval.tv_sec, tval.tv_usec + 0x01B21DD213814000ULL, thisp32, random()%255);
                    
                    strVal = "Digest realm=\"";
                    strVal += strRealm;
                    strVal += "\", qop=\"auth,auth-int\", nonce=\"";
                    strVal += szNonce;
                    strVal += "\", opaque=\"";
                    strVal += szHexMD5Realm;
                    strVal += "\"";
                    
                }
                //printf("%s\n", strVal.c_str());
                header.SetField("WWW-Authenticate", strVal.c_str());
                
                header.SetField("Content-Type", "text/html");
                header.SetField("Content-Length", header.GetDefaultHTMLLength());
                
                SendHeader(header.Text(), header.Length());
                SendContent(header.GetDefaultHTML(), header.GetDefaultHTMLLength());
                return httpContinue;
            }
        }
        else
        {
            if((GetProxyAuthScheme() == asBasic || GetProxyAuthScheme() == asDigest)
                && !IsPassedProxyAuth())
            {
                CHttpResponseHdr header(m_response_header.GetMap());
                header.SetStatusCode(SC407);
                string strRealm = "Proxy User or Administrator";
                
                unsigned char md5key[17];
                sprintf((char*)md5key, "%016lx", pthread_self());
                
                unsigned char szMD5Realm[16];
                char szHexMD5Realm[33];
                HMAC_MD5((unsigned char*)strRealm.c_str(), strRealm.length(), md5key, 16, szMD5Realm);
                sprintf(szHexMD5Realm, "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x", 
                    szMD5Realm[0], szMD5Realm[1], szMD5Realm[2], szMD5Realm[3],
                    szMD5Realm[4], szMD5Realm[5], szMD5Realm[6], szMD5Realm[7],
                    szMD5Realm[8], szMD5Realm[9], szMD5Realm[10], szMD5Realm[11],
                    szMD5Realm[12], szMD5Realm[13], szMD5Realm[14], szMD5Realm[15]);
                        
                string strVal;
                if(GetProxyAuthScheme() == asBasic)
                {
                    strVal = "Basic realm=\"";
                    strVal += strRealm;
                    strVal += "\"";
                }
                else if(GetProxyAuthScheme() == asDigest)
                {
                    
                    struct timeval tval;
                    struct timezone tzone;
                    gettimeofday(&tval, &tzone);
                    char szNonce[35];
                    srandom(time(NULL));
                    unsigned long long thisp64 = (unsigned long long)this;
                    thisp64 <<= 32;
                    thisp64 >>= 32;
                    unsigned long thisp32 = (unsigned long)thisp64;
                    
                    sprintf(szNonce, "%08x%016lx%08x%02x", tval.tv_sec, tval.tv_usec + 0x01B21DD213814000ULL, thisp32, random()%255);
                    
                    strVal = "Digest realm=\"";
                    strVal += strRealm;
                    strVal += "\", qop=\"auth,auth-int\", nonce=\"";
                    strVal += szNonce;
                    strVal += "\", opaque=\"";
                    strVal += szHexMD5Realm;
                    strVal += "\"";
                    
                }
                //printf("%s\n", strVal.c_str());
                header.SetField("Proxy-Authenticate", strVal.c_str());
                
                header.SetField("Content-Type", "text/html");
                header.SetField("Content-Length", header.GetDefaultHTMLLength());
                
                SendHeader(header.Text(), header.Length());
                SendContent(header.GetDefaultHTML(), header.GetDefaultHTMLLength());
                return httpContinue;
            }
        }
        
        //go ahead after authentication
        if(m_http_tunneling_connection == HTTP_Tunneling_None)
        {
            if(m_http_method == hmPut || m_http_method == hmDelete)
            {
                CHttpResponseHdr header(m_response_header.GetMap());
                header.SetStatusCode(SC405);

                header.SetField("Content-Type", "text/html");
                header.SetField("Content-Length", header.GetDefaultHTMLLength());
                SendHeader(header.Text(), header.Length());
                SendContent(header.GetDefaultHTML(), header.GetDefaultHTMLLength());
            }
            else
            {
                Response();
            }
        }
        else
        {
            if(!CHttpBase::m_enable_http_tunneling && (m_http_tunneling_connection == HTTP_Tunneling_Without_CONNECT || m_http_tunneling_connection == HTTP_Tunneling_Without_CONNECT_SSL))
            {
                CHttpResponseHdr header(m_response_header.GetMap());
                header.SetStatusCode(SC404);

                header.SetField("Content-Type", "text/html");
                header.SetField("Content-Length", header.GetDefaultHTMLLength());
                SendHeader(header.Text(), header.Length());
                SendContent(header.GetDefaultHTML(), header.GetDefaultHTMLLength());
            }
            else if(!CHttpBase::m_enable_http_tunneling && m_http_tunneling_connection == HTTP_Tunneling_With_CONNECT)
            {
                CHttpResponseHdr header(m_response_header.GetMap());
                header.SetStatusCode(SC405);

                header.SetField("Content-Type", "text/html");
                header.SetField("Content-Length", header.GetDefaultHTMLLength());
                SendHeader(header.Text(), header.Length());
                SendContent(header.GetDefaultHTML(), header.GetDefaultHTMLLength());
            }
            else
            {
                Tunneling();
            }
        }
        
        m_http_state = httpComplete;
    }
    return (m_keep_alive && m_enabled_keep_alive) ? httpKeepAlive : httpClose;
}

Http_Connection CHttp::DataParse()
{    
    if(m_http_state == httpReqData)
    {
        //printf("DataParse\n");
        if(m_content_length < 0)
        {
            CHttpResponseHdr header(m_response_header.GetMap());
            header.SetStatusCode(SC411);

            header.SetField("Content-Type", "text/html");
            header.SetField("Content-Length", header.GetDefaultHTMLLength());
            SendHeader(header.Text(), header.Length());
            SendContent(header.GetDefaultHTML(), header.GetDefaultHTMLLength());
            
            return httpClose;
        }
        
        if(m_content_length >= 0)
        {
#ifdef _WITH_ASYNC_
            AsyncRecvPostData();
#else
            RecvPostData();  
#endif /* _WITH_ASYNC_ */
        }
    }
#ifdef _WITH_ASYNC_
    return httpContinue;
#else
    return ResponseReply();
#endif /* _WITH_ASYNC_ */
            
}

Http_Connection CHttp::LineParse(const char* text)
{
    string strtext;
    m_line_text += text;
    std::size_t new_line;
    while((new_line = m_line_text.find('\n')) != std::string::npos)
    {
        strtext = m_line_text.substr(0, new_line + 1);
        m_line_text = m_line_text.substr(new_line + 1);

        strtrim(strtext);
        /* printf("<<<< %s\r\n", strtext.c_str()); */
        BOOL High = TRUE;
        for(int c = 0; c < strtext.length(); c++)
        {
            if(High)
            {
                strtext[c] = HICH(strtext[c]);
                High = FALSE;
            }
            if(strtext[c] == '-')
                High = TRUE;
            if(strtext[c] == ':' || strtext[c] == ' ')
                break;
        }
        if(strncasecmp(strtext.c_str(),"GET ", 4) == 0)
        {	
            m_cgi.SetMeta("REQUEST_METHOD", "GET");
                    
            m_http_method = hmGet;
            m_request_hdr.SetMethod(m_http_method);
            ParseMethod(strtext);
            //store the header content
            m_header_content += strtext;
            m_header_content += "\r\n";
        }
        else if(strncasecmp(strtext.c_str(), "POST ", 5) == 0)
        {
            m_cgi.SetMeta("REQUEST_METHOD", "POST");
            
            m_http_method = hmPost;
            m_request_hdr.SetMethod(m_http_method);

            ParseMethod(strtext);
            //store the header content
            m_header_content += strtext;
            m_header_content += "\r\n";
        }
        else if(strncasecmp(strtext.c_str(), "PUT ", 4) == 0)
        {
            m_cgi.SetMeta("REQUEST_METHOD", "PUT");
                    
            m_http_method = hmPut;
            m_request_hdr.SetMethod(m_http_method);

            ParseMethod(strtext);
            //store the header content
            m_header_content += strtext;
            m_header_content += "\r\n";
        }
        else if(strncasecmp(strtext.c_str(), "HEAD ", 5) == 0)
        {
            m_cgi.SetMeta("REQUEST_METHOD", "HEAD");
                    
            m_http_method = hmHead;
            m_request_hdr.SetMethod(m_http_method);

            ParseMethod(strtext);
            //store the header content
            m_header_content += strtext;
            m_header_content += "\r\n";
        }
        else if(strncasecmp(strtext.c_str(), "DELETE ", 7) == 0)
        {
            m_cgi.SetMeta("REQUEST_METHOD", "DELETE");
                    
            m_http_method = hmDelete;
            m_request_hdr.SetMethod(m_http_method);

            ParseMethod(strtext);
            //store the header content
            m_header_content += strtext;
            m_header_content += "\r\n";
        }
        else if(strncasecmp(strtext.c_str(), "OPTIONS ", 8) == 0)
        {
            m_cgi.SetMeta("REQUEST_METHOD", "OPTIONS");
                    
            m_http_method = hmOptions;
            m_request_hdr.SetMethod(m_http_method);

            ParseMethod(strtext);
            //store the header content
            m_header_content += strtext;
            m_header_content += "\r\n";
        }
        else if(strncasecmp(strtext.c_str(), "TRACE ", 6) == 0)
        {
            m_cgi.SetMeta("REQUEST_METHOD", "TRACE");
                    
            m_http_method = hmTrace;
            m_request_hdr.SetMethod(m_http_method);

            ParseMethod(strtext);
            //store the header content
            m_header_content += strtext;
            m_header_content += "\r\n";
        }
        else if(strncasecmp(strtext.c_str(), "CONNECT ", 8) == 0)
        {
            m_cgi.SetMeta("REQUEST_METHOD", "CONNECT");
                    
            m_http_method = hmConnect;
            m_request_hdr.SetMethod(m_http_method);

            ParseMethod(strtext);
            //store the header content
            m_header_content += strtext;
            m_header_content += "\r\n";
        }
        else if(strcasecmp(strtext.c_str(), "") == 0) /* if true, then http request header finished. */
        {
            //store the header content
            m_header_content += "\r\n";
            m_http_state = httpReqData;
#ifdef _WITH_ASYNC_
            return httpContinue;
#else
            return DataParse();
#endif /* _WITH_ASYNC_ */ 
        }
        else
        {
            //store the header content except the proxy authentication
            if(strncasecmp(strtext.c_str(), "Proxy-Authorization:", 20) != 0)
            {
                m_header_content += strtext;
                m_header_content += "\r\n";
            }
            
            /* Protocol-Specific Meta-Variables for CGI */
            string strSpecVarName, strSpecVarValue;
            strcut(strtext.c_str(), NULL, ":", strSpecVarName);
            strtrim(strSpecVarName);
            if(strSpecVarName != "")
            {
                strcut(strtext.c_str(), ":", NULL, strSpecVarValue);
                strtrim(strSpecVarValue);
                
                m_request_hdr.SetField(strSpecVarName.c_str(), strSpecVarValue.c_str());
                
                /* For CGI Env Val*/
                strSpecVarName = "HTTP_" + strSpecVarName;
                Replace(strSpecVarName, "-", "_");
                Toupper(strSpecVarName);
                m_cgi.SetMeta(strSpecVarName.c_str(), strSpecVarValue.c_str());
            }
            if(strncasecmp(strtext.c_str(),"Connection:", 11) == 0)
            {
                string strConnection;
                strcut(strtext.c_str(), "Connection:", NULL, strConnection);
                strtrim(strConnection);
                if(strcasestr(strConnection.c_str(), "Close") != NULL)
                {
                    m_keep_alive = FALSE;
                }
                else if(strcasestr(strConnection.c_str(), "Keep-Alive") != NULL)
                {
                    m_keep_alive = TRUE;
                }
                
                if(strcasestr(strConnection.c_str(), "Upgrade") != NULL)
                {
                    m_protocol_upgrade = TRUE;
                    //in case "Upgrade: websocket" is in front of "Connection: ... Upgrade ..."
                    if(m_upgrade_protocol != "" && strcasecmp(m_upgrade_protocol.c_str(), "websocket") == 0)
                    {
                        m_web_socket_handshake = Websocket_Sync;
                    }
                }
                
            }
            else if(strncasecmp(strtext.c_str(),"Upgrade:", 8) == 0)
            {
                if(m_protocol_upgrade)
                {
                    strcut(strtext.c_str(), "Upgrade:", NULL, m_upgrade_protocol);
                    strtrim(m_upgrade_protocol);
                    if(strcasecmp(m_upgrade_protocol.c_str(), "websocket") == 0)
                    {
                        m_web_socket_handshake = Websocket_Sync;
                    }
                }
            }
            else if(strncasecmp(strtext.c_str(),"Content-Length:", 15) == 0)
            {
                string strLen;
                strcut(strtext.c_str(), "Content-Length:", NULL, strLen);
                strtrim(strLen);	
                m_content_length = atoll(strLen.c_str());
            }
            else if(strncasecmp(strtext.c_str(),"Content-Type:", 13) == 0)
            {
                string strType;
                strcut(strtext.c_str(), "Content-Type:", NULL, strType);
                strtrim(strType);
                m_cgi.SetMeta("CONTENT_TYPE", strType.c_str());
                
                if(strtext.find("application/x-www-form-urlencoded", 0, sizeof("application/x-www-form-urlencoded") - 1) != string::npos)
                {
                    m_content_type = application_x_www_form_urlencoded;
                }
                else if(strtext.find("multipart/form-data", 0, sizeof("multipart/form-data") - 1) != string::npos) 
                {
                    m_content_type = multipart_form_data;
                    strcut(strtext.c_str(), "boundary=", NULL, m_boundary);
                    strtrim(m_boundary);
                }
                else
                {
                    m_content_type = application_x_www_form_urlencoded;
                }
            }
            else if(strncasecmp(strtext.c_str(), "Cookie:", 7) == 0)
            {
                //printf("%s\n", strtext.c_str());
                string strcookie;
                strcut(strtext.c_str(), "Cookie:", NULL, strcookie);
                strtrim(strcookie);
                m_cookie += strcookie;
                m_cookie += "; ";
            }
            else if(strncasecmp(strtext.c_str(), "Host:",5) == 0)
            {
                string host_port;
                strcut(strtext.c_str(), "Host: ", NULL, host_port);
                strcut(host_port.c_str(), NULL, ":", host_port);
                m_host_addr = host_port;
                strcut(host_port.c_str(), ":", NULL, host_port);
                m_host_port = (unsigned short)atoi(host_port.c_str());
            }
            else if(strncasecmp(strtext.c_str(), "Authorization: Basic", 20) == 0)
            {
                string strauth;
                strcut(strtext.c_str(), "Authorization: Basic ", NULL, strauth);

                m_cgi.SetMeta("AUTH_TYPE", "Basic");
                
                string php_auth_pwd;
                
                if(WWW_Auth(this, asBasic, CHttpBase::m_integrate_local_users ? true : false, strauth.c_str(), m_username, php_auth_pwd))
                {
                    m_passed_wwwauth = TRUE;
                    m_cgi.SetMeta("REMOTE_USER", m_username.c_str());
                }
                m_cgi.SetMeta("PHP_AUTH_USER", m_username.c_str());
                m_cgi.SetMeta("PHP_AUTH_UPW", php_auth_pwd.c_str());
                
            }
            else if(strncasecmp(strtext.c_str(), "Authorization: Digest", 21) == 0)
            {
                string strauth;
                strcut(strtext.c_str(), "Authorization: Digest ", NULL, strauth);
                
                string php_digest;
                
                m_cgi.SetMeta("AUTH_TYPE", "Digest");
                
                if(WWW_Auth(this, asDigest, CHttpBase::m_integrate_local_users ? true : false, strauth.c_str(), m_username, php_digest, HTTP_METHOD_NAME[m_http_method]))
                {
                    m_passed_wwwauth = TRUE;
                    m_cgi.SetMeta("REMOTE_USER", m_username.c_str());
                }
                
                m_cgi.SetMeta("PHP_AUTH_DIGEST", php_digest.c_str());
            }
            else if(strncasecmp(strtext.c_str(), "Proxy-Authorization: Basic", 26) == 0)
            {
                string strauth;
                strcut(strtext.c_str(), "Proxy-Authorization: Basic ", NULL, strauth);

                string php_auth_pwd;
                
                if(WWW_Auth(this, asBasic, CHttpBase::m_integrate_local_users ? true : false, strauth.c_str(), m_username, php_auth_pwd))
                {
                    m_passed_proxyauth = TRUE;
                }
            }
            else if(strncasecmp(strtext.c_str(), "Proxy-Authorization: Digest", 27) == 0)
            {
                string strauth;
                strcut(strtext.c_str(), "Proxy-Authorization: Digest ", NULL, strauth);
                
                string php_digest;
                
                if(WWW_Auth(this, asDigest, CHttpBase::m_integrate_local_users ? true : false, strauth.c_str(), m_username, php_digest, HTTP_METHOD_NAME[m_http_method]))
                {
                    m_passed_proxyauth = TRUE;
                }
                
            }
            else if(strncasecmp(strtext.c_str(), "Cache-Control:", 14) == 0)
            {
                string strcache;
                strcut(strtext.c_str(), "Cache-Control:", NULL, strcache);
                strtrim(strcache);
                if(strcasecmp(strcache.c_str(), "no-cache") == 0 || strcasecmp(strcache.c_str(), "no-store") == 0)
                {
                    m_request_no_cache = TRUE;
                }
                
            }
            else if(strncasecmp(strtext.c_str(), "Pragma:", 7) == 0)
            {
                string strcache;
                strcut(strtext.c_str(), "Pragma:", NULL, strcache);
                strtrim(strcache);
                if(strcasecmp(strcache.c_str(), "no-cache") == 0)
                {
                    m_request_no_cache = TRUE;
                }
                
            }
            /*  */
            else if(m_web_socket_handshake == Websocket_Sync && strncasecmp(strtext.c_str(), "Sec-WebSocket-Key", 17) == 0)
            {
               //Web-socket
            }
            else if(m_web_socket_handshake == Websocket_Sync && strncasecmp(strtext.c_str(), "Sec-WebSocket-Extensions", 24) == 0)
            {

            }
            else if(m_web_socket_handshake == Websocket_Sync && strncasecmp(strtext.c_str(), "Sec-WebSocket-Protocol", 22) == 0)
            {

            }
            else if(m_web_socket_handshake == Websocket_Sync && strncasecmp(strtext.c_str(), "Sec-WebSocket-Version", 21) == 0)
            {

            }
        }
    }
	return httpContinue;
}

int CHttp::parse_multipart_formdata(const char* content_name, string& content_filename, string& content_filetype, const char* &content_valbuf, int& content_vallen)
{
    if(m_content_type == multipart_form_data)
	{
        content_filename = "";
        content_filetype = "";
        content_valbuf = NULL;
        content_vallen = 0;
        
		for(int x = 0; x < m_formdata->m_paramters.size(); x++)
		{			
			char* szHeader = (char*)malloc(m_formdata->m_paramters[x].m_header.m_byte_end - m_formdata->m_paramters[x].m_header.m_byte_beg + 2);
			memcpy(szHeader, m_formdata->c_buffer() + m_formdata->m_paramters[x].m_header.m_byte_beg, m_formdata->m_paramters[x].m_header.m_byte_end - m_formdata->m_paramters[x].m_header.m_byte_beg + 1);
			szHeader[m_formdata->m_paramters[x].m_header.m_byte_end - m_formdata->m_paramters[x].m_header.m_byte_beg + 1] = '\0';
			string current_name;
			fnfy_strcut(szHeader, " name=", " \t\r\n\"", "\r\n\";", current_name);
			if(current_name == "")
            {
				fnfy_strcut(szHeader, "\tname=", " \t\r\n\"", "\r\n\";", current_name);
				if(current_name == "")
                {
					fnfy_strcut(szHeader, ";name=", " \t\r\n\"", "\r\n\";", current_name);
					if(current_name == "")
                    {
						fnfy_strcut(szHeader, "name=", " \t\r\n\"", "\r\n\";", current_name);
                    }
                }
            }
			if(strcmp(current_name.c_str(), content_name) == 0)
			{
				fnfy_strcut(szHeader, "filename=", " \t\r\n\"", "\r\n\";", content_filename);
                fnfy_strcut(szHeader, "Content-Type:", " \t\r\n\"", "\r\n\";", content_filetype);
                
                content_valbuf = (char* )GetFormData()->c_buffer() + m_formdata->m_paramters[x].m_data.m_byte_beg;
                content_vallen = m_formdata->m_paramters[x].m_data.m_byte_end >= m_formdata->m_paramters[x].m_data.m_byte_beg ? (m_formdata->m_paramters[x].m_data.m_byte_end - m_formdata->m_paramters[x].m_data.m_byte_beg + 1) : 0;
                
                free(szHeader);
				return 0;
			}
			else
			{
				free(szHeader);
			}
		}
	}
	return -1;
}

void CHttp::SetMetaVar(const char* name, const char* val)
{
	m_cgi.m_meta_var[name] = val;
}

void CHttp::SetMetaVarsToEnv()
{
	map<string, string>::iterator it;
	for(it = m_cgi.m_meta_var.begin(); it != m_cgi.m_meta_var.end(); ++it)
	{
		setenv(it->first.c_str(), it->second.c_str(), 1);
	}
	
}

void CHttp::SetServiceObject(const char * objname, IServiceObj* objptr)
{
   m_srvobj->SetObj(objname, objptr);
}

IServiceObj* CHttp::GetServiceObject(const char * objname)
{
   return m_srvobj->GetObj(objname);
}

void CHttp::Http2PushPromise(const char * path)
{
    if(m_http2)
        m_http2->PushPromise(m_http2_stream_ind, path);
}

BOOL CHttp::IsHttp2()
{
    return m_http2 ? TRUE : FALSE;
}

BOOL CHttp::GetClientCertCommonName(vector<string>& common_names)
{
    if(m_ssl)
    {
        X509* client_cert;
        client_cert = SSL_get_peer_certificate(m_ssl);
        if (client_cert != NULL)
        {
            X509_NAME * owner = X509_get_subject_name(client_cert);
            const char * owner_buf = X509_NAME_oneline(owner, 0, 0);
            
            const char* commonName;
            
            int lastpos = -1;
            X509_NAME_ENTRY *e;
            for (;;)
            {
                lastpos = X509_NAME_get_index_by_NID(owner, NID_commonName, lastpos);
                if (lastpos == -1)
                    break;
                e = X509_NAME_get_entry(owner, lastpos);
                if(!e)
                    break;
                ASN1_STRING *d = X509_NAME_ENTRY_get_data(e);
                char *commonName = (char*)ASN1_STRING_data(d);
                if(commonName)
                    common_names.push_back(commonName);
            }

            X509_free (client_cert);
            
            return TRUE;
        }
        else
        {
            return FALSE;
        }
    }
    else
    {
        return FALSE;
    }
}
