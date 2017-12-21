/*
	Copyright (c) openheap, uplusware
	uplusware@gmail.com
*/

#include "http_client.h"
#include "version.h"

//http_chunk
http_chunk::http_chunk(int client_sockfd, SSL* client_ssl, int backend_sockfd)
{
    m_client_sockfd = client_sockfd;
    m_client_ssl = client_ssl;
    m_backend_sockfd = backend_sockfd;
    m_chunk_len = -1;
    m_sent_chunk = 0;
    m_state = HTTP_Client_Parse_State_Chunk_Header;
}

http_chunk::~http_chunk()
{
    
}

int http_chunk::client_send(const char* buf, int len)
{
	if(m_client_ssl)
		return SSLWrite(m_client_sockfd, m_client_ssl, buf, len, CHttpBase::m_connection_idle_timeout);
	else
		return _Send_( m_client_sockfd, buf, len, CHttpBase::m_connection_idle_timeout);
		
}

bool http_chunk::processing(char* & derived_buf, int& derived_buf_used_len)
{
    //share buffer with http_client instance
    while(derived_buf_used_len > 0)
    {
        if(m_state == HTTP_Client_Parse_State_Chunk_Header)
        {
            bool found_crlf = false;
            
            if(derived_buf_used_len >= 2)
            {
                for(int last_search_pos_crlf = 0; last_search_pos_crlf <= derived_buf_used_len - 2; last_search_pos_crlf++)
                {
                    if(memcmp(derived_buf + last_search_pos_crlf, "\r\n", 2) == 0)
                    {
                        found_crlf = true;
                        
                        char chunked_len[32];
                        memcpy(chunked_len, derived_buf, last_search_pos_crlf > 31 ? 31 : last_search_pos_crlf);                       
                        chunked_len[last_search_pos_crlf > 31 ? 31 : last_search_pos_crlf] = '\0';
                        
                        if(sscanf(chunked_len, "%x", &m_chunk_len) != 1)
                            return false;
                        
                        if(m_chunk_len > 0)
                        {
                            m_chunk_len += 2;
                        
                            if(client_send(derived_buf, last_search_pos_crlf + 2) < 0) //send both the length number and crlf
                                return false;
                            
                            memmove(derived_buf, derived_buf + last_search_pos_crlf + 2, derived_buf_used_len - last_search_pos_crlf - 2);
                            derived_buf_used_len = derived_buf_used_len - last_search_pos_crlf - 2;
                            
                            m_state = HTTP_Client_Parse_State_Chunk_Content;
                        
                            if(derived_buf_used_len > 0) /* begin to send chunk data */
                            {
                                int should_send_len = derived_buf_used_len > (m_chunk_len - m_sent_chunk) ? (m_chunk_len - m_sent_chunk) : derived_buf_used_len;
                                
                                if(client_send( derived_buf, should_send_len) < 0)
                                    return false;
                                
                                memmove(derived_buf, derived_buf + should_send_len, derived_buf_used_len - should_send_len);
                                
                                m_sent_chunk += should_send_len;
                                derived_buf_used_len -= should_send_len;
                                
                                if(m_sent_chunk == m_chunk_len)
                                {
                                    m_state = HTTP_Client_Parse_State_Chunk_Header;
                                    m_chunk_len = 0;
                                    m_sent_chunk = 0;
                                }                                
                            }
                            
                            break;
                        
                        
                        }
                        else if(m_chunk_len == 0)
                        {
                            if(client_send(derived_buf, last_search_pos_crlf) < 0) //only send the length number
                                return false;
                            
                            memmove(derived_buf, derived_buf + last_search_pos_crlf, derived_buf_used_len - last_search_pos_crlf);
                            derived_buf_used_len = derived_buf_used_len - last_search_pos_crlf;
                            
                            m_state = HTTP_Client_Parse_State_Chunk_Footer;
                            
                            bool found_2crlfs = false;
                            
                            if(derived_buf_used_len >= 4)
                            {
                                for(int last_search_pos_2crlfs = 0; last_search_pos_2crlfs <= derived_buf_used_len - 4; last_search_pos_2crlfs++)
                                {
                                    if(memcmp(derived_buf + last_search_pos_2crlfs, "\r\n\r\n", 4) == 0)
                                    {
                                        found_2crlfs = true;
                                        if(client_send(derived_buf, last_search_pos_2crlfs + 4) < 0)
                                            return false;
                                        
                                        memmove(derived_buf, derived_buf + last_search_pos_2crlfs + 4, derived_buf_used_len - last_search_pos_2crlfs - 4);
                                        derived_buf_used_len = derived_buf_used_len - last_search_pos_2crlfs - 4;
                                        return false;
                                    }
                                }
                                
                            }
                            
                            if(!found_2crlfs)
                                return true;
                        }
                        else
                        {
                            return false;
                        }
                    }
                }
                

            }
            if(!found_crlf)
                return true;
        }
        else if(m_state == HTTP_Client_Parse_State_Chunk_Content)
        {
            //Send the left data from the last send.
            if(derived_buf_used_len > 0)
            {
                int should_send_len = derived_buf_used_len > (m_chunk_len - m_sent_chunk) ? (m_chunk_len - m_sent_chunk) : derived_buf_used_len;
                if(client_send( derived_buf, should_send_len) < 0)
                    return false;
                
                memmove(derived_buf, derived_buf + should_send_len, derived_buf_used_len - should_send_len);
                
                m_sent_chunk += should_send_len;
                derived_buf_used_len -= should_send_len;
                
                if(m_sent_chunk == m_chunk_len)
                {
                    m_state = HTTP_Client_Parse_State_Chunk_Header;
                    m_chunk_len = 0;
                    m_sent_chunk = 0;                    
                }
            }
        }
        else if(m_state == HTTP_Client_Parse_State_Chunk_Footer)
        {            
            bool found_2crlfs = false;
            
            if(derived_buf_used_len >= 4)
            {
                for(int last_search_pos_2crlfs = 0; last_search_pos_2crlfs <= derived_buf_used_len - 4; last_search_pos_2crlfs++)
                {
                    if(memcmp(derived_buf + last_search_pos_2crlfs, "\r\n\r\n", 4) == 0)
                    {
                        found_2crlfs = true;
                        
                        if(client_send(derived_buf, last_search_pos_2crlfs + 4) < 0)
                            return false;
                        
                        memmove(derived_buf, derived_buf + last_search_pos_2crlfs + 4, derived_buf_used_len - last_search_pos_2crlfs - 4);
                        derived_buf_used_len = derived_buf_used_len - last_search_pos_2crlfs - 4;
                        return false;
                    }
                }
            }
            
            if(!found_2crlfs)
                return true;
        }
    }
    return true;
}


