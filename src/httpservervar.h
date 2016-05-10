/*
	Copyright (c) openheap, uplusware
	uplusware@gmail.com
*/

#ifndef _HTTP_SERVER_H_
#define _HTTP_SERVER_H_

#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <string>
using namespace std;
#include "util/general.h"

typedef enum
{
    SERVER_VAR_CREATE = 0,
    SERVER_VAR_ACCESS,
    SERVER_VAR_KEYVAL,
    SERVER_VAR_OPTION
} ServerVarParsePhase;

class server_var
{
public:
    server_var(const char* name, const char* value);
    server_var(const char* szline);
    virtual ~server_var();
    
    time_t getCreateTime();
    time_t getAccessTime();
    void   setAccessTime(time_t t);
    const char* getName();
    const char* getValue();
private:
    time_t m_access;
    time_t m_create;
    string m_name;
    string m_value;
};

#endif /* _HTTP_SERVER_H_ */
