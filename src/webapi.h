#ifndef _WEBAPI_H_
#define _WEBAPI_H_

#include "http.h"
#include "httpcomm.h"
#include "util/escape.h"

class doc
{
protected:
	CHttp * m_session;
    string m_work_path;
	
public:
	doc(CHttp* session, const char* work_path)
	{
		m_session = session;
        m_work_path = work_path;
	}
	virtual ~doc(){}

	virtual void Response();
	
protected:
	
	int generate_cookie_content(const char* szName, const char* szValue, const char* szPath, string & strOut)
	{
		strOut = szName;
		strOut += "=";
		strOut += szValue;
		strOut += "; Version=1; Path=";
		strOut += szPath;
		return 0;
	}
	
	void to_safety_querystring(const char* src, string& dst)
	{
		encodeURI((unsigned char *) src, dst);
	}
	
	const char* XMLSaftyString(string& str)
	{
		Replace(str, "&", "&amp;");
		Replace(str, "<", "&lt;");
		Replace(str, ">", "&gt;");
		Replace(str, "'", "&apos;");
		Replace(str, "\"", "&quot;");
		
		return str.c_str();
	}
	
	void ToHTML(string& str)
	{
		Replace(str, "&", "&amp;");
		
		Replace(str, "\r", "");
		Replace(str, "\n", "<BR>");
		Replace(str, " ", "&nbsp;");
		Replace(str, "\t", "&nbsp;&nbsp;&nbsp;&nbsp;");
		
		Replace(str, "<", "&lt;");
		Replace(str, ">", "&gt;");
		Replace(str, "'", "&apos;");
		Replace(str, "\"", "&quot;");
	}
};

#endif /* _WEBAPI_H_ */
