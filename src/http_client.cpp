/*
	Copyright (c) openheap, uplusware
	uplusware@gmail.com
*/

#include "http_client.h"

//http_chunk
http_chunk::http_chunk(int client_sockfd, int backend_sockfd)
{
    m_client_sockfd = client_sockfd;
    m_backend_sockfd = backend_sockfd;
    m_chunk_len = 0;
    m_sent_chunk = 0;
    m_state = HTTP_Client_Parse_State_Chunk_Header;
    
    m_buf = (char*)malloc(HTTP_CLIENT_BUF_LEN + 1);
    m_buf[HTTP_CLIENT_BUF_LEN] = '\0';
    m_buf_len = HTTP_CLIENT_BUF_LEN;
    m_buf_used_len = 0;
}

http_chunk::~http_chunk()
{
    if(m_buf)
        free(m_buf);
}

bool http_chunk::parse(const char* text)
{
    string strtext;
    m_line_text += text;
    std::size_t new_line;
    if((new_line = m_line_text.find('\n')) != std::string::npos)
    {
        strtext = m_line_text.substr(0, new_line + 1);
        m_line_text = m_line_text.substr(new_line + 1);

        strtrim(strtext);
        m_chunk_len = atoi(strtext.c_str());
		if(m_chunk_len < 0)
			return false;
		m_chunk_len += 2; /* \r\n */
    }
	
	return true;
}

bool http_chunk::processing(const char* buf, int buf_len, int& next_recv_len)
{
    if(m_state == HTTP_Client_Parse_State_Chunk_Header)
    {
        next_recv_len = 1;
        
        if((m_buf_len - m_buf_used_len) < buf_len)
        {
            char* new_buf = (char*)malloc(m_buf_used_len + buf_len + 1);
            new_buf[m_buf_used_len + buf_len] = '\0';
            
            memcpy(new_buf, m_buf, m_buf_used_len);
            
            free(m_buf);
            
            m_buf = new_buf;
            m_buf_len = m_buf_used_len + buf_len;
        }
        
        memcpy(m_buf + m_buf_used_len, buf, buf_len);
        m_buf_used_len += buf_len;
        
        if(m_buf_used_len >= 2)
        {
            bool found_endle = false;
            for(int x = 0; x < m_buf_used_len; x++)
            {
                if(memcmp(m_buf + x, "\r\n", 2) == 0)
                {
                    if(m_buf_used_len > 64)
                        return false;
                    
                    if(!parse(m_buf))
						return false;
					
                    if(_Send_(m_client_sockfd, m_buf, x + 2) < 0)
                        return false;
                    
                    memmove(m_buf, m_buf + x + 2, m_buf_used_len - x - 2);
                    m_buf_used_len = m_buf_used_len - x - 2;
                    
                    m_state = HTTP_Client_Parse_State_Chunk_Content;
                    
                    if(m_buf_used_len > 0) /* begin to send chunk data */
                    {
                        if(_Send_(m_client_sockfd, m_buf, m_buf_used_len) < 0)
                            return false;
                        m_sent_chunk += m_buf_used_len;
                        m_buf_used_len = 0;
                        
                        if(m_sent_chunk == m_chunk_len)
                        {
                            return false;
                        }
                        
                    }
                        
                    if(0 == m_chunk_len)
                    {
                        return false;
                    }
                    next_recv_len = m_chunk_len - m_sent_chunk;
                    found_endle = true;
                    break;
                }
            }
        }
    }
    else if(m_state == HTTP_Client_Parse_State_Chunk_Content)
    {
        //Send the left data from the last send.
        if(m_buf_used_len > 0)
        {
            if(_Send_(m_client_sockfd, m_buf, m_buf_used_len) < 0)
                return false;
            m_sent_chunk += m_buf_used_len;
            m_buf_used_len = 0;
            
            if(m_sent_chunk == m_chunk_len)
            {
                m_chunk_len = 0;
                m_sent_chunk = 0;
                m_state = HTTP_Client_Parse_State_Chunk_Header;
            }
        
        }
        else
        {
            int expected_send_len = m_chunk_len - m_sent_chunk;
            
            int should_send_len = buf_len < expected_send_len ? buf_len : expected_send_len;
            
            if(_Send_(m_client_sockfd, buf, should_send_len) < 0)
                    return false;
            
            m_sent_chunk += should_send_len;
            
            if(buf_len > should_send_len)
            {
                
                int left_data_len = buf_len - should_send_len;
                
                if((m_buf_len - m_buf_used_len) < left_data_len)
                {
                    char* new_buf = (char*)malloc(m_buf_used_len + left_data_len + 1);
                    new_buf[m_buf_used_len + left_data_len] = '\0';
                    
                    memcpy(new_buf, m_buf, m_buf_used_len);
                    
                    free(m_buf);
                    
                    m_buf = new_buf;
                    m_buf_len = m_buf_used_len + left_data_len;
                }
                
                memcpy(m_buf, buf + should_send_len, left_data_len);
                m_buf_used_len = left_data_len;
            }
            
            
            if(m_sent_chunk == m_chunk_len)
            {
                m_chunk_len = 0;
                m_sent_chunk = 0;
                m_state = HTTP_Client_Parse_State_Chunk_Header;
            }
        }
    }
}


