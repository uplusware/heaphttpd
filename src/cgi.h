/*
	Copyright (c) openheap, uplusware
	uplusware@gmail.com
*/

#ifndef _CGI_H_
#define _CGI_H_

#include <unistd.h>
#include <fcntl.h>
#include <stdio.h> 
#include <sys/types.h> 
#include <netdb.h>
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <arpa/inet.h>
#include <sys/un.h>
#include <sys/syscall.h>
#include <string.h>
#include <string>
#include <vector>
#include <map>

using namespace std;

#include "util/general.h"

#define gettid() syscall(__NR_gettid)


typedef enum {
    INET_SOCK = 0,
    UNIX_SOCK
}CGI_SOCK;

class cgi_base
{
public:
	cgi_base(const char* ipaddr, unsigned short port);
	cgi_base(const char* sock_file);
    cgi_base(int sockfd);
	virtual ~cgi_base();
	
	virtual int Connect();
    virtual int Close();
	virtual int Send(const char* buf, unsigned long long len);
	virtual int Recv(const char* buf, unsigned long long len);
	
private:
	string m_strIP;
	unsigned short m_nPort;
	string m_strSockfile;
    CGI_SOCK m_sockType;
protected:
   	int m_sockfd;
};


class forkcgi : public cgi_base
{
public:
	forkcgi(int sockfd);
	virtual ~forkcgi();
    
    int SendData(const char* data, int len);
    int ReadAppData(vector<char>& binaryResponse, BOOL& continue_recv);
};

#endif /* _CGI_H_ */