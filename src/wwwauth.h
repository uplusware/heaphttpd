/*
	Copyright (c) openheap, uplusware
	uplusware@gmail.com
*/
#ifndef _WWW_AUTH_
#define _WWW_AUTH_

#include <stdio.h>
#include <string>
#include <map>

class CHttp;

using namespace std;

typedef enum
{
	asNone = 0,
	asBasic,
	asDigest
} AUTH_SCHEME;

bool WWW_Auth(CHttp* psession, AUTH_SCHEME scheme, const char* authinfo, string& username, string &keywords, const char* method = "GET");

#endif /* _WWW_AUTH_ */
