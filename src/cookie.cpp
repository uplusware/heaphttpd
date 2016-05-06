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
    m_Name = szName == NULL ? "" : szName;
    m_Value = szValue == NULL ? "" : szValue;
    m_Expires = szExpires == NULL ? "" : szExpires;
    m_Path = szPath == NULL ? "" : szPath;
    m_Domain = szDomain == NULL ? "" : szDomain;
    m_MaxAge = nMaxAge;
    m_Secure = bSecure;
    m_HttpOnly = bHttpOnly;
    
    m_CreateTime = time(NULL);
}

Cookie::Cookie(const char* szSetCookie)
{
    m_HttpOnly = FALSE;
    m_Secure = FALSE;
    m_MaxAge = -1;
    
    vector<string> vCookie;
    vSplitString(szSetCookie, vCookie, ";", TRUE);
    vector<string>::iterator iter_c;
    
    bool isNameValue = false;
    for(iter_c = vCookie.begin(); iter_c != vCookie.end(); iter_c++)
    {
        string strField(*iter_c);
        strtrim(strField);
        
        if(iter_c == vCookie.begin())
        {
            m_CreateTime = atoi(strField.c_str());
            isNameValue = true;
        }
        else if(isNameValue)
        {
            strcut(strField.c_str(), NULL, "=", m_Name);
            strcut(strField.c_str(), "=", NULL, m_Value);
            strtrim(m_Name);
            strtrim(m_Value);
            isNameValue = false;
        }
        else
        {
            if(strncasecmp(strField.c_str(), "Expires", strlen("Expires")) == 0)
		    {
			    strcut(strField.c_str(), "=", NULL, m_Expires);
			    strtrim(m_Expires);
		    }
		    else if(strncasecmp(strField.c_str(), "Path", strlen("Path")) == 0)
		    {
			    strcut(strField.c_str(), "=", NULL, m_Path);
			    strtrim(m_Path);
		    }
		    else if(strncasecmp(strField.c_str(), "Domain", strlen("Domain")) == 0)
		    {
			    strcut(strField.c_str(), "=", NULL, m_Domain);
			    strtrim(m_Domain);
		    }
		    else if(strncasecmp(strField.c_str(), "MaxAge", strlen("MaxAge")) == 0)
		    {
		        string strMaxAge;
			    strcut(strField.c_str(), "=", NULL, strMaxAge);
			    strtrim(strMaxAge);
			    m_MaxAge = atoi(strMaxAge.c_str());
		    }
		    else if(strncasecmp(strField.c_str(), "Secure", strlen("Secure")) == 0)
		    {
		        m_Secure = TRUE;
		    }
		    else if(strncasecmp(strField.c_str(), "HttpOnly", strlen("HttpOnly")) == 0)
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
