/*
	Copyright (c) openheap, uplusware
	uplusware@gmail.com
*/
#include <stdio.h>
#include <string.h>
#include "niuauth.h"

///////////////////////////////////////////////////////////////////////////
//TODO: DEMO
bool niuhttpd_usrdef_get_password(const char* username, string& password)
{
    if(strcasecmp(username, "admin") == 0)
    {
        password = "Passw0rd";
        return true;
    }
    else
    {
        return false;
    }
}
// End
///////////////////////////////////////////////////////////////////////////