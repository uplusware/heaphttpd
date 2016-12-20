/*
	Copyright (c) openheap, uplusware
	uplusware@gmail.com
*/
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
#define gettid() syscall(__NR_gettid) 
#include "scgi.h"
#include "util/general.h"

SimpleCGI::SimpleCGI(const char* ipaddr, unsigned short port)
    : cgi_base(ipaddr, port)
{
    
}

SimpleCGI::SimpleCGI(const char* sock_file)
    : cgi_base(sock_file)
{
}

SimpleCGI::~SimpleCGI()
{
    
}

int SimpleCGI::SendRequest(map<string, string> &params_map, const char* postdata, unsigned int postdata_len)
{
    vector<char> scgi_send_data;
    const char* pc = "CONTENT_LENGTH";
    for(int x = 0; x <= strlen(pc); x++)
    {
        scgi_send_data.push_back(pc[x]);
    }
    
    pc = params_map["CONTENT_LENGTH"].c_str();
    
    for(int x = 0; x <= strlen(pc); x++)
    {
        scgi_send_data.push_back(pc[x]);
    }
    
    pc = "SCGI";
    for(int x = 0; x <= strlen(pc); x++)
    {
        scgi_send_data.push_back(pc[x]);
    }
    
    pc = "1";
    for(int x = 0; x <= strlen(pc); x++)
    {
        scgi_send_data.push_back(pc[x]);
    }
    map<string, string>::iterator it;
    for(it = params_map.begin(); it != params_map.end(); ++it)
    {
        if(it->first != "CONTENT_LENGTH")
        {
            pc = it->first.c_str();
            for(int x = 0; x <= strlen(pc); x++)
            {
                scgi_send_data.push_back(pc[x]);
            }
            
            pc = it->second.c_str();
            for(int x = 0; x <= strlen(pc); x++)
            {
                scgi_send_data.push_back(pc[x]);
            }
        }
    }
    pc = ",";
    for(int x = 0; x < strlen(pc); x++) //without <00>
    {
        scgi_send_data.push_back(pc[x]);
    }
    
    for(int x = 0; x < postdata_len; x++)
    {
        scgi_send_data.push_back(postdata[x]);
    }
    
    char szlen[64];
    sprintf(szlen, "%u:", scgi_send_data.size());
    Send(szlen, strlen(szlen));
    return Send(&scgi_send_data[0], scgi_send_data.size());
}

int SimpleCGI::RecvResponse(vector<char> &appout)
{
    char buf[1025];
    while(1)
    {
        int r = Recv(buf, 1024);
        if(r < 0)
            break;
        
        for(int x = 0; x < r; x++)
        {
            appout.push_back(buf[x]);
        }
    }
    return 0;
}
