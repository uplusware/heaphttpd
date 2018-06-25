/*
	Copyright (c) openheap, uplusware
	uplusware@gmail.com
*/

#ifndef _HTTP_H_
#define _HTTP_H_
#include "backend_session.h"
#include "httpcomm.h"
#include "formdata.h"
#include "cache.h"
#include "webcgi.h"
#include "wwwauth.h"
#include "serviceobjmap.h"
#include "extension.h"
#include "fastcgi.h"
#include "scgi.h"

class http_tunneling;

#include "tunneling.h"

enum WebSocket_HandShake
{
    Websocket_None = 0,
    Websocket_Sync,
    Websocket_Ack,
    Websocket_Nak
};

enum Http_Connection
{
    httpContinue = 0,
    httpKeepAlive,
    httpClose,
	httpWebSocket
};

enum Http_State
{
    httpReqHeader = 0,
    httpReqData,
    httpAuthentication,
    httpResponse,
    httpResponding,
    httpRespondingExtension1 = httpResponding,
    httpRespondingExtensionComplete1,
    httpRespondingDocNew = httpRespondingExtensionComplete1,
    httpRespondingExtension2 = httpRespondingDocNew,
    httpRespondingExtensionComplete2,
    httpRespondingDocOngoing = httpRespondingExtensionComplete2,
    httpRespondingDocComplete,
    httpRespondingExtension3 = httpRespondingDocComplete,
    httpRespondingExtensionComplete3,
	httpResponseComplete = httpRespondingExtensionComplete3,
	httpTunnling,
	httpTunnlingExtension = httpTunnling,
    httpTunnlingExtensionComplete,
    httpTunnlingRelaying = httpTunnlingExtensionComplete,
	httpTunnlingComplete,
    httpComplete
};

class IHttp
{
public:
    IHttp()
    {
        m_async_send_buf = NULL;
        m_async_send_data_len = 0;
        m_async_send_buf_size = 0;
        
        m_async_recv_buf = NULL;
        m_async_recv_data_len = 0;
        m_async_recv_buf_size = 0;
    
    }
    virtual ~IHttp()
    {
        if(m_async_send_buf)
            free(m_async_send_buf);
        m_async_send_buf = NULL;
        m_async_send_data_len = 0;
        m_async_send_buf_size = 0;
        
        if(m_async_recv_buf)
            free(m_async_recv_buf);
        m_async_recv_buf = NULL;
        m_async_recv_data_len = 0;
        m_async_recv_buf_size = 0;
    } //Must be have, otherwise there will be memory leak for CHttp and CHttp2
    
    virtual Http_Connection Processing() = 0;
    virtual Http_Connection AsyncProcessing() = 0;
    virtual int HttpSend(const char* buf, int len) = 0;
    virtual int HttpRecv(char* buf, int len) = 0;
    virtual int AsyncHttpSend(const char* buf, int len) = 0;
    virtual int AsyncHttpRecv(char* buf, int len) = 0;
    virtual int AsyncSend() = 0;
    virtual int AsyncRecv() = 0;
    virtual http_tunneling* GetHttpTunneling() = 0;
    virtual fastcgi* GetPhpFpm() = 0;
protected:
    char* m_async_send_buf;
    int m_async_send_data_len;
    int m_async_send_buf_size;
    
    char* m_async_recv_buf;
    int m_async_recv_data_len;
    int m_async_recv_buf_size;
    
};

#include "htdoc.h"
class Htdoc;

#include "http2.h"
class CHttp2;

class CHttp : public IHttp
{
public:
    friend class CHttp2;
	CHttp(int epoll_fd, map<int, backend_session*>* backend_list, time_t connection_first_request_time, time_t connection_keep_alive_timeout, unsigned int connection_keep_alive_request_tickets,
        http_tunneling* tunneling, fastcgi* php_fpm_instance, map<string, fastcgi*>* fastcgi_instances, map<string, scgi*>* scgi_instances, ServiceObjMap* srvobj, int sockfd, const char* servername, unsigned short serverport,
	    const char* clientip, X509* client_cert, memory_cache* ch,
		const char* work_path, vector<string>* default_webpages, vector<http_extension_t>* ext_list, vector<http_extension_t>* reverse_ext_list, const char* php_mode, 
        cgi_socket_t fpm_socktype, const char* fpm_sockfile, 
        const char* fpm_addr, unsigned short fpm_port, const char* phpcgi_path,
        map<string, cgi_cfg_t>* cgi_list,
        const char* private_path, AUTH_SCHEME wwwauth_scheme = asNone, AUTH_SCHEME proxyauth_scheme = asNone,
		SSL* ssl = NULL, CHttp2* phttp2 = NULL, uint_32 http2_stream_ind = 0);
        
