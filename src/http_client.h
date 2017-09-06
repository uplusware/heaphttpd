/*
	Copyright (c) openheap, uplusware
	uplusware@gmail.com
*/

#ifndef _HTTP_CLIENT_H_
#define _HTTP_CLIENT_H_

#include <sys/epoll.h>  
#include <arpa/inet.h>
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <netdb.h>

#include "base.h"
#include "util/general.h"

#include <arpa/inet.h>
 
using namespace std;
class http_client
{
public:
    http_client();
    virtual ~http_client();
    
    bool parse(const char* text);
    
    int get_content_length() { return m_content_length; }
    
protected:
    string m_line_text;
    int m_content_length;
};
#endif /* _HTTP_CLIENT_H_ */