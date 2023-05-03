/*
        Copyright (c) openheap, uplusware
        uplusware@gmail.com
*/

#ifndef _NIUAPI_
#define _NIUAPI_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <map>
#include <string>
using namespace std;

void NIU_URLFORMAT_ENCODE(const unsigned char* src, string& dst);
void NIU_URLFORMAT_DECODE(const unsigned char* src, string& dst);

/*  POST and QUERY STRING vars*/
void NIU_POST_GET_VARS(const char* data, map<string, string>& varmap);

/* Cookie vars*/
void NIU_COOKIE_VARS(const char* data, map<string, string>& varmap);

#endif /* _NIUAPI_ */
