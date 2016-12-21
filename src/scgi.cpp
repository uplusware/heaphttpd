/*
	Copyright (c) openheap, uplusware
	uplusware@gmail.com
*/
#include "scgi.h"

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

int SimpleCGI::SendParamsAndData(map<string, string> &params_map, const char* postdata, unsigned int postdata_len)
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
            if(it->second != "")
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
    }
    
    char szlen[64];
    sprintf(szlen, "%u:", scgi_send_data.size());
    
    pc = ",";
    for(int x = 0; x < strlen(pc); x++) //without <00>
    {
        scgi_send_data.push_back(pc[x]);
    }
    
    for(int x = 0; x < postdata_len; x++)
    {
        scgi_send_data.push_back(postdata[x]);
    }
    
    Send(szlen, strlen(szlen));
    return Send(&scgi_send_data[0], scgi_send_data.size());
}

int SimpleCGI::RecvAppData(vector<char> &appout, BOOL& continue_recv)
{
    continue_recv = FALSE;
    char buf[1501];
    while(1)
    {
        int rlen = Recv(buf, 1500);
        if(rlen <= 0)
            break;
        
        for(int x = 0; x < rlen; x++)
        {        
            appout.push_back(buf[x]);
        }
        if(appout.size() >= 1024*64) /* 64k */
        {
            continue_recv = TRUE;
            break;
        }
    }
    return appout.size();
}
