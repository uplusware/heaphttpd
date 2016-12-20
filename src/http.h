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
#include "serviceobj.h"
#include "extension.h"

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
};

#include "http2.h"
class CHttp2;
class CHttp : public IHttp
{
public:
    friend class CHttp2;
	CHttp(ServiceObjMap* srvobj, int sockfd, const char* servername, unsigned short serverport,
	    const char* clientip, X509* client_cert, memory_cache* ch,
		const char* work_path, vector<stExtension>* ext_list, const char* php_mode, 
        const char* fpm_socktype, const char* fpm_sockfile, 
        const char* fpm_addr, unsigned short fpm_port, const char* phpcgi_path,
        map<string, cgi_cfg_t>* cgi_list,
        const char* private_path, AUTH_SCHEME wwwauth_scheme = asNone,
		SSL* ssl = NULL, CHttp2* phttp2 = NULL, uint_32 http2_stream_ind = 0);
        
	virtual ~CHttp();

	Http_Connection LineParse(const char* text);
    
	virtual int HttpSend(const char* buf, int len);
    virtual int HttpRecv(char* buf, int len);
	virtual Http_Connection Processing();
    
    int ProtRecv(char* buf, int len);
    void PushPostData(const char* buf, int len);
    void RecvPostData();
    void Response();
    
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
	void WriteDataToCGI(int fd);
    void ReadDataFromCGI(int fd, string& strResponse);
	
	int parse_multipart_value(const char* szKey, fbufseg & seg);
	int parse_multipart_filename(const char* szKey, string& filename);
	int parse_multipart_type(const char* szKey, string& type);

	
	const char* GetResource() { return m_resource.c_str(); }
	const char* GetUserName() { return m_username.c_str(); }
	const char* GetPassword() { return m_password.c_str(); }
	
	formdata* GetFormData() { return m_formdata; }

	void SetResource(const char* resource)
	{ 
		m_resource = resource;
	}

	memory_cache* m_cache;

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

    map<string, string> _POST_VARS_; /* var in post data */
	map<string, string> _GET_VARS_; /* var in query string */
    map<string, string> _COOKIE_VARS_; /* var in cookie */
    
	BOOL IsPassedWWWAuth() { return m_passed_wwwauth; }
	AUTH_SCHEME GetWWWAuthScheme() { return m_wwwauth_scheme; };
    
    WebSocket_HandShake GetWebSocketHandShake() { return m_web_socket_handshake; }
	void SetWebSocketHandShake(WebSocket_HandShake shake) { m_web_socket_handshake = shake;}
	
	int GetSocket() { return m_sockfd; }
    SSL* GetSSL() { return m_ssl; }
    void SetServiceObject(const char * objname, SERVICE_OBJECT_HANDLE objptr);
    void* GetServiceObject(const char* objname);
	WebCGI m_cgi;
	
	X509* GetClientX509Cert() { return m_client_cert; }
	
	void SetExtensionDate(void* extdata) { m_extdata = extdata; }
	void* GetExtensionDate() { return m_extdata; }
    
    BOOL GetKeepAlive() { return m_keep_alive; }
    BOOL IsEnabledKeepAlive() { return m_enabled_keep_alive; }
    
    void EnableKeepAlive(BOOL keep_alive) { m_enabled_keep_alive = keep_alive; }
    
    unsigned int GetContentLength() { return m_content_length; }
    
    void Http2PushPromise(const char * path);
    BOOL IsHttp2();
private:
    void ParseMethod(const string & strtext);
protected:
	SSL* m_ssl;
	CHttp2 * m_http2;
    uint_32 m_http2_stream_ind;
	int m_sockfd;
	linesock* m_lsockfd;
	linessl * m_lssl;
	string m_line_text;
	
	X509* m_client_cert;
	vector<stExtension>* m_ext_list;
	Http_Method m_http_method;
	string m_resource;
	string m_uri;
	string m_querystring;
	string m_postdata;
	
	fbuffer* m_postdata_ex; /* huge size post data */
	formdata* m_formdata; /* For upload files */
	
	unsigned int m_content_length;

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
    string m_fpm_socktype;
    string m_fpm_sockfile;
	string m_fpm_addr;
	unsigned short m_fpm_port;
    string m_phpcgi_path;
    string m_private_path;
   
    map<string, cgi_cfg_t>* m_cgi_list;
    
    CHttpRequestHdr m_request_hdr;
	
	BOOL m_passed_wwwauth;
	AUTH_SCHEME m_wwwauth_scheme;
    
    BOOL m_keep_alive;
    BOOL m_enabled_keep_alive;
    WebSocket_HandShake m_web_socket_handshake;
    ServiceObjMap* m_srvobj;
    
    map<string, string> m_set_cookies;

    void* m_extdata;
};

#endif /* _HTTP_H_ */

