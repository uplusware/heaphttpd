#include "cookie.h"

Cookie::Cookie()
{
    m_MaxAge = -1;
    m_Secure = FALSE;
    m_HttpOnly = FALSE;
}

Cookie::Cookie(const char* szName, const char* szValue,
    int nMaxAge, const char* szExpires, const char* szPath, const char* szDomain, 
    BOOL bSecure, BOOL bHttpOnly, const char* szVersion)
{
    m_Name = (szName == NULL ? "" : szName);
    m_Value = (szValue == NULL ? "" : szValue);
    m_Expires = (szExpires == NULL ? "" : szExpires);
    m_Path = (szPath == NULL ? "/" : szPath);
    m_Domain = (szDomain == NULL ? "" : szDomain);
    m_MaxAge = nMaxAge;
    m_Secure = bSecure;
    m_HttpOnly = bHttpOnly;
    m_Version = (szVersion == NULL ? "1" : szVersion);
    m_CreateTime = time(NULL);
    m_AccessTime = time(NULL);
}

Cookie::Cookie(const char* szSetCookie)
{
    m_HttpOnly = FALSE;
    m_Secure = FALSE;
    m_MaxAge = -1;
    m_Path = "/";
    
    vector<string> vCookie;
    vSplitString(szSetCookie, vCookie, ";", TRUE);
    vector<string>::iterator iter_c;
    
    CookieParsePhase phase = COOKIE_CREATE;
    for(iter_c = vCookie.begin(); iter_c != vCookie.end(); iter_c++)
    {
        string strField(*iter_c);
        strtrim(strField);
        
        if(phase == COOKIE_CREATE)
        {
            m_CreateTime = atoi(strField.c_str());
            phase = COOKIE_ACCESS;
        }
        else if(phase == COOKIE_ACCESS)
        {
            m_AccessTime = atoi(strField.c_str());
            phase = COOKIE_KEYVAL;
        }
        else if(phase == COOKIE_KEYVAL)
        {
            strcut(strField.c_str(), NULL, "=", m_Name);
            strcut(strField.c_str(), "=", NULL, m_Value);
            strtrim(m_Name);
            strtrim(m_Value);
            phase = COOKIE_OPTION;
        }
        else
        {
            if(strncasecmp(strField.c_str(), "Version", sizeof("Version") - 1) == 0)
		    {
			    strcut(strField.c_str(), "=", NULL, m_Version);
			    strtrim(m_Version);
		    }
		    else if(strncasecmp(strField.c_str(), "Expires", sizeof("Expires") - 1) == 0)
		    {
			    strcut(strField.c_str(), "=", NULL, m_Expires);
			    strtrim(m_Expires);
		    }
		    else if(strncasecmp(strField.c_str(), "Path", sizeof("Path") - 1) == 0)
		    {
			    strcut(strField.c_str(), "=", NULL, m_Path);
			    strtrim(m_Path);
		    }
		    else if(strncasecmp(strField.c_str(), "Domain", sizeof("Domain") - 1) == 0)
		    {
			    strcut(strField.c_str(), "=", NULL, m_Domain);
			    strtrim(m_Domain);
		    }
		    else if(strncasecmp(strField.c_str(), "MaxAge", sizeof("MaxAge") - 1) == 0)
		    {
		        string strMaxAge;
			    strcut(strField.c_str(), "=", NULL, strMaxAge);
			    strtrim(strMaxAge);
			    m_MaxAge = atoi(strMaxAge.c_str());
		    }
		    else if(strncasecmp(strField.c_str(), "Secure", sizeof("Secure") - 1) == 0)
		    {
		        m_Secure = TRUE;
		    }
		    else if(strncasecmp(strField.c_str(), "HttpOnly", sizeof("HttpOnly") - 1) == 0)
		    {
		        m_HttpOnly = TRUE;
		    }
		}
    }
}

Cookie::~Cookie()
{

}

time_t Cookie::getCreateTime()
{
    return m_CreateTime;
}

time_t Cookie::getAccessTime()
{
    return m_AccessTime;
}

void Cookie::setAccessTime(time_t t_access)
{
    m_AccessTime = t_access;
}

const char* Cookie::getName()
{
    return m_Name.c_str();
}

void Cookie::toString(string & strCookie)
{
    strCookie = m_Name;
	strCookie += "=";
	strCookie += m_Value;
	strCookie += ";";
	if(m_Version != "")
	{
	    strCookie += " Version=";
	    strCookie += m_Version;
	    strCookie += ";";
	}
	if(m_MaxAge != -1)
	{
	    char szMaxAge[64];
	    sprintf(szMaxAge, "%d", m_MaxAge);
	    strCookie += " MaxAge=";
        strCookie += szMaxAge;
        strCookie += ";";
	}
	if(m_Expires[0] != '\0')
	{
	    strCookie += " Expires=";
	    strCookie += m_Expires;
	    strCookie += ";";
	}
	if(m_Path[0] != '\0')
	{
	    strCookie += " Path=";
	    strCookie += m_Path;
	    strCookie += ";";
	}
	if(m_Domain[0] != '\0')
	{
	    strCookie += " Domain=";
	    strCookie += m_Domain;
	    strCookie += ";";
	}
	if(m_Secure)
	{
	    strCookie += " Secure";
	    strCookie += ";";
	}
	if(m_HttpOnly)
	{
	    strCookie += " HttpOnly";
	}
}
