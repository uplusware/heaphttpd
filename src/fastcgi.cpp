/*
	Copyright (c) openheap, uplusware
	uplusware@gmail.com
*/

#include "fastcgi.h"

FastCGI::FastCGI(const char* ipaddr, unsigned short port)
    : cgi_base(ipaddr, port)
{

}

FastCGI::FastCGI(const char* sock_file)
    : cgi_base(sock_file)
{

}

FastCGI::~FastCGI()
{

}

int FastCGI::BeginRequest(unsigned short request_id)
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

int FastCGI::SendParams(map<string, string> &params_map)
{
	FCGI_Header fcgi_header;
	memset(&fcgi_header, 0, sizeof(FCGI_Header));
	fcgi_header.version = FCGI_VERSION_1;
	fcgi_header.type = FCGI_PARAMS;
    fcgi_header.requestIdB0 = m_RequestIDB0;
	fcgi_header.requestIdB1 = m_RequestIDB1;
	
	unsigned long contentLength_32bit = 0;
	unsigned short contentLength_16bit = 0;
	map<string, string>::iterator it;
	for(it = params_map.begin(); it != params_map.end(); ++it)
	{
		unsigned int name_len = it->first.length();
		unsigned int value_len = it->second.length();
		if(name_len > 0 && value_len > 0)
		{
			contentLength_32bit += name_len + value_len;
			if(name_len >> 7 == 0 && value_len >> 7 == 0)
			{
				contentLength_32bit += sizeof(FCGI_NameValuePair11);
			}
			else if(name_len >> 7 == 0 && value_len >> 7 == 1)
			{
				contentLength_32bit += sizeof(FCGI_NameValuePair14);
			}
			else if(name_len >> 7 == 1 && value_len >> 7 == 0)
			{
				contentLength_32bit += sizeof(FCGI_NameValuePair41);
			}
			else if(name_len >> 7 == 1 && value_len >> 7 == 1)
			{
				contentLength_32bit += sizeof(FCGI_NameValuePair44);
			}
		}
	}
	
	contentLength_16bit = contentLength_32bit > 65535 ? 65535 : contentLength_32bit;
    fcgi_header.contentLengthB0 = contentLength_16bit & 0x00FF;
	fcgi_header.contentLengthB1 = (contentLength_16bit & 0xFF00) >> 8;
	
	if(Send((char*)&fcgi_header, sizeof(FCGI_Header)) >= 0)
	{
		for(it = params_map.begin(); it != params_map.end(); ++it)
		{
			
			unsigned int name_len = it->first.length();
			unsigned int value_len = it->second.length();
			if(name_len > 0 && value_len > 0)
			{
				if(name_len >> 7 == 0 && value_len >> 7 == 0)
				{
					FCGI_NameValuePair11 name_value_pair11;
					name_value_pair11.nameLengthB0 = name_len;
					name_value_pair11.valueLengthB0 = value_len;
					
					Send((char*)&name_value_pair11, sizeof(FCGI_NameValuePair11));
				}
				else if(name_len >> 7 == 0 && value_len >> 7 == 1)
				{
					FCGI_NameValuePair14 name_value_pair14;
					name_value_pair14.nameLengthB0 = name_len;
					name_value_pair14.valueLengthB0 = value_len & 0x000000FF;
					name_value_pair14.valueLengthB1 = (value_len & 0x0000FF00) >>8;
					name_value_pair14.valueLengthB2 = (value_len & 0x00FF0000) >>16;
					name_value_pair14.valueLengthB3 = (value_len & 0xFF000000) >>24;
					
					Send((char*)&name_value_pair14, sizeof(FCGI_NameValuePair14));
				}
				else if(name_len >> 7 == 1 && value_len >> 7 == 0)
				{
					FCGI_NameValuePair41 name_value_pair41;
					name_value_pair41.nameLengthB0 = name_len & 0x000000FF;
					name_value_pair41.nameLengthB1 = (name_len & 0x0000FF00) >> 8;
					name_value_pair41.nameLengthB2 = (name_len & 0x00FF0000) >> 16;
					name_value_pair41.nameLengthB3 = (name_len & 0xFF000000) >> 24;
					name_value_pair41.valueLengthB0 = value_len;
					Send((char*)&name_value_pair41, sizeof(FCGI_NameValuePair41));
				}
				else if(name_len >> 7 == 1 && value_len >> 7 == 1)
				{
					FCGI_NameValuePair44 name_value_pair44;
					name_value_pair44.nameLengthB0 = name_len & 0x000000FF;
					name_value_pair44.nameLengthB1 = (name_len & 0x0000FF00) >> 8;
					name_value_pair44.nameLengthB2 = (name_len & 0x00FF0000) >> 16;
					name_value_pair44.nameLengthB3 = (name_len & 0xFF000000) >>24;
					name_value_pair44.valueLengthB0 = value_len & 0x000000FF;
					name_value_pair44.valueLengthB1 = (value_len & 0x0000FF00) >> 8;
					name_value_pair44.valueLengthB2 = (value_len & 0x00FF0000) >> 16;
					name_value_pair44.valueLengthB3 = (value_len & 0xFF000000) >> 24;
					Send((char*)&name_value_pair44, sizeof(FCGI_NameValuePair44));
				}
				/* printf("%s = [%s]\n", it->first.c_str(), it->second.c_str()); */
				Send((char*)it->first.c_str(), it->first.length());
				Send((char*)it->second.c_str(), it->second.length());
			}
		}
		return 0;
	}
	else
		return -1;
}

