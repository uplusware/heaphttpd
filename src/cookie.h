#ifndef _COOKIE_H_
#define _COOKIE_H_

#include <time.h>
#include <string>
#include "util/general.h"

using namespace std;

class Cookie
{
public:
    Cookie();
    Cookie(const char* szName, const char* szValue,
        int nMaxAge, const char* szExpires, const char* szPath, const char* szDomain, 
        BOOL bSecure, BOOL bHttpOnly);
    virtual ~Cookie();
    
    time_t getCreateTime();
    void toString(string & strCookie);
    const char* getName();
private:
    time_t m_CreateTime;
    
    string m_Name;
    string m_Value;
    string m_Domain;
    string m_Expires;
    string m_Path;
    int m_MaxAge;
    BOOL m_Secure;
    BOOL m_HttpOnly;
};

#endif /* _COOKIE_H_ */
