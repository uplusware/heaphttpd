/*
	Copyright (c) openheap, uplusware
	uplusware@gmail.com
*/

#ifndef _AUTH_H_
#define _AUTH_H_

#include <string>
using namespace std;

extern "C" bool heaphttpd_usrdef_get_password(const char* username, string& password);

#endif /* _AUTH_H_ */