// http_client
http_client::http_client(int client_sockfd, SSL* client_ssl, int backend_sockfd, memory_cache* cache, const char* http_url)
{
    m_client_sockfd = client_sockfd;
    m_client_ssl = client_ssl;
    m_backend_sockfd = backend_sockfd;
    
    m_http_tunneling_url = http_url;
    
    strtrim(m_http_tunneling_url);
    
    m_state = HTTP_Client_Parse_State_Header;
    
    m_content_length = -1;
    m_cache_max_age = 300;
    m_cache_shared_max_age = -1;
    
    m_has_content_length = true;
    m_is_chunked = false;
    
    m_out_of_cache_header_scope = false;
    m_has_via = false;
    
    m_buf = (char*)malloc(HTTP_CLIENT_BUF_LEN + 1);
    m_buf[HTTP_CLIENT_BUF_LEN] = '\0';
    m_buf_len = HTTP_CLIENT_BUF_LEN;
    m_buf_used_len = 0;
    
    m_sent_content_length = 0;

    m_chunk = NULL;
    
    m_cache = cache;
    
    m_use_cache = false; //default is true
    m_cache_buf = NULL;
    m_cache_data_len = 0;
    
    m_cache_completed = false;
    
    m_is_200_ok = false;
}

http_client::~http_client()
{
    if(CHttpBase::m_enable_http_tunneling_cache && m_use_cache && m_cache_completed && m_cache_buf && m_cache_data_len > 0 && m_http_tunneling_url != "")
    {
        tunneling_cache* c_out;
        m_cache->wrlock_tunneling_cache();
        m_cache->_push_tunneling_(m_http_tunneling_url.c_str(), m_cache_buf, m_cache_data_len, m_content_type.c_str(), m_cache_control.c_str(),
            m_allow.c_str(),
            m_encoding.c_str(),
            m_language.c_str(),
            m_last_modified.c_str(), m_etag.c_str(), m_expires.c_str(), m_server.c_str(), m_via.c_str(), m_cache_shared_max_age > 0 ? m_cache_shared_max_age : m_cache_max_age, &c_out);
        m_cache->unlock_tunneling_cache();
    }

    if(m_cache_buf)
        free(m_cache_buf);
    
    if(m_buf)
        free(m_buf);
    
    if(m_chunk)
        delete m_chunk;
}

