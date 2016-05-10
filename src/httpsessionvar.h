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

#include "util/md5.h"

#define HTTP_SESSION_UID_LEN 32

class session_var
{
public:
    session_var(const char* name, const char* value, const char* uid);
    virtual ~session_var();
private:
    char m_uid[HTTP_SESSION_UID_LEN + 1];
};

#endif /* _HTTP_SESSION_VAR_H_ */
