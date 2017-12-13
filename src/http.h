/*
	Copyright (c) openheap, uplusware
	uplusware@gmail.com
*/

#ifndef _HTTP_H_
#define _HTTP_H_

#include "httpcomm.h"
#include "formdata.h"
#include "cache.h"
#include "webcgi.h"
#include "wwwauth.h"
#include "serviceobjmap.h"
#include "extension.h"
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

class IHttp
{
public:
    IHttp() {}
    virtual ~IHttp() {} //Must be have, otherwise there will be memory leak for CHttp and CHttp2
    
    virtual Http_Connection Processing() = 0;
    virtual int HttpSend(const char* buf, int len) = 0;
    virtual int HttpRecv(char* buf, int len) = 0;
    virtual http_tunneling* GetHttpTunneling() = 0;
};

#include "http2.h"
class CHttp2;
class CHttp : public IHttp
{
public:
    friend class CHttp2;
	CHttp(time_t connection_first_request_time, time_t connection_keep_alive_timeout, unsigned int connection_keep_alive_request_tickets, http_tunneling* tunneling, ServiceObjMap* srvobj, int sockfd, const char* servername, unsigned short serverport,
	    const char* clientip, X509* client_cert, memory_cache* ch,
		const char* work_path, vector<string>* default_webpages, vector<http_extension_t>* ext_list, vector<http_extension_t>* reverse_ext_list, const char* php_mode, 
        cgi_socket_t fpm_socktype, const char* fpm_sockfile, 
        const char* fpm_addr, unsigned short fpm_port, const char* phpcgi_path,
        map<string, cgi_cfg_t>* cgi_list,
        const char* private_path, AUTH_SCHEME wwwauth_scheme = asNone, AUTH_SCHEME proxyauth_scheme = asNone,
		SSL* ssl = NULL, CHttp2* phttp2 = NULL, uint_32 http2_stream_ind = 0);
        
	virtual ~CHttp();

	Http_Connection LineParse(const char* text);
    
	virtual int HttpSend(const char* buf, int len);
    virtual int HttpRecv(char* buf, int len);
	virtual Http_Connection Processing();
    
    int ProtRecv(char* buf, int len, int alive_timeout);
    void PushPostData(const char* buf, int len);
    void RecvPostData();
    void Response();
    void Tunneling();
    
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
    
    BOOL RequestNoCache() { return m_request_no_cache; }
    
    map<string, string> _POST_VARS_; /* var in post data */
	map<string, string> _GET_VARS_; /* var in query string */
    map<string, string> _COOKIE_VARS_; /* var in cookie */
    
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
    
private:
    void ParseMethod(string & strtext);
protected:
	SSL* m_ssl;
	CHttp2 * m_http2;
    uint_32 m_http2_stream_ind;
	int m_sockfd;
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
    
    BOOL m_request_no_cache;
    
    CHttpResponseHdr m_response_header;
    
    time_t m_connection_first_request_time;
    time_t m_connection_keep_alive_timeout;
    unsigned int m_connection_keep_alive_request_tickets;
};

#endif /* _HTTP_H_ */

