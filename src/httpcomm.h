/*
	Copyright (c) openheap, uplusware
	uplusware@gmail.com
*/

#ifndef _HTTPCOMM_H_
#define _HTTPCOMM_H_
#include <string>
#include <map>
#include "cache.h"
#include "util/general.h"
#include "cookie.h"

using namespace std;

/* 
Please keep this order for "const char* HTTP_METHOD_NAME[] " in http.cpp
"OPTIONS" 
"GET"    
"HEAD"   
"POST"   
"PUT"    
"DELETE" 
"TRACE"  
"CONNECT"
*/
typedef enum 
{
	hmOptions = 0,
	hmGet,
	hmHead,
	hmPost,
	hmPut,
	hmDelete,
	hmTrace,
	hmConnect
}Http_Method;

/*
HTTP Request Header:
    Accept
    Accept-Charset
    Accept-Encoding
    Accept-Language
    Accept-Datetime
    Authorization
    Cache-Control
    Connection
    Cookie
    Content-Length
    Content-MD5
    Content-Type
    Date
    Expect
    From
    Host
    If-Match
    If-Modified-Since
    If-None-Match
    If-Range
    If-Unmodified-Since
    Max-Forwards
    Origin
    Pragma
    Proxy-Authorization
    Range
    Referer
    TE
    User-Agent
    Upgrade
    Via
    Warning
*/
typedef enum
{
	rqhNone = 0,
	rqhAccept,
	rqhAccept_Charset,
	rqhAccept_Encoding,
	rqhAccept_Language,
	rqhAccept_Datetime,
	rqhAuthorization,
	rqhCache_Control,
	rqhConnection,
	rqhCookie,
	rqhContent_Length,
	rqhContent_MD5,
	rqhContent_Type,
	rqhDate,
	rqhExpect,
	rqhFrom,
	rqhHost,
	rqhIf_Match,
	rqhIf_Modified_Since,
	rqhIf_None_Match,
	rqhIf_Range,
	rqhIf_Unmodified_Since,
	rqhMax_Forwards,
	rqhOrigin,
	rqhPragma,
	rqhProxy_Authorization,
	rqhRange,
	rqhReferer,
	rqhTE,
	rqhUser_Agent,
	rqhUpgrade,
	rqhVia,
	rqhWarning,
	rqhTop
}Http_Request_Header_Field;

class CHttpRequestHdr
{
public:
    CHttpRequestHdr();
    virtual ~CHttpRequestHdr();
    void SetField(const char* name, const char * value);
    
    const char* GetField(const char* name);
    
    void GetField(const char* name, int &val);
    
    void SetMethod(Http_Method method);
    
    Http_Method GetMethod();
    
    
    
private:
	map<string, string> m_mapHeader;
    Http_Method m_method;
};

enum Http_StatsCode
{
    SCBEG = 0,
	SC100 = SCBEG,
    SC101,
    SC102,
    SC200,
    SC201,
    SC202,
    SC203,
    SC204,
    SC205,
    SC206,
    SC207,
    SC300,
    SC301,
    SC302,
    SC303,
    SC304,
    SC305,
    SC306,
    SC307,
    SC400,
    SC401,
    SC402,
    SC403,
    SC404,
    SC405,
    SC406,
    SC407,
    SC408,
    SC409,
    SC410,
    SC411,
    SC412,
    SC413,
    SC414,
    SC415,
    SC416,
    SC417,
    SC422,
    SC423,
    SC424,
    SC500,
    SC501,
    SC502,
    SC503,
    SC504,
    SC505,
    SC507,
    SCMAX
};

static const char* HttpStatusText[][2] = {
    {"100", "Continue"},
    {"101", "Switching Protocols"},
    {"102", "Processing"},
    {"200", "OK"},
    {"201", "Created"},
    {"202", "Accepted"},
    {"203", "Non-Authoriative Information"},
    {"204", "No Content"},
    {"205", "Reset Content"},
    {"206", "Partial Content"},
    {"207", "Multi-Status"},
    {"300", "Multiple Choices"},
    {"301", "Moved Permanently"},
    {"302", "Found"},
    {"303", "See Other"},
    {"304", "Not Modified"},
    {"305", "Use Proxy"},
    {"306", "(Unused)"},
    {"307", "Temporary Redirect"},
    {"400", "Bad Request"},
    {"401", "Unauthorized"},
    {"402", "Payment Granted"},
    {"403", "Forbidden"},
    {"404", "File Not Found"},
    {"405", "Method Not Allowed"},
    {"406", "Not Acceptable"},
    {"407", "Proxy Authentication Required"},
    {"408", "Request Time-out"},
    {"409", "Conflict"},
    {"410", "Gone"},
    {"411", "Length Required"},
    {"412", "Precondition Failed"},
    {"413", "Request Entity Too Large"},
    {"414", "Request-URI Too Large"},
    {"415", "Unsupported Media Type"},
    {"416", "Requested range not satisfiable"},
    {"417", "Expectation Failed"},
    {"422", "Unprocessable Entity"},
    {"423", "Locked"},
    {"424", "Failed Dependency"},
    {"500", "Internal Server Error"},
    {"501", "Not Implemented"},
    {"502", "Bad Gateway"},
    {"503", "Service Unavailable"},
    {"504", "Gateway Timeout"},
    {"505", "HTTP Version Not Supported"},
    {"507", "Insufficient Storage"},
    {NULL,  NULL}
};

typedef enum
{
	rshNone = 0,
	rshAge,
	rshAllow,
	rshServer,
	rshContent_Encoding,
	rshContent_Language,
	rshContent_Range,
	rshContent_Location,
	rshContent_MD5,
	rshAccept_Ranges,
	rshDate,
	rshLast_Modified,
	rshWWW_Authenticate,
	rshProxy_Authenticate,
	rshTransfer_Encoding,
	rshETag,
	rshPragma,
	rshCache_Control,
	rshExpires,
	rshLocation,
	rshContent_Type,
	rshRetry_After,
	rshContent_Length,
	rshConnection,
	rshSet_Cookie,
	rshTop
}Http_Response_Header_Element;

class CHttpResponseHdr
{
private:
	map<string, string> m_mapHeader;
    string m_strHeader;
	Http_StatsCode m_StatusCode;
	bool m_isHdrUpdated;
	string m_strUserDefined;
	string m_strDefaultHTML;
	void _update_header_();
public:
    CHttpResponseHdr();
    CHttpResponseHdr(map<string, string>* map_header);
	virtual ~CHttpResponseHdr();
	void SetStatusCode(Http_StatsCode StatusCode);
    void SetStatusCode(const char* StrStatusCode);
	
	Http_StatsCode GetStatusCode();
	const char* GetDefaultHTML();
    int GetDefaultHTMLLength();
	void SetField(const char* name, const char * value);
    
    void SetField(const char* name, long long value);
    
	void SetFields(const char* fields);

	const char* Text();
	
	unsigned int Length();
    
    map<string, string>* GetMap() { return &m_mapHeader; }
};
    
typedef enum
{
	application_x_www_form_urlencoded = 0,
	multipart_form_data
} Post_Content_Type;


#endif /* _HTTPCOMM_H_ */
