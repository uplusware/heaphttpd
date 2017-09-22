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
#include "cache.h"
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
    http_chunk(int client_sockfd, int backend_sockfd);
    virtual ~http_chunk();
    bool parse(const char* text);
    bool processing(const char* buf, int buf_len, int& next_recv_len);

protected:
    int m_client_sockfd;
    int m_backend_sockfd;
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
    http_client(int client_sockfd, int backend_sockfd, memory_cache* cache, const char* http_url);
    virtual ~http_client();
    
    bool parse(const char* text);
    
    bool processing(const char* buf, int buf_len, int& next_recv_len);
    
    int get_content_length() { return m_content_length; }
    
    bool is_chunked() { return m_is_chunked; }
    bool has_content_length() { return m_has_content_length; }
    
protected:
    string m_line_text;
    bool m_has_content_length;
    int m_content_length;
    bool m_is_200_ok;
    bool m_use_cache;
    int m_cache_max_age;
    string m_cache_control;
    string m_content_type;
    string m_allow;
    string m_encoding;
    string m_language;
    string m_etag;
    string m_last_modified;
    string m_expires;
    string m_server;
    
    bool m_is_chunked;
    
    int m_chunk_length;
    
    HTTP_Client_Parse_State m_state;
    
    char* m_buf;
    int m_buf_len;
    int m_buf_used_len;
    
    int m_client_sockfd;
    int m_backend_sockfd;
    
    int m_sent_content;
    

    http_chunk * m_chunk;
    string m_http_tunneling_url;
    memory_cache* m_cache;
    
    char* m_cache_buf;
    unsigned int m_cache_data_len;
};
#endif /* _HTTP_CLIENT_H_ */