// http_client
http_client::http_client(int client_sockfd, int backend_sockfd, memory_cache* cache, const char* http_url)
{
    m_client_sockfd = client_sockfd;
    m_backend_sockfd = backend_sockfd;
    
    m_http_tunneling_url = http_url;
    
    m_state = HTTP_Client_Parse_State_Header;
    
    m_content_length = -1;
    m_cache_max_age = 300;
    m_has_content_length = false;
    m_is_chunked = false;
    
    m_buf = (char*)malloc(HTTP_CLIENT_BUF_LEN + 1);
    m_buf[HTTP_CLIENT_BUF_LEN] = '\0';
    m_buf_len = HTTP_CLIENT_BUF_LEN;
    m_buf_used_len = 0;
    
    m_sent_content = 0;

    m_chunk = NULL;
    
    m_cache = cache;
    
    m_use_cache = false; //default is true
    m_cache_buf = NULL;
    m_cache_data_len = 0;
    
    m_is_200_ok = false;
}

http_client::~http_client()
{
    if(CHttpBase::m_enable_http_tunneling_cache && m_use_cache && m_cache_buf && m_cache_data_len > 0)
    {
        tunneling_cache* c_out;
        m_cache->wrlock_tunneling_cache();
        m_cache->_push_tunneling_(m_http_tunneling_url.c_str(), m_cache_buf, m_cache_data_len, m_content_type.c_str(), m_cache_control.c_str(),
            m_last_modified.c_str(), m_etag.c_str(), m_expires.c_str(), m_server.c_str(), m_cache_max_age, &c_out);
        m_cache->unlock_tunneling_cache();
    }

    if(m_cache_buf)
        free(m_cache_buf);
    
    if(m_buf)
        free(m_buf);
    
    if(m_chunk)
        delete m_chunk;
}

