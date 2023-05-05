/*
        Copyright (c) openheap, uplusware
        uplusware@gmail.com
*/
#ifndef _WWW_AUTH_
#define _WWW_AUTH_

#include <stdio.h>
#include <map>
#include <string>

class CHttp;

using namespace std;

typedef enum { asNone = 0, asBasic, asDigest } AUTH_SCHEME;

bool WWW_Auth(CHttp* psession,
              AUTH_SCHEME scheme,
              bool integrate_local_users,
              const char* authinfo,
              string& username,
              string& keywords,
              const char* method = "GET");

#endif /* _WWW_AUTH_ */
