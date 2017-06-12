#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "httpcomm.h"

void _gen_http_date_string_(unsigned int nTime, string & strTime)
{
	char buf[128];
	time_t clock = nTime;
	struct tm *tm;
	tm = localtime(&clock);

	strftime(buf, sizeof(buf), "%a, %d %h %Y %H:%M:%S %Z", tm);

	strTime = buf;

}

//CHttpRequestHdr
CHttpRequestHdr::CHttpRequestHdr()
{
	m_mapHeader.clear();
}
CHttpRequestHdr::~CHttpRequestHdr()
{
	
}
void CHttpRequestHdr::SetField(const char* name, const char * value)
{
	m_mapHeader[name] = value;
}

const char* CHttpRequestHdr::GetField(const char* name)
{
	return m_mapHeader[name].c_str();
}

void CHttpRequestHdr::GetField(const char* name, int &val)
{
	val = atoi(m_mapHeader[name].c_str());
}

void CHttpRequestHdr::SetMethod(Http_Method method)
{
	m_method = method;
}

Http_Method CHttpRequestHdr::GetMethod()
{
	return m_method;
}

//CHttpResponseHdr
CHttpResponseHdr::CHttpResponseHdr()
{
	m_isHdrUpdated = true;
	
	m_mapHeader.clear();
	m_mapHeader.insert(map<string, string>::value_type("Accept-Ranges", ""));
	m_mapHeader.insert(map<string, string>::value_type("Access-Control-Allow-Origin", ""));
	m_mapHeader.insert(map<string, string>::value_type("Accept-Patch", ""));
	
	m_mapHeader.insert(map<string, string>::value_type("Age", ""));
	m_mapHeader.insert(map<string, string>::value_type("Allow", ""));

	m_mapHeader.insert(map<string, string>::value_type("Cache-Control", ""));
	m_mapHeader.insert(map<string, string>::value_type("Connection", ""));
	m_mapHeader.insert(map<string, string>::value_type("Content-Disposition", ""));
	m_mapHeader.insert(map<string, string>::value_type("Content-Encoding", ""));
	m_mapHeader.insert(map<string, string>::value_type("Content-Language", ""));
	m_mapHeader.insert(map<string, string>::value_type("Content-Length", ""));
	m_mapHeader.insert(map<string, string>::value_type("Content-Location", ""));
	m_mapHeader.insert(map<string, string>::value_type("Content-MD5", ""));
	m_mapHeader.insert(map<string, string>::value_type("Content-Range", ""));
	m_mapHeader.insert(map<string, string>::value_type("Content-Type", ""));
	
	string strDate;
	_gen_http_date_string_(time(NULL), strDate);
	m_mapHeader.insert(map<string, string>::value_type("Date", strDate));
	
	m_mapHeader.insert(map<string, string>::value_type("ETag", ""));
	m_mapHeader.insert(map<string, string>::value_type("Expires", ""));
			
	m_mapHeader.insert(map<string, string>::value_type("Last-Modified", ""));
	m_mapHeader.insert(map<string, string>::value_type("Link", ""));
	m_mapHeader.insert(map<string, string>::value_type("Location", ""));
	
	m_mapHeader.insert(map<string, string>::value_type("P3P", ""));
	m_mapHeader.insert(map<string, string>::value_type("Pragma", ""));
	m_mapHeader.insert(map<string, string>::value_type("Proxy-Authenticate", ""));
	m_mapHeader.insert(map<string, string>::value_type("Public-Key-Pins", ""));
	
	m_mapHeader.insert(map<string, string>::value_type("Refresh", ""));
	m_mapHeader.insert(map<string, string>::value_type("Retry-After", ""));
	
	m_mapHeader.insert(map<string, string>::value_type("Server", "heaphttpd/1.0"));
	m_mapHeader.insert(map<string, string>::value_type("Set-Cookie", ""));
	m_mapHeader.insert(map<string, string>::value_type("Strict-Transport-Security", ""));
	
	m_mapHeader.insert(map<string, string>::value_type("Trailer", ""));
	m_mapHeader.insert(map<string, string>::value_type("Transfer-Encoding", ""));
	
	m_mapHeader.insert(map<string, string>::value_type("Upgrade", ""));
	m_mapHeader.insert(map<string, string>::value_type("Vary", ""));
	m_mapHeader.insert(map<string, string>::value_type("Via", ""));
	m_mapHeader.insert(map<string, string>::value_type("Warning", ""));
	m_mapHeader.insert(map<string, string>::value_type("WWW-Authenticate", ""));
	m_mapHeader.insert(map<string, string>::value_type("X-Frame-Options", ""));
	m_mapHeader.insert(map<string, string>::value_type("X-Powered-By", ""));
}