bool http_client::parse(const char* text)
{
    string strtext;
    m_line_text += text;
    std::size_t new_line;
    while((new_line = m_line_text.find('\n')) != std::string::npos)
    {
        strtext = m_line_text.substr(0, new_line + 1);
        m_line_text = m_line_text.substr(new_line + 1);

        strtrim(strtext);
        // printf(">>>>> %s\r\n", strtext.c_str());
        BOOL High = TRUE;
        for(int c = 0; c < strtext.length(); c++)
        {
            if(High)
            {
                strtext[c] = HICH(strtext[c]);
                High = FALSE;
            }
            if(strtext[c] == '-')
                High = TRUE;
            if(strtext[c] == ':' || strtext[c] == ' ')
                break;
        }
        if(strcmp(strtext.c_str(),"") == 0)
        {
            if(CHttpBase::m_enable_http_tunneling_cache && m_use_cache && has_content_length() && m_content_length <= CHttpBase::m_single_tunneling_cache_size && m_content_length > 0) //only cache the little size data
            {
                m_cache_buf = (char*)malloc(m_content_length + 1);
                m_cache_data_len = 0;
            }
            break;
        }
        else if(strncasecmp(strtext.c_str(), "HTTP/1.0 ", 9) == 0
            || strncasecmp(strtext.c_str(), "HTTP/1.1 ", 9) == 0)
        {
            if(strstr(strtext.c_str() + 9, "200 OK") != NULL)
            {
                m_is_200_ok = true;
                m_use_cache = true;
            }
        }
        else if(strncasecmp(strtext.c_str(), "Content-Length:", 15) == 0)
        {
            string strLen;
            strcut(strtext.c_str(), "Content-Length:", NULL, strLen);
            strtrim(strLen);	
            m_content_length = atoi(strLen.c_str());
            if(m_content_length >= 0)
            {
                m_has_content_length = true;
            }
        }
        else if(strncasecmp(strtext.c_str(), "Transfer-Encoding:", 18) == 0)
        {
            string strEncoding;
            strcut(strtext.c_str(), "Transfer-Encoding:", NULL, strEncoding);
            strtrim(strEncoding);
            if(strEncoding == "chunked")
            {
                m_is_chunked = true;
            }
        }
        else if(strncasecmp(strtext.c_str(), "Cache-Control:", 14) == 0)
        {
            strcut(strtext.c_str(), "Cache-Control:", NULL, m_cache_control);
            strtrim(m_cache_control);
            
            if(strstr(m_cache_control.c_str() + 14, "no-cache") != NULL
                || strstr(m_cache_control.c_str() + 14, "no-store") != NULL
                || strstr(m_cache_control.c_str() + 14, "proxy-revalidate") != NULL
                || strstr(m_cache_control.c_str() + 14, "must-revalidate") != NULL
                || strstr(m_cache_control.c_str() + 14, "private") != NULL)
            {
                m_use_cache = false;
            }
            else
            {
                const char* p_max_age = strstr(m_cache_control.c_str() + 14, "max-age="); // eg.: Cache-Control:public, max-age=31536000
                
                if(p_max_age != NULL)
                {
                    string str_max_age;
                    strcut(p_max_age, "max-age=", NULL, str_max_age);
                    strtrim(str_max_age);	
                    int n_max_age = atoi(str_max_age.c_str());
                    
                    m_cache_max_age = n_max_age < m_cache_max_age ? (n_max_age < 0 ? 0 : n_max_age) : m_cache_max_age;
                
                    if(m_cache_max_age > 0)
                    {
                        m_use_cache = true;
                    }
                    else
                    {
                        m_use_cache = false;
                    }
                }
            }            
        }
        else if(strncasecmp(strtext.c_str(), "ETag:", 5) == 0)
        {
            strcut(strtext.c_str(), "ETag:", NULL, m_etag);
            strtrim(m_etag, "\" ");
        }
        else if(strncasecmp(strtext.c_str(), "Server:", 7) == 0)
        {
            strcut(strtext.c_str(), "Server:", NULL, m_server);
            strtrim(m_server);
        }
        else if(strncasecmp(strtext.c_str(), "Last-Modified:", 14) == 0)
        {
            strcut(strtext.c_str(), "Last-Modified:", NULL, m_last_modified);
            strtrim(m_last_modified);
        }
        else if(strncasecmp(strtext.c_str(), "Expires:", 8) == 0)
        {
            strcut(strtext.c_str(), "Expires:", NULL, m_expires);
            strtrim(m_expires);
            
            int n_expire = ParseGMTorUTCTimeString(m_expires.c_str()) - time(NULL);
            
            m_cache_max_age = n_expire < m_cache_max_age ? (n_expire < 0 ? 0 : n_expire) : m_cache_max_age;
            
            
            if(m_cache_max_age > 0)
            {
                m_use_cache = true;
            }
            else
            {
                m_use_cache = false;
            }
        }
        else if(strncasecmp(strtext.c_str(), "Content-Type:", 13) == 0)
        {
            strcut(strtext.c_str(), "Content-Type:", NULL, m_content_type);
            strtrim(m_content_type);
        }
    }
	
	return true;
}

