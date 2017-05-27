/*
	Copyright (c) openheap, uplusware
	uplusware@gmail.com
*/

#include "fastcgi.h"

fastcgi::fastcgi(const char* ipaddr, unsigned short port)
    : cgi_base(ipaddr, port)
{

}

fastcgi::fastcgi(const char* sock_file)
    : cgi_base(sock_file)
{

}

fastcgi::~fastcgi()
{

}

int fastcgi::BeginRequest(unsigned short request_id)
{
	m_RequestIDB0 = request_id & 0x00FF;
	m_RequestIDB1 = (request_id & 0xFF00) >> 8;
	FCGI_Header fcgi_header;
	memset(&fcgi_header, 0, sizeof(FCGI_Header));
	fcgi_header.version = FCGI_VERSION_1;
	fcgi_header.type = FCGI_BEGIN_REQUEST;
    fcgi_header.requestIdB0 = m_RequestIDB0;
	fcgi_header.requestIdB1 = m_RequestIDB1;
    fcgi_header.contentLengthB0 = sizeof(FCGI_BeginRequestBody);
	fcgi_header.contentLengthB1 = 0;
	FCGI_BeginRequestBody begin_request_body;
	memset(&begin_request_body, 0, sizeof(FCGI_BeginRequestBody));
	begin_request_body.roleB0 = FCGI_RESPONDER;
	
	if(Send((char*)&fcgi_header, sizeof(FCGI_Header)) >= 0 && Send((char*)&begin_request_body, sizeof(FCGI_BeginRequestBody)) >= 0)
	{
		return 0;
	}
	else
	{
		return -1;
	}
}

