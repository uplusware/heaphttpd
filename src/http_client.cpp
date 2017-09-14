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
http_client::http_client(int client_sockfd, int backend_sockfd)
{
    m_client_sockfd = client_sockfd;
    m_backend_sockfd = backend_sockfd;
    
    m_state = HTTP_Client_Parse_State_Header;
    
    m_content_length = -1;
    m_has_content_length = false;
    m_is_chunked = false;
    
    m_buf = (char*)malloc(HTTP_CLIENT_BUF_LEN + 1);
    m_buf[HTTP_CLIENT_BUF_LEN] = '\0';
    m_buf_len = HTTP_CLIENT_BUF_LEN;
    m_buf_used_len = 0;
    
    m_sent_content = 0;
    
    m_chunk = NULL;
}

http_client::~http_client()
{
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
        //printf(">>>>> %s\r\n", strtext.c_str());
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
            break;
        }
        else if(strncasecmp(strtext.c_str(), "Content-Length", 14) == 0)
        {
            string strLen;
            strcut(strtext.c_str(), "Content-Length:", NULL, strLen);
            strtrim(strLen);	
            m_content_length = atoi(strLen.c_str());
            if(m_content_length >= 0)
                m_has_content_length = true;
			/*else
				return false;*/
        }
        else if(strncasecmp(strtext.c_str(), "Transfer-Encoding", 17) == 0)
        {
            string strEncoding;
            strcut(strtext.c_str(), "Content-Length:", NULL, strEncoding);
            strtrim(strEncoding);
            if(strEncoding == "chunked")
            {
                m_is_chunked = true;
            }
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