	virtual ~CHttp();

	Http_Connection LineParse(const char* text);
    Http_Connection DataParse();
    Http_Connection ResponseReply();
    
	virtual int HttpSend(const char* buf, int len);
    virtual int HttpRecv(char* buf, int len);
    virtual int AsyncHttpSend(const char* buf, int len);
    virtual int AsyncHttpRecv(char* buf, int len);
    virtual int AsyncHttpFlush();
    virtual int AsyncSend();
    virtual int AsyncRecv();
	virtual Http_Connection Processing();
    virtual Http_Connection AsyncProcessing();
    
    int ProtRecv(char* buf, int len, int alive_timeout);
    int AsyncProtRecv(char* buf, int len);
    
    void PushPostData(const char* buf, int len);
    void RecvPostData();
    void AsyncRecvPostData();
    int Response();
    int Tunneling();
    
    void SetCookie(const char* szName, const char* szValue,
        int nMaxAge = -1, const char* szExpires = NULL,
        const char* szPath = NULL, const char* szDomain = NULL, 
        BOOL bSecure = FALSE, BOOL bHttpOnly = FALSE);
        
    void SetSessionVar(const char* szName, const char* szValue);
	int GetSessionVar(const char* szName, string& strValue);
	
	void SetServerVar(const char* szName, const char* szValue);
	int GetServerVar(const char* szName, string& strValue);
	
	int SendHeader(const char* buf, int len);
	int SendContent(const char* buf, int len);
	
	void SetMetaVarsToEnv();
	void SetMetaVar(const char* name, const char* val);
	
    int parse_multipart_formdata(const char* content_name, string& content_filename, string& content_filetype, const char* &content_valbuf, int& content_vallen);
	
	const char* GetResource() { return m_resource.c_str(); }
	const char* GetUserName() { return m_username.c_str(); }
	const char* GetPassword() { return m_password.c_str(); }
    
    BOOL GetClientCertCommonName(vector<string>& common_names);
	
	formdata* GetFormData() { return m_formdata; }

	void SetResource(const char* resource)
	{ 
		m_resource = resource;
	}
    
    virtual http_tunneling* GetHttpTunneling() { return m_http_tunneling; }
    virtual fastcgi* GetPhpFpm() { return m_php_fpm_instance; }
    
	memory_cache* m_cache;
    
    memory_cache* GetCache() { return m_cache; }
    
	string m_servername;
	unsigned short m_serverport;
	string m_clientip;
	
	Http_Method GetMethod()
	{
		return m_http_method;
	}
	
    const char* GetRequestField(const char* name)
    {
        return m_request_hdr.GetField(name);
    }
    
    void GetRequestField(const char* name, int & val)
    {
        m_request_hdr.GetField(name, val);
    }
    
    const char* GetPostDataVar(const char* key);
    const char* GetQueryStringVar(const char* key);    
    const char* GetCookieVar(const char* key);
    
    BOOL RequestNoCache() { return m_request_no_cache; }
    
	BOOL IsPassedWWWAuth() { return m_passed_wwwauth; }
    BOOL IsPassedProxyAuth() { return m_passed_proxyauth; }
	AUTH_SCHEME GetWWWAuthScheme() { return m_wwwauth_scheme; };
    AUTH_SCHEME GetProxyAuthScheme() { return m_proxyauth_scheme; };
    
    WebSocket_HandShake GetWebSocketHandShake() { return m_web_socket_handshake; }
	void SetWebSocketHandShake(WebSocket_HandShake shake) { m_web_socket_handshake = shake;}
	
	int GetSocket() { return m_sockfd; }
    SSL* GetSSL() { return m_ssl; }
    void SetServiceObject(const char * objname, IServiceObj* objptr);
    IServiceObj* GetServiceObject(const char* objname);
	WebCGI m_cgi;
	
