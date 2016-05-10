/*
	Copyright (c) openheap, uplusware
	uplusware@gmail.com
*/
#ifndef _HTTP_SESSION_VAR_H_
#define _HTTP_SESSION_VAR_H_

#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <string>
using namespace std;
#include "util/general.h"
#include "util/md5.h"

typedef std::pair<std::string, std::string> session_var_key;

#define HTTP_SESSION_UID_LEN 32

typedef enum
{
    SESSION_VAR_UID = 0,
    SESSION_VAR_CREATE,
    SESSION_VAR_ACCESS,
    SESSION_VAR_KEYVAL,
    SESSION_VAR_OPTION
} SessionVarParsePhase;

class session_var
{
public:
    session_var(const char* uid, const char* name, const char* value);
    session_var(const char* szline);
    virtual ~session_var();
    
    time_t getCreateTime();
    time_t getAccessTime();
    void   setAccessTime(time_t t);
    const char* getName();
    const char* getValue();
    const char* getUID();
private:
    char m_uid[HTTP_SESSION_UID_LEN + 1];
    time_t m_access;
    time_t m_create;
    string m_name;
    string m_value;
};

#endif /* _HTTP_SESSION_VAR_H_ */
