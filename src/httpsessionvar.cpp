/*
	Copyright (c) openheap, uplusware
	uplusware@gmail.com
*/

#include "httpsessionvar.h"

session_var::session_var(const char* name, const char* value, const char* uid)
{
    memcpy(m_uid, uid, HTTP_SESSION_UID_LEN);
    m_uid[HTTP_SESSION_UID_LEN] = '\0';      
}

session_var::~session_var()
{

}
