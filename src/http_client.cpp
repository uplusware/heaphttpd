/*
	Copyright (c) openheap, uplusware
	uplusware@gmail.com
*/

#include "http_client.h"

http_client::http_client()
{
    m_content_length = -1;
}

http_client::~http_client()
{
    
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
        }
    }
}