int fastcgi::SendParams(map<string, string> &params_map)
{
	FCGI_Header fcgi_header;
	memset(&fcgi_header, 0, sizeof(FCGI_Header));
	fcgi_header.version = FCGI_VERSION_1;
	fcgi_header.type = FCGI_PARAMS;
    fcgi_header.requestIdB0 = m_RequestIDB0;
	fcgi_header.requestIdB1 = m_RequestIDB1;
	fcgi_header.paddingLength = 0;
    
	unsigned long contentLength_32bit = 0;
	unsigned short contentLength_16bit = 0;
	map<string, string>::iterator it;
	for(it = params_map.begin(); it != params_map.end(); ++it)
	{
		unsigned int name_len = it->first.length();
		unsigned int value_len = it->second.length();
		if(name_len > 0 && value_len > 0)
		{
			if(name_len < 0x80 && value_len < 0x80)
			{
                contentLength_32bit += sizeof(FCGI_NameValuePair11);
			}
			else if(name_len < 0x80 && value_len > 0x80)
			{
                contentLength_32bit += sizeof(FCGI_NameValuePair14);
			}
			else if(name_len > 0x80 && value_len < 0x80)
			{
                contentLength_32bit += sizeof(FCGI_NameValuePair41);
			}
			else if(name_len > 0x80 && value_len > 0x80)
			{
                contentLength_32bit += sizeof(FCGI_NameValuePair44);
			}
            else
            {
                fprintf(stderr, "fcgi: wrong param, %d %d\n", name_len, value_len);
                continue;
            }
            contentLength_32bit += name_len;
            contentLength_32bit += value_len;
		}
	}
	
	contentLength_16bit = contentLength_32bit > 0xFFFF ? 0xFFFF : contentLength_32bit;
    fcgi_header.contentLengthB0 = contentLength_16bit & 0x00FF;
	fcgi_header.contentLengthB1 = (contentLength_16bit & 0xFF00) >> 8;
	
    fcgi_header.paddingLength = (8 - contentLength_16bit % 8) % 8;
    
	if(Send((char*)&fcgi_header, sizeof(FCGI_Header)) >= 0)
	{
		for(it = params_map.begin(); it != params_map.end(); ++it)
		{
			unsigned int name_len = it->first.length();
			unsigned int value_len = it->second.length();
			if(name_len > 0 && value_len > 0)
			{
				if(name_len < 0x80 && value_len < 0x80)
				{
					FCGI_NameValuePair11 name_value_pair11;
					name_value_pair11.nameLengthB0 = name_len;
					name_value_pair11.valueLengthB0 = value_len;
					
					Send((char*)&name_value_pair11, sizeof(FCGI_NameValuePair11));
				}
				else if(name_len < 0x80 && value_len > 0x80)
				{
					FCGI_NameValuePair14 name_value_pair14;
					name_value_pair14.nameLengthB0 = name_len;
                    
					name_value_pair14.valueLengthB0 = value_len & 0x000000FF;
					name_value_pair14.valueLengthB1 = (value_len & 0x0000FF00) >> 8;
					name_value_pair14.valueLengthB2 = (value_len & 0x00FF0000) >> 16;
					name_value_pair14.valueLengthB3 = (value_len & 0xFF000000) >> 24;
                    name_value_pair14.valueLengthB3 |= 0x80;
					
					Send((char*)&name_value_pair14, sizeof(FCGI_NameValuePair14));
				}
				else if(name_len > 0x80 && value_len < 0x80)
				{
					FCGI_NameValuePair41 name_value_pair41;
					name_value_pair41.nameLengthB0 = name_len & 0x000000FF;
					name_value_pair41.nameLengthB1 = (name_len & 0x0000FF00) >> 8;
					name_value_pair41.nameLengthB2 = (name_len & 0x00FF0000) >> 16;
					name_value_pair41.nameLengthB3 = (name_len & 0xFF000000) >> 24;
                    name_value_pair41.nameLengthB3 |= 0x80;
                    
					name_value_pair41.valueLengthB0 = value_len;
                    
					Send((char*)&name_value_pair41, sizeof(FCGI_NameValuePair41));
				}
				else if(name_len > 0x80 && value_len > 0x80)
				{
					FCGI_NameValuePair44 name_value_pair44;
					name_value_pair44.nameLengthB0 = name_len & 0x000000FF;
					name_value_pair44.nameLengthB1 = (name_len & 0x0000FF00) >> 8;
					name_value_pair44.nameLengthB2 = (name_len & 0x00FF0000) >> 16;
					name_value_pair44.nameLengthB3 = (name_len & 0xFF000000) >> 24;
					name_value_pair44.nameLengthB3 |= 0x80;
                    
                    name_value_pair44.valueLengthB0 = value_len & 0x000000FF;
					name_value_pair44.valueLengthB1 = (value_len & 0x0000FF00) >> 8;
					name_value_pair44.valueLengthB2 = (value_len & 0x00FF0000) >> 16;
					name_value_pair44.valueLengthB3 = (value_len & 0xFF000000) >> 24;
                    name_value_pair44.valueLengthB3 |= 0x80;
                    
					Send((char*)&name_value_pair44, sizeof(FCGI_NameValuePair44));
				}
                else
                {
                    fprintf(stderr, "fcgi: wrong param, %d %d\n", name_len, value_len);
                    continue;
                }
                Send((char*)it->first.c_str(), it->first.length());
				Send((char*)it->second.c_str(), it->second.length());
			}
		}
        
        if(fcgi_header.paddingLength > 0)
        {
            char pad_data[8];
            memset(pad_data, 0, fcgi_header.paddingLength);
            Send(pad_data, fcgi_header.paddingLength);
        }
		return 0;
	}
	else
		return -1;
}

int fastcgi::SendEmptyParams()
{
	FCGI_Header fcgi_header;
	memset(&fcgi_header, 0, sizeof(FCGI_Header));
	fcgi_header.version = FCGI_VERSION_1;
	fcgi_header.type = FCGI_PARAMS;
    fcgi_header.requestIdB0 = m_RequestIDB0;
	fcgi_header.requestIdB1 = m_RequestIDB1;
    fcgi_header.contentLengthB0 = 0;
	fcgi_header.contentLengthB1 = 0;
    fcgi_header.paddingLength = 0;
	if(Send((char*)&fcgi_header, sizeof(FCGI_Header)) >= 0 )
	{
        return 0;
	}
	else
		return -1;
}

