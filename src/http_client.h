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

#define HTTP_CLIENT_BUF_LEN 4096

using namespace std;

enum HTTP_Client_Parse_State
{
    HTTP_Client_Parse_State_Header = 0,
    HTTP_Client_Parse_State_Content,
    HTTP_Client_Parse_State_Chunk_Header,
    HTTP_Client_Parse_State_Chunk_Content
};

class http_chunk
{
public:
    http_chunk(int sockfd);
    virtual ~http_chunk();
    bool parse(const char* text);
    bool processing(const char* buf, int buf_len, int& next_recv_len);

protected:
    int m_sockfd;
    int m_chunk_len;
    int m_sent_chunk;
    HTTP_Client_Parse_State m_state;
    
    char* m_buf;
    int m_buf_len;
    int m_buf_used_len;
    
    string m_line_text;
    
};

class http_client
{
public:
    http_client(int sockfd);
    virtual ~http_client();
    
    bool parse(const char* text);
    
    bool processing(const char* buf, int buf_len, int& next_recv_len);
    
    int get_content_length() { return m_content_length; }
    
    bool is_chunked() { return m_is_chunked; }
    bool has_content_length() { return m_has_content_length; }
    
protected:
    string m_line_text;
    int m_content_length;
    bool m_is_chunked;
    bool m_has_content_length;
    int m_chunk_length;
    
    HTTP_Client_Parse_State m_state;
    
    char* m_buf;
    int m_buf_len;
    int m_buf_used_len;
    
    int m_sockfd;
    
    int m_sent_content;
    
    http_chunk * m_chunk;
};
#endif /* _HTTP_CLIENT_H_ */