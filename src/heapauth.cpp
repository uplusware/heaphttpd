/*
	Copyright (c) openheap, uplusware
	uplusware@gmail.com
*/
#include <stdio.h>
#include <string.h>
#include "heapauth.h"
#include "http.h"

///////////////////////////////////////////////////////////////////////////
//TODO: DEMO
bool heaphttpd_usrdef_get_password(CHttp* psession, const char* username, string& password)
{
    return psession->GetCache()->find_user(username, password);
}
// End
///////////////////////////////////////////////////////////////////////////