int fastcgi::SendRequestData(const char* inbuf, unsigned long inbuf_len)
{
	FCGI_Header fcgi_header;
	memset(&fcgi_header, 0, sizeof(FCGI_Header));
	fcgi_header.version = FCGI_VERSION_1;
	fcgi_header.type = FCGI_STDIN;
    fcgi_header.requestIdB0 = m_RequestIDB0;
	fcgi_header.requestIdB1 = m_RequestIDB1;
	
	unsigned short contentLength_16bit = inbuf_len > 0xFFFF ? 0xFFFF : inbuf_len;
    fcgi_header.contentLengthB0 = contentLength_16bit & 0x00FF;
	fcgi_header.contentLengthB1 = (contentLength_16bit & 0xFF00) >> 8;
	
    if(Send((char*)&fcgi_header, sizeof(FCGI_Header)) >= 0 )
	{
		Send(inbuf, inbuf_len);
		return 0;
	}
	else
		return -1;
}

int fastcgi::SendEmptyRequestData()
{
	FCGI_Header fcgi_header;
	memset(&fcgi_header, 0, sizeof(FCGI_Header));
	fcgi_header.version = FCGI_VERSION_1;
	fcgi_header.type = FCGI_STDIN;
    fcgi_header.requestIdB0 = m_RequestIDB0;
	fcgi_header.requestIdB1 = m_RequestIDB1;
    fcgi_header.contentLengthB0 = 0;
	fcgi_header.contentLengthB1 = 0;
    if(Send((char*)&fcgi_header, sizeof(FCGI_Header)) >= 0 )
	{
    	return 0;
	}
	else
		return -1;
}

int fastcgi::RecvAppData(vector<char>& binaryResponse, string& strerr, unsigned int & appstatus, unsigned char & protocolstatus,
    BOOL& continue_recv)
{
    continue_recv = FALSE;
	FCGI_Header fcgi_header;

    char * contentBuffer = NULL;
    char paddingBuffer[255];
    unsigned short contentLength = 0;
    unsigned char paddingLength = 0;
    
    int nlen = Recv((char*)&fcgi_header, sizeof(FCGI_Header));
    if( nlen == sizeof(FCGI_Header) )
    {
        contentLength = fcgi_header.contentLengthB1;
        contentLength <<= 8;
        contentLength += fcgi_header.contentLengthB0;
        
        paddingLength = fcgi_header.paddingLength;
        
        if(contentLength > 0)
        {
            contentBuffer  = new char[contentLength];
            if(!contentBuffer || Recv(contentBuffer, contentLength) < 0)
            {
                
                if(contentBuffer)
                    delete[] contentBuffer;
                contentBuffer = NULL;
                return -1;
            }
        }
        
        if(paddingLength > 0)
        {
            Recv(paddingBuffer, paddingLength);
        }
        
        switch(fcgi_header.type)
        {
            case FCGI_STDOUT:
                if(contentLength > 0)
                {
                    for(int x = 0; x < contentLength; x++)
                    {
                        binaryResponse.push_back(contentBuffer[x]);
                    }
                }
                continue_recv = TRUE;
                break;
            case FCGI_STDERR:
                if(contentLength > 0)
                {
                    strerr += contentBuffer;
                }
                continue_recv = TRUE;
                break;
            case FCGI_END_REQUEST:
                if(contentLength > 0)
                {
                    FCGI_EndRequestBody * end_request_body = (FCGI_EndRequestBody*)contentBuffer;
                    appstatus = end_request_body->appStatusB3 << 24;
                    appstatus += end_request_body->appStatusB2 << 16;
                    appstatus += end_request_body->appStatusB1 << 8;
                    appstatus += end_request_body->appStatusB0;
                    
                    protocolstatus = end_request_body->protocolStatus;
                }
                else
                {
                    appstatus = 0;
                    protocolstatus = FCGI_UNKNOWN_STATUS;
                }
                break;
            default:
                fprintf(stderr, "fastcgi: unknown type: %d\n", fcgi_header.type);
                break;
            
        }
        
        if(contentBuffer)
            delete[] contentBuffer;
        contentBuffer = NULL;			
    }
    else
    {
        fprintf(stderr, "fastcgi: recv header length (%d)\n", nlen);
        return -1;
    }
    
	return contentLength;
}