int http_client::client_send(const char* buf, int len)
{
	if(m_client_ssl)
		return SSLWrite(m_client_sockfd, m_client_ssl, buf, len, CHttpBase::m_connection_idle_timeout);
	else
		return _Send_( m_client_sockfd, buf, len, CHttpBase::m_connection_idle_timeout);
		
}

bool http_client::parse(const char* text)
{
    string strtext;
    m_line_text += text;
    
    if(m_line_text.length() > 64*1024) // too long line
    {
        close(m_client_sockfd);
        close(m_backend_sockfd);
                
        return false;
    }
    
    std::size_t new_line;
    while((new_line = m_line_text.find('\n')) != std::string::npos)
    {
        strtext = m_line_text.substr(0, new_line + 1);
        m_line_text = m_line_text.substr(new_line + 1);

        strtrim(strtext);
        
        //printf(">>>>> %s\r\n", strtext.c_str());
        
        //format the header line.
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
            if(m_out_of_cache_header_scope)
            {
                m_use_cache = false;
            }
    
            if(CHttpBase::m_enable_http_tunneling_cache && m_use_cache && has_content_length()
                && m_content_length <= CHttpBase::m_single_tunneling_cache_size && m_content_length > 0) //only cache the little size data
            {
                m_cache_buf = (char*)malloc(m_content_length + 1);
                m_cache_data_len = 0;
            }
            
            if(!m_has_via)
            {
                string strVia = "Via: HTTP/1.1 ";
                strVia += CHttpBase::m_localhostname.c_str();
                strVia += "("VERSION_STRING")\r\n"; //append additional \r\n for ending header
                
                m_header += strVia;
            }
            
            //The empty line
            m_header += "\r\n";
            
            //Send all 
            if(client_send( m_header.c_str(), m_header.length()) < 0)
            {
                close(m_client_sockfd);
                close(m_backend_sockfd);
                return false;
            }
                
            break;
        }
        else if(strncasecmp(strtext.c_str(), "HTTP/1.0 ", 9) == 0
            || strncasecmp(strtext.c_str(), "HTTP/1.1 ", 9) == 0)
        {
            if(strstr(strtext.c_str() + 8, " 200 OK") != NULL)
            {
                m_is_200_ok = true;
            }
            else
            {
                m_has_content_length = true;
                m_content_length = 0;
            }
        }
        else if(strncasecmp(strtext.c_str(), "Content-Length:", 15) == 0)
        {
            string strLen;
            strcut(strtext.c_str(), "Content-Length:", NULL, strLen);
            strtrim(strLen);	
            m_content_length = atoll(strLen.c_str());
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
            
            if(strstr(strEncoding.c_str(), "chunked") != NULL)
            {
                m_is_chunked = true;
                m_use_cache = false; //unavailable in chunk mode
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
                
                    if(m_cache_max_age > 0 && m_is_200_ok && !is_chunked())
                    {
                        m_use_cache = true;
                    }
                    else
                    {
                        m_use_cache = false;
                    }
                }
                
                const char* p_shared_max_age = strstr(m_cache_control.c_str() + 14, "s-maxage="); // eg.: Cache-Control:public, max-age=31536000
                
                if(p_shared_max_age != NULL)
                {
                    string str_shared_max_age;
                    strcut(p_shared_max_age, "s-maxage=", NULL, str_shared_max_age);
                    strtrim(str_shared_max_age);	
                    int n_shared_max_age = atoi(str_shared_max_age.c_str());
                    
                    m_cache_shared_max_age = n_shared_max_age < m_cache_shared_max_age ? (n_shared_max_age < 0 ? 0 : n_shared_max_age) : m_cache_shared_max_age;
                
                    if(m_cache_shared_max_age > 0 && m_is_200_ok && !is_chunked())
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
        else if(strncasecmp(strtext.c_str(), "Allow:", 6) == 0)
        {
            strcut(strtext.c_str(), "Allow:", NULL, m_allow);
            strtrim(m_allow);
        }
        else if(strncasecmp(strtext.c_str(), "Content-Encoding:", 17) == 0)
        {
            strcut(strtext.c_str(), "Content-Encoding:", NULL, m_encoding);
            strtrim(m_encoding);
        }
        else if(strncasecmp(strtext.c_str(), "Content-Language:", 17) == 0)
        {
            strcut(strtext.c_str(), "Content-Language:", NULL, m_language);
            strtrim(m_language);
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
     
            if(m_cache_max_age > 0 && m_is_200_ok && !is_chunked())
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
        else if(strncasecmp(strtext.c_str(), "Via:", 4) == 0)
        {
            strcut(strtext.c_str(), "Server:", NULL, m_via);
            strtrim(m_via);
            
            m_has_via = true;
            
            // add the current via
            strtext += ", HTTP/1.1 ";
            strtext += CHttpBase::m_localhostname.c_str();
            strtext += "("VERSION_STRING")";
            
        }
        else if(strncasecmp(strtext.c_str(), "Connection:", 11) == 0 
            || strncasecmp(strtext.c_str(), "Accept-Ranges:", 14) == 0
            || strncasecmp(strtext.c_str(), "Date:", 5) == 0)
        {
            //do nothing
        }
        else
        {
            m_out_of_cache_header_scope = true;
        }
        
        //Each header line with \r\n
        m_header += strtext;
        m_header += "\r\n";
        
        //Send the header buffer if overflow/exceed the threshold
        if(m_header.length() > 4096)
        {
            if(client_send( m_header.c_str(), m_header.length()) < 0) // not x + 4 since need to append Via header fragment.
            {
                close(m_client_sockfd);
                close(m_backend_sockfd);
                return false;
            }
            m_header = "";
        }
    }
	return true;
}

bool http_client::processing(const char* buf, int buf_len)
{
    //Concatenate the buffer
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
    
    if(m_state == HTTP_Client_Parse_State_Header)
    {        
        if(m_buf_used_len >= 4)
        {
            for(int last_search_pos_2crlfs = 0; last_search_pos_2crlfs <= m_buf_used_len - 4; last_search_pos_2crlfs++)
            {
                if(memcmp(m_buf + last_search_pos_2crlfs, "\r\n\r\n", 4) == 0)
                {                    
                    if(!parse(m_buf))
                        return false;
                    
                    if(!has_content_length() && !is_chunked())
                    {
                        close(m_client_sockfd);
                        close(m_backend_sockfd);
                        return false;
                    }
                    
                    /*
                        The header has been sent in parse(...) function
                    */
                    memmove(m_buf, m_buf + last_search_pos_2crlfs + 4, m_buf_used_len - last_search_pos_2crlfs - 4);
                    m_buf_used_len = m_buf_used_len - last_search_pos_2crlfs - 4;

                    m_state = HTTP_Client_Parse_State_Content;
                    
                    m_sent_content_length = 0;
                    
                    if(is_chunked())
                    {
                        if(!m_chunk)
                            m_chunk = new http_chunk(m_client_sockfd, m_client_ssl, m_backend_sockfd);
                        
                        return m_chunk->processing(m_buf, m_buf_used_len);
                    }
                    else
                    {
                        if(has_content_length() && m_content_length == 0)
                        {
                            return false;
                        }
                        
                        if(m_buf_used_len > 0)
                        {
                            if(m_use_cache && m_cache_buf)
                            {
                                memcpy(m_cache_buf + m_cache_data_len, m_buf, m_buf_used_len);
                                m_cache_data_len += m_buf_used_len;
                            }
                            
                            if(client_send( m_buf, m_buf_used_len) < 0)
                            {
                                close(m_client_sockfd);
                                close(m_backend_sockfd);
                                return false;
                            }
                            m_sent_content_length += m_buf_used_len;
                            
                            m_buf_used_len = 0;
                            
                            if(has_content_length() && m_sent_content_length == m_content_length)
                            {
                                m_cache_completed = true;
                                
                                return false;
                            }
                        }
                    }
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
                m_chunk = new http_chunk(m_client_sockfd, m_client_ssl, m_backend_sockfd);
            
            return m_chunk->processing(m_buf, m_buf_used_len);
            
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
            
                if(client_send( m_buf, m_buf_used_len) < 0)
                {
                    close(m_client_sockfd);
                    close(m_backend_sockfd);
                    return false;
                }
                m_sent_content_length += m_buf_used_len;
                
                m_buf_used_len = 0;
                
                if(has_content_length() && m_sent_content_length == m_content_length)
                {
                    m_cache_completed = true;
                    
                    return false;
                }
                
            }
        }
    }
    return true;
}
