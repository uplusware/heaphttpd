#ifndef _COOKIE_H_
#define _COOKIE_H_

#include <time.h>
#include <fstream>
#include <string>
#include "util/general.h"

using namespace std;

typedef enum {
  COOKIE_CREATE = 0,
  COOKIE_ACCESS,
  COOKIE_KEYVAL,
  COOKIE_OPTION
} CookieParsePhase;

class Cookie {
 public:
  Cookie();
  Cookie(const char* szName,
         const char* szValue,
         int nMaxAge,
         const char* szExpires,
         const char* szPath,
         const char* szDomain,
         BOOL bSecure = FALSE,
         BOOL bHttpOnly = FALSE,
         const char* szVersion = "1");
  Cookie(const char* szSetCookie);
  virtual ~Cookie();

  time_t getCreateTime();
  time_t getAccessTime();
  void setAccessTime(time_t t_access);

  void toString(string& strCookie);
  const char* getName();
  const char* getExpires() { return m_Expires.c_str(); }
  int getMaxAge() { return m_MaxAge; }

 private:
  time_t m_CreateTime;
  time_t m_AccessTime;

  string m_Name;
  string m_Value;
  string m_Version;
  string m_Domain;
  string m_Expires;
  string m_Path;
  int m_MaxAge;
  BOOL m_Secure;
  BOOL m_HttpOnly;
};

#endif /* _COOKIE_H_ */