int FastCGI::SendParams(const char* name, unsigned int name_len, const char* value, unsigned int value_len)
{
	FCGI_Header fcgi_header;
	memset(&fcgi_header, 0, sizeof(FCGI_Header));
	fcgi_header.version = FCGI_VERSION_1;
	fcgi_header.type = FCGI_PARAMS;
    fcgi_header.requestIdB0 = m_RequestIDB0;
	fcgi_header.requestIdB1 = m_RequestIDB1;
	
	unsigned long contentLength_32bit = 0;
	unsigned short contentLength_16bit = 0;
	
	contentLength_32bit = name_len + value_len;
	if(name_len >> 7 == 0 && value_len >> 7 == 0)
	{
		contentLength_32bit += sizeof(FCGI_NameValuePair11);
	}
	else if(name_len >> 7 == 0 && value_len >> 7 == 1)
	{
		contentLength_32bit += sizeof(FCGI_NameValuePair14);
	}
	else if(name_len >> 7 == 1 && value_len >> 7 == 0)
	{
		contentLength_32bit += sizeof(FCGI_NameValuePair41);
	}
	else if(name_len >> 7 == 1 && value_len >> 7 == 1)
	{
		contentLength_32bit += sizeof(FCGI_NameValuePair44);
	}
	
	contentLength_16bit = contentLength_32bit > 0xFFFF ? 0xFFFF : contentLength_32bit;
    fcgi_header.contentLengthB0 = contentLength_16bit & 0x00FF;
	fcgi_header.contentLengthB1 = (contentLength_16bit & 0xFF00) >> 8;
	
	if(Send((char*)&fcgi_header, sizeof(FCGI_Header)) >= 0 )
	{
		Send((char*)name, name_len);
		Send((char*)value, value_len);
	
		return 0;
	}
	else
		return -1;
}

int FastCGI::SendEmptyParams()
{
	FCGI_Header fcgi_header;
	memset(&fcgi_header, 0, sizeof(FCGI_Header));
	fcgi_header.version = FCGI_VERSION_1;
	fcgi_header.type = FCGI_PARAMS;
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

int FastCGI::Send_STDIN(const char* inbuf, unsigned long inbuf_len)
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

int FastCGI::SendEmpty_STDIN()
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

int FastCGI::RecvAppData(vector<char>& appout, string& strerr, unsigned int & appstatus, unsigned char & protocolstatus,
    BOOL& continue_recv)
{
    continue_recv = FALSE;
	FCGI_Header fcgi_header;

    char * contentBuffer = NULL;
    char paddingBuffer[255];
    unsigned short contentLength = 0;
    unsigned char paddingLength = 0;
    
    if(Recv((char*)&fcgi_header, sizeof(FCGI_Header)) >= 0 )
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
                        appout.push_back(contentBuffer[x]);
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
        return -1;
    
	return contentLength;
}


int FastCGI::AbortRequest()
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

int FastCGI::EndRequest(unsigned int app_status, unsigned char protocol_status)
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

int FastCGI::GetAppValue(const char* name, string& value)
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
