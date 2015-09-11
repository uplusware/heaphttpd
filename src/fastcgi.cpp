/*
	Copyright (c) openheap, uplusware
	uplusware@gmail.com
*/
#include <unistd.h>
#include <fcntl.h>
#include            <stdio.h> 
#include            <sys/types.h> 
#include            <sys/socket.h> 
#include            <netinet/in.h> 
#include            <arpa/inet.h>
#include "fastcgi.h"
#include "util/general.h"

FastCGI::FastCGI(const char* ipaddr, unsigned short port)
{
	m_sockfd = -1;
	m_strIP = ipaddr;
	m_nPort = port;
}

FastCGI::~FastCGI()
{
	if(m_sockfd >= 0)
		close(m_sockfd);
}

int FastCGI::Connect()
{
	string err_msg;
	int err_code = 1;
	int err_code_len = sizeof(err_code);
	int res;
	struct sockaddr_in ser_addr;
	fd_set mask_r, mask_w; 
	struct timeval timeout; 
	
	m_sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(m_sockfd < 0)
	{
		err_msg = "system error\r\n";
		return -1;
	}
	
	int flags = fcntl(m_sockfd, F_GETFL, 0); 
	fcntl(m_sockfd, F_SETFL, flags | O_NONBLOCK); 
	
	ser_addr.sin_family = AF_INET;
	ser_addr.sin_port = htons(m_nPort);
	ser_addr.sin_addr.s_addr = inet_addr(m_strIP.c_str());
	
	timeout.tv_sec = 10; 
	timeout.tv_usec = 0;
	
	connect(m_sockfd,(struct sockaddr*)&ser_addr,sizeof(struct sockaddr));
	
	FD_ZERO(&mask_r);
	FD_ZERO(&mask_w);
	
	FD_SET(m_sockfd, &mask_r);
	FD_SET(m_sockfd, &mask_w);
	res = select(m_sockfd + 1, &mask_r, &mask_w, NULL, &timeout);
	
	//printf("res: %d\n", res);
	if( res != 1) 
	{
		close(m_sockfd);
		m_sockfd = -1;
		err_msg = "can not connect the ";
		err_msg += m_strIP;
		err_msg += ".\r\n";
		
		return -1;
	}
	getsockopt(m_sockfd,SOL_SOCKET,SO_ERROR,(char*)&err_code,(socklen_t*)&err_code_len);
	if(err_code !=0)
	{
		close(m_sockfd);
		m_sockfd = -1;
		err_msg = "system error\r\n";
		return -1;
	}
	
	//printf("connected\n");
	return 0;
}

int FastCGI::Send(const char* buf, unsigned long long len)
{
	//printf("SEND: %d\n", len);
	return m_sockfd >= 0 ? _Send_(m_sockfd, (char*)buf, len) : -1;
}

int FastCGI::Recv(const char* buf, unsigned long long len)
{
	return m_sockfd >= 0 ? _Recv_(m_sockfd, (char*)buf, len) : -1;
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
		//printf("BeginRequest 0\n");
		return 0;
	}
	else
	{
		//printf("BeginRequest -1\n");
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
	
	//printf("contentLength_32bit: %d contentLength_16bit: %d\n", contentLength_32bit, contentLength_16bit);
	if(Send((char*)&fcgi_header, sizeof(FCGI_Header)) >= 0)
	{
		//printf("fcgi_header: %d %d\n", fcgi_header.contentLengthB0, fcgi_header.contentLengthB1);
		for(it = params_map.begin(); it != params_map.end(); ++it)
		{
			
			unsigned int name_len = it->first.length();
			unsigned int value_len = it->second.length();
			if(name_len > 0 && value_len > 0)
			{
				//printf("name_len: %d value_len: %d\n", name_len, value_len);
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
				//printf("%s = [%s]\n", it->first.c_str(), it->second.c_str());
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

int FastCGI::RecvAppData(string &strout, string& strerr, unsigned int & appstatus, unsigned char & protocolstatus)
{
	strout = "";
	strerr = "";
	FCGI_Header fcgi_header;
	bool bQuit = false;
	while(!bQuit)
	{
		char * contentBuffer = NULL;
		char paddingBuffer[255];
		unsigned short contentLength = 0;
		unsigned char paddingLength = 0;
		if(Recv((char*)&fcgi_header, sizeof(FCGI_Header)) >= 0 )
		{
			//printf("%d : %d\n", fcgi_header.contentLengthB1, fcgi_header.contentLengthB0);
			contentLength = fcgi_header.contentLengthB1;
			contentLength <<= 8;
			contentLength += fcgi_header.contentLengthB0;
			
			paddingLength = fcgi_header.paddingLength;
			
			//printf("contentLength: %d paddingLength: %d\n", contentLength, paddingLength);
			if(contentLength > 0)
			{
				contentBuffer  = new char[contentLength + 1];
				if(contentBuffer && Recv(contentBuffer, contentLength) >= 0)
				{
					contentBuffer[contentLength] = '\0';
				    //printf("%d - %s\n", contentLength, contentBuffer);
				}
				else
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
			//printf("fcgi_header.type: %d\n",fcgi_header.type);
			switch(fcgi_header.type)
			{
				case FCGI_STDOUT:
					if(contentLength > 0)
					{
						strout += contentBuffer;
						//printf("%s\n", contentBuffer);
					
					}
				break;
				case FCGI_STDERR:
					if(contentLength > 0)
						strerr += contentBuffer;
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
						//printf("appstatus: %d protocolstatus: %d\n", appstatus, protocolstatus);
					}
					else
					{
						appstatus = 0;
						protocolstatus = FCGI_UNKNOWN_STATUS;
					}
					bQuit = true;
				break;
				default:
				printf("unknown type: %d\n", fcgi_header.type);
				break;
				
			}
			
			if(contentBuffer)
				delete[] contentBuffer;
			contentBuffer = NULL;			
		}
		else
			return -1;
	}
	return 0;
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