bool http_client::processing(const char* buf, int buf_len, int& next_recv_len)
{
    if(m_state == HTTP_Client_Parse_State_Header)
    {
        next_recv_len = 1;
                        
        if(m_buf_len - m_buf_used_len < buf_len)
        {
            char* new_buf = (char*)malloc(m_buf_used_len + buf_len + 1);
            new_buf[m_buf_used_len + buf_len] = '\0';
            
            memcpy(new_buf, m_buf, m_buf_used_len);
            
            free(m_buf);
            
            m_buf = new_buf;
            m_buf_len = m_buf_used_len + buf_len;
        }
        
        memcpy(m_buf + m_buf_used_len, buf, buf_len);
        m_buf_used_len += buf_len;        
        
        if(m_buf_used_len >= 4)
        {
            bool found_endle = false;
            for(int x = 0; x < m_buf_used_len; x++)
            {
                if(memcmp(m_buf + x, "\r\n\r\n", 4) == 0)
                {
                    if(m_buf_used_len > 64*1024)
                        return false;
                    
                    if(!parse(m_buf))
						return false;
                    
                    if(_Send_(m_client_sockfd, m_buf, x + 4) < 0)
                    {
                        close(m_client_sockfd);
                        close(m_backend_sockfd);
                        return false;
                    }
                    
                    memmove(m_buf, m_buf + x + 4, m_buf_used_len - x - 4);
                    m_buf_used_len = m_buf_used_len - x - 4;
                    
                    
                    m_state = HTTP_Client_Parse_State_Content;
                    
                    m_sent_content = 0;
                    
                    if(is_chunked())
                    {
                        if(!m_chunk)
                            m_chunk = new http_chunk(m_client_sockfd, m_backend_sockfd);
                        if(!m_chunk->processing(m_buf, m_buf_used_len, next_recv_len))
                            return false;
                    }
                    else
                    {
                        if(m_buf_used_len > 0)
                        {
                            if(m_use_cache && m_cache_buf)
                            {
                                memcpy(m_cache_buf + m_cache_data_len, m_buf, m_buf_used_len);
                                m_cache_data_len += m_buf_used_len;
                            }
                            if(_Send_(m_client_sockfd, m_buf, m_buf_used_len) < 0)
                            {
                                close(m_client_sockfd);
                                close(m_backend_sockfd);
                                return false;
                            }
                            m_sent_content += m_buf_used_len;
                            
                            m_buf_used_len = 0;
                            
                            if(has_content_length() && m_sent_content == m_content_length)
                            {
                                return false;
                            }
                        }
            
                        if(has_content_length() && 0 == m_content_length)
                        {
                            return false;
                        }
                        next_recv_len = m_content_length - m_sent_content;
                        
                    }
                    found_endle = true;
                    break;
                }
            }
        }
        
    }
    else if(m_state == HTTP_Client_Parse_State_Content)
    {
        if(is_chunked())
        {
            if(!m_chunk)
                m_chunk = new http_chunk(m_client_sockfd, m_backend_sockfd);
            m_chunk->processing(buf, buf_len, next_recv_len);
        }
        else
        {
            //Send the left data from the last send.
            if(m_buf_used_len > 0)
            {
                
                if(m_use_cache && m_cache_buf)
                {
                    memcpy(m_cache_buf + m_cache_data_len, m_buf, m_buf_used_len);
                    m_cache_data_len += m_buf_used_len;
                }
                
                if(_Send_(m_client_sockfd, m_buf, m_buf_used_len) < 0)
                {
                    close(m_client_sockfd);
                    close(m_backend_sockfd);
                    return false;
                }
                m_sent_content += m_buf_used_len;
                
                m_buf_used_len = 0;
                
                if(has_content_length() && m_sent_content == m_content_length)
                {
                    return false;
                }
                
            }
            else
            {
                int expected_send_len = has_content_length() ? (m_content_length - m_sent_content) : buf_len;
                
                int should_send_len = buf_len < expected_send_len ? buf_len : expected_send_len;
                
                if(m_use_cache && m_cache_buf)
                {
                    memcpy(m_cache_buf + m_cache_data_len, buf, should_send_len);
                    m_cache_data_len += should_send_len;
                }
                
                if(_Send_(m_client_sockfd, buf, should_send_len) < 0)
                {
                    close(m_client_sockfd);
                    close(m_backend_sockfd);
                    return false;
                }
                
                m_sent_content += should_send_len;
                
                if(has_content_length() && m_sent_content == m_content_length)
                {
                    return false;
                }
            }
        }
    }
    
    return true;
}