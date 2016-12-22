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
    //MUST have CONTENT_LENGTH even if the value is 0
    const char* p_temp = "CONTENT_LENGTH";
    for(int x = 0; x <= strlen(p_temp); x++)
    {
        scgi_send_data.push_back(p_temp[x]);
    }
    
    char szdatalen[64];
    sprintf(szdatalen, "%u", postdata_len);
    p_temp = szdatalen;
    
    for(int x = 0; x <= strlen(p_temp); x++)
    {
        scgi_send_data.push_back(p_temp[x]);
    }
    
    //MUST have SCGI and value is 1
    p_temp = "SCGI";
    for(int x = 0; x <= strlen(p_temp); x++)
    {
        scgi_send_data.push_back(p_temp[x]);
    }
    
    p_temp = "1";
    for(int x = 0; x <= strlen(p_temp); x++)
    {
        scgi_send_data.push_back(p_temp[x]);
    }
    
    //Other paramters except of CONTENT_LENGTH
    map<string, string>::iterator it;
    for(it = params_map.begin(); it != params_map.end(); ++it)
    {
        if(it->first != "CONTENT_LENGTH")
        {
            if(it->second != "")
            {
                p_temp = it->first.c_str();
                for(int x = 0; x <= strlen(p_temp); x++)
                {
                    scgi_send_data.push_back(p_temp[x]);
                }
                
                p_temp = it->second.c_str();
                for(int x = 0; x <= strlen(p_temp); x++)
                {
                    scgi_send_data.push_back(p_temp[x]);
                }
            }
        }
    }
    
    char szlen[64];
    sprintf(szlen, "%u:", scgi_send_data.size());
    
    p_temp = ",";
    for(int x = 0; x < strlen(p_temp); x++) //without <00>
    {
        scgi_send_data.push_back(p_temp[x]);
    }
    
    for(int x = 0; x < postdata_len; x++)
    {
        scgi_send_data.push_back(postdata[x]);
    }
    
    Send(szlen, strlen(szlen));
    return Send(&scgi_send_data[0], scgi_send_data.size());
}

int SimpleCGI::RecvAppData(vector<char> &binaryResponse, BOOL& continue_recv)
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
            binaryResponse.push_back(buf[x]);
        }
        if(binaryResponse.size() >= 1024*64) /* 64k */
        {
            continue_recv = TRUE;
            break;
        }
    }
    return binaryResponse.size();
}