	X509* GetClientX509Cert() { return m_client_cert; }
    
    BOOL GetKeepAlive() { return m_keep_alive; }
    BOOL IsEnabledKeepAlive() { return m_enabled_keep_alive; }
    
    void EnableKeepAlive(BOOL keep_alive) { m_enabled_keep_alive = keep_alive; }
    
    long long GetContentLength() { return m_content_length; }
    
    void Http2PushPromise(const char * path);
    BOOL IsHttp2();
    
    CHttpResponseHdr* GetResponseHeader() { return &m_response_header; }
    
    vector<string>* GetHttp2PushPromiseList() { return &m_http2_push_promise_list; }
    
    void PushHttp2PushPromiseFile(const char* file_path);
    
private:
    void ParseMethod(string & strtext);
protected:
    map<string, string> _POST_VARS_; /* var in post data */
	map<string, string> _GET_VARS_; /* var in query string */
    map<string, string> _COOKIE_VARS_; /* var in cookie */
    
    Htdoc * m_http_doc;
    
	SSL* m_ssl;
	CHttp2 * m_http2;
    uint_32 m_http2_stream_ind;
	int m_sockfd;
    int m_epoll_fd;
	linesock* m_lsockfd;
	linessl * m_lssl;
	string m_line_text;
	
	X509* m_client_cert;
	vector<http_extension_t>* m_ext_list;
    vector<http_extension_t>* m_reverse_ext_list;
    vector<string>* m_default_webpages;
	Http_Method m_http_method;
	string m_resource;
	string m_uri;
	string m_querystring;
	string m_postdata;
	
	fbuffer* m_postdata_ex; /* huge size post data */
	formdata* m_formdata; /* For upload files */
	
	long long m_content_length;

	Post_Content_Type m_content_type;
	string m_boundary;

	string m_host_addr;
	unsigned short m_host_port;

	string m_username;
	string m_password;

	string m_cookie;
    string m_session_var_uid;
    
    string m_work_path;

	string m_php_mode;
    cgi_socket_t m_fpm_socktype;
    string m_fpm_sockfile;
	string m_fpm_addr;
	unsigned short m_fpm_port;
    string m_phpcgi_path;
    string m_private_path;
   
    map<string, cgi_cfg_t>* m_cgi_list;
    
    CHttpRequestHdr m_request_hdr;
	
	BOOL m_passed_wwwauth;
    BOOL m_passed_proxyauth;
	AUTH_SCHEME m_wwwauth_scheme;
    AUTH_SCHEME m_proxyauth_scheme;
    BOOL m_protocol_upgrade;
    string m_upgrade_protocol;
    
    BOOL m_keep_alive;
    BOOL m_enabled_keep_alive;
    WebSocket_HandShake m_web_socket_handshake;
    ServiceObjMap* m_srvobj;
    
    map<string, string> m_set_cookies;
    
    string m_header_content;
    
    HTTPTunneling m_http_tunneling_connection;
    
    string m_http_tunneling_backend_address;
    unsigned short m_http_tunneling_backend_port;
    string m_http_tunneling_url;
    
    //back backends is only availiable in reverse proxy enabled;
    string m_http_tunneling_backend_address_backup1;
    unsigned short m_http_tunneling_backend_port_backup1;
    string m_http_tunneling_url_backup1;
    
    string m_http_tunneling_backend_address_backup2;
    unsigned short m_http_tunneling_backend_port_backup2;
    string m_http_tunneling_url_backup2;
       
    
    http_tunneling* m_http_tunneling;
    fastcgi* m_php_fpm_instance;
    map<string, fastcgi*>* m_fastcgi_instances;
    map<string, scgi*>* m_scgi_instances;
    
    BOOL m_request_no_cache;
    
    CHttpResponseHdr m_response_header;
    
    time_t m_connection_first_request_time;
    time_t m_connection_keep_alive_timeout;
    unsigned int m_connection_keep_alive_request_tickets;
    
    Http_State m_http_state;
    
    BOOL m_tunneling_ext_handled;
    BOOL m_backend_connected;
    BOOL m_tunneling_connection_established;
    map<int, backend_session*>* m_backend_list;
    vector<string> m_http2_push_promise_list;
};

#endif /* _HTTP_H_ */