void CHttpResponseHdr::SetStatusCode(Http_StatsCode StatusCode)
{
	if(StatusCode == SC200)
	{
		m_mapHeader["Accept-Ranges"] = "bytes";
	}
	m_StatusCode = StatusCode;
    
    string strDate;
    _gen_http_date_string_(time(NULL), strDate);
    m_strDefaultHTML = "<HTML><HEAD><TITLE>";
    m_strDefaultHTML += HttpStatusText[m_StatusCode][0];
    m_strDefaultHTML += "-";
    m_strDefaultHTML += HttpStatusText[m_StatusCode][1];
    m_strDefaultHTML += "</TITLE></HEAD><BODY><H1>";
    m_strDefaultHTML += HttpStatusText[m_StatusCode][0];
    m_strDefaultHTML += " - ";
    m_strDefaultHTML += HttpStatusText[m_StatusCode][1];
    m_strDefaultHTML += "</H1><HR>";
    m_strDefaultHTML += strDate;
    if(StatusCode == SC301 && m_mapHeader["Location"] != "")
    {
        m_strDefaultHTML += "<p>";
        m_strDefaultHTML += "Moved to <a href=\"";
        m_strDefaultHTML += m_mapHeader["Location"];
        m_strDefaultHTML += "\">";
        m_strDefaultHTML += m_mapHeader["Location"];
        m_strDefaultHTML += "</a>";
        m_strDefaultHTML += "</p>";
        /* JavaScript */
        m_strDefaultHTML += "<script language=\"JavaScript\"> ";
        m_strDefaultHTML += "window.location.href = \"";
        m_strDefaultHTML += m_mapHeader["Location"];
        m_strDefaultHTML += "\";";
        m_strDefaultHTML += " </script>";
    }
    m_strDefaultHTML += "<p><i>This default page is generated by heaphttpd.</i></p></BODY></HTML>";
}

void CHttpResponseHdr::SetStatusCode(const char* StrStatusCode) /*200*/
{
    Http_StatsCode StatusCode = SC200;
    
    for(int x = SCBEG; x < SCMAX; x++)
    {
        if(strncmp(HttpStatusText[x][0], StrStatusCode, 3) == 0)
        {
            StatusCode = (Http_StatsCode)x;
            break;
        }
    }
    SetStatusCode(StatusCode);
}

Http_StatsCode CHttpResponseHdr::GetStatusCode()
{
	return m_StatusCode;
}

const char* CHttpResponseHdr::GetDefaultHTML()
{
    return m_strDefaultHTML.c_str();
}

int CHttpResponseHdr::GetDefaultHTMLLength()
{
    return m_strDefaultHTML.length();
}
    
void CHttpResponseHdr::SetField(const char* name, const char * value)
{
	m_mapHeader[name] = value;
	m_isHdrUpdated = true;
    if(strcasecmp(name, "Location") == 0)
    {
        SetStatusCode(m_StatusCode);
    }
}

void CHttpResponseHdr::SetField(const char* name, long long value)
{
	char szVal[128];
	sprintf(szVal, "%lld", value);
	m_mapHeader[name] = szVal;
	m_isHdrUpdated = true;
}

void CHttpResponseHdr::SetFields(const char* fields)
{
	m_strUserDefined += fields;
	m_isHdrUpdated = true;
}

CHttpResponseHdr::~CHttpResponseHdr()
{
	
}
   
const char*CHttpResponseHdr::Text()
{
	_update_header_();
	//printf("%s\n", m_strHeader.c_str());
	return m_strHeader.c_str();
}

unsigned int CHttpResponseHdr::Length()
{
	_update_header_(); //Check for update
	return m_strHeader.length();
}

void CHttpResponseHdr::_update_header_()
{
	if(m_isHdrUpdated)
	{
		m_strHeader  = "HTTP/1.1 ";
		m_strHeader += HttpStatusText[m_StatusCode][0];
		m_strHeader += " ";
		m_strHeader += HttpStatusText[m_StatusCode][1];
		m_strHeader += "\r\n";
		
		map<string, string>::iterator it;
		for(it = m_mapHeader.begin(); it != m_mapHeader.end(); ++it)
		{
			if(it->second != "")
			{
				m_strHeader += it->first;
				m_strHeader += ": ";
				m_strHeader += it->second;
				m_strHeader += "\r\n";
			}
		}
		m_strHeader += m_strUserDefined;

		m_isHdrUpdated = false;
	}
}