int fastcgi::AbortRequest()
{
	FCGI_Header fcgi_header;
	memset(&fcgi_header, 0, sizeof(FCGI_Header));
	fcgi_header.version = FCGI_VERSION_1;
	fcgi_header.type = FCGI_ABORT_REQUEST;
    fcgi_header.requestIdB0 = m_RequestIDB0;
	fcgi_header.requestIdB1 = m_RequestIDB1;
	
	
	if(Send((char*)&fcgi_header, sizeof(FCGI_Header)) >= 0)
		return 0;
	else
		return -1;
}

int fastcgi::EndRequest(unsigned int app_status, unsigned char protocol_status)
{
	FCGI_Header fcgi_header;
	memset(&fcgi_header, 0, sizeof(FCGI_Header));
	fcgi_header.version = FCGI_VERSION_1;
	fcgi_header.type = FCGI_END_REQUEST;
    fcgi_header.requestIdB0 = m_RequestIDB0;
	fcgi_header.requestIdB1 = m_RequestIDB1;
    fcgi_header.contentLengthB0 = sizeof(FCGI_EndRequestBody);
	
	FCGI_EndRequestBody end_request_body;
	memset(&end_request_body, 0, sizeof(FCGI_EndRequestBody));
	end_request_body.appStatusB0 = app_status&0x000000FF;
	end_request_body.appStatusB1 = (app_status&0x0000FF00)>>8;
	end_request_body.appStatusB1 = (app_status&0x00FF0000)>>16;
	end_request_body.appStatusB1 = (app_status&0xFF000000)>>24;
	end_request_body.protocolStatus = protocol_status;
	
	if(Send((char*)&fcgi_header, sizeof(FCGI_Header)) >= 0 && Send((char*)&end_request_body, sizeof(FCGI_EndRequestBody)) >= 0)
		return 0;
	else
		return -1;
}

int fastcgi::GetAppValue(const char* name, string& value)
{
	FCGI_Header fcgi_header;
	memset(&fcgi_header, 0, sizeof(FCGI_Header));
	fcgi_header.version = FCGI_VERSION_1;
	fcgi_header.type = FCGI_GET_VALUES;
    fcgi_header.requestIdB0 = m_RequestIDB0;
	fcgi_header.requestIdB1 = m_RequestIDB1;
    fcgi_header.contentLengthB0 = strlen(name);
	
	if(Send((char*)&fcgi_header, sizeof(FCGI_Header)) >= 0 && Send((char*)&name, strlen(name)) >= 0)
	{
		//Send empty as ending
		fcgi_header.contentLengthB0 = 0;
		if(Send((char*)&fcgi_header, sizeof(FCGI_Header)) >= 0)
		{
			FCGI_Header fcgi_header;
			bool bQuit = false;
			while(bQuit)
			{
				char * contentBuffer = NULL;
				if(Recv((char*)&fcgi_header, sizeof(FCGI_Header)) >= 0 )
				{
					unsigned short contentLength = 0;
					contentLength = fcgi_header.contentLengthB1;
					contentLength << 8;
					contentLength += fcgi_header.contentLengthB0;
					
					if(contentLength > 0)
					{
						char * contentBuffer  = new char[contentLength + 1];
						if(contentBuffer && Recv(contentBuffer, contentLength) > 0)
						{
							contentBuffer[contentLength] = '\0';
						}
						else
						{
							if(contentBuffer)
								delete[] contentBuffer;
							return -1;
						}
					}
					else
					{
						bQuit = true;
					}
					
					switch(fcgi_header.type)
					{
						case FCGI_GET_VALUES_RESULT:
							if(contentLength > 0)
								value += contentBuffer;
						break;
						default:
						break;
						
					}
					
					if(contentBuffer)
						delete[] contentBuffer;		
				}
				else
					return -1;
			}
			return 0;
		}
		else
			return -1;
	}
	else
		return -1;

}

int fastcgi::SendParamsAndData(map<string, string> &params_map, const char* postdata, unsigned int postdata_len)
{
    if(BeginRequest(1) != 0)
        return -1;
    
    if(params_map.size() > 0)
    {
        if(SendParams(params_map) < 0)
            return -1;
    }
    
    if(SendEmptyParams() < 0)
        return -1;
    
    if(postdata_len > 0)
    {
        if(SendRequestData(postdata, postdata_len) < 0)
            return -1;
    }
    return SendEmptyRequestData();
}