#include "cookie.h"

Cookie::Cookie()
{
    m_MaxAge = -1;
    m_Secure = FALSE;
    m_HttpOnly = FALSE;
}

Cookie::Cookie(const char* szName, const char* szValue,
    int nMaxAge, const char* szExpires, const char* szPath, const char* szDomain, 
    BOOL bSecure, BOOL bHttpOnly)
{
    m_Name = szName;
    m_Value = szValue;
    m_Expires = szExpires;
    m_Path = szPath;
    m_Domain = szDomain;
    m_MaxAge = nMaxAge;
    m_Secure = bSecure;
    m_HttpOnly = bHttpOnly;
    
    m_CreateTime = time(NULL);
}

Cookie::~Cookie()
{

}

time_t Cookie::getCreateTime()
{
    return m_CreateTime;
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
	
	if(m_MaxAge != -1)
	{
	    char szMaxAge[64];
	    sprintf(szMaxAge, "%s", m_MaxAge);
	    strCookie += " MaxAge=";
        strCookie += szMaxAge;
        strCookie += ";";
	}
	
	if(m_Expires[0] == '\0')
	{
	    strCookie += " Expires=";
	    strCookie += m_Expires;
	    strCookie += ";";
	}
	
	if(m_Path[0] == '\0')
	{
	    strCookie += " Path=";
	    strCookie += m_Path;
	    strCookie += ";";
	}
	
	if(m_Domain[0] == '\0')
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
