/*
	Copyright (c) openheap, uplusware
	uplusware@gmail.com
*/
#include "httpservervar.h"

class server_var
{
public:
    server_var();
    virtual ~server_var();
private:
    char m_uid[HTTP_SESSION_UID_LEN + 1];
};
