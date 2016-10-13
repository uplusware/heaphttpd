/*
	Copyright (c) openheap, uplusware
	uplusware@gmail.com
*/
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <endian.h>
#include <time.h>
#include "websocket.h"
#include "util/general.h"

int WebSocket_Buffer_Alloc(WebSocket_Buffer * data, int len, bool mask, int opcode)
{
	data->buf = len == 0 ? NULL : new char[len];
	data->len = data->buf ? len : 0;
    data->mask = mask;
    data->opcode = opcode;
}

int WebSocket_Buffer_Free(WebSocket_Buffer * data)
{
	if(data->buf == NULL)
	{
		delete[] data->buf;
		data->buf = NULL;
	}
	data->len =0;
}

int WebSocket::Send(WebSocket_Buffer * data)
{
	int res = -1;
	WebSocket_Base_Framing websocket_framing;
	websocket_framing.base_hdr.h.hstruct.fin = 1;
	websocket_framing.base_hdr.h.hstruct.rsv1 = 0;
	websocket_framing.base_hdr.h.hstruct.rsv2 = 0;
	websocket_framing.base_hdr.h.hstruct.rsv3 = 0;
	websocket_framing.base_hdr.h.hstruct.opcode = data->opcode;
	websocket_framing.base_hdr.h.hstruct.mask = data->mask == true ? 1 : 0;
    
	if(data->len > 65535) //64bit length
		websocket_framing.base_hdr.h.hstruct.payload_len_7bit = 127;
	else if(data->len > 125 && data->len < 65535) // 16bit length
		websocket_framing.base_hdr.h.hstruct.payload_len_7bit = 126;
	else
		websocket_framing.base_hdr.h.hstruct.payload_len_7bit = data->len;
	
	unsigned short hunit16 = htons(websocket_framing.base_hdr.h.huint16);
    
    if(m_ssl)
        res = SSLWrite(m_sockfd, m_ssl, (char*)&hunit16, sizeof(unsigned short));
	else
        res = _Send_(m_sockfd, (char*)&hunit16, sizeof(unsigned short));
	if(res == -1)
	{
		return -1;
	}
	
	if(websocket_framing.base_hdr.h.hstruct.payload_len_7bit == 126)
	{
		unsigned short len16bit = data->len;
		len16bit = htons(len16bit);
        if(m_ssl)
            res = SSLWrite(m_sockfd, m_ssl, (char*)&len16bit, sizeof(unsigned short));
        else
            res = _Send_(m_sockfd, (char*)&len16bit, sizeof(unsigned short));
		if(res == -1)
		{
			return -1;
		}
	}
	else if(websocket_framing.base_hdr.h.hstruct.payload_len_7bit == 127)
	{
		unsigned long long len64bit = data->len;
		
		len64bit = htobe64(len64bit);
        if(m_ssl)
            res = SSLWrite(m_sockfd, m_ssl, (char*)&len64bit, sizeof(unsigned long long));
        else
            res = _Send_(m_sockfd, (char*)&len64bit, sizeof(unsigned long long));
		if(res == -1)
		{
			return -1;
		}
	}
	
	websocket_framing.payload_len = data->len;
	websocket_framing.payload_buf = new unsigned char[websocket_framing.payload_len + 1];
	
	if(!websocket_framing.payload_buf)
		return -1;
	
	if(websocket_framing.base_hdr.h.hstruct.mask == 1)
	{
		srandom(time(NULL));
		unsigned int mkey = random();
		memcpy(websocket_framing.masking_key, (char*)&mkey, 4);
	
		for (int i = 0; i < websocket_framing.payload_len && websocket_framing.base_hdr.h.hstruct.mask == 1; i++)
			websocket_framing.payload_buf[i] = data->buf[i]^websocket_framing.masking_key[i%4];
		
        if(m_ssl)
            res = SSLWrite(m_sockfd, m_ssl, (char*)websocket_framing.masking_key, 4);
        else
            res = _Send_(m_sockfd, (char*)websocket_framing.masking_key, 4);
		if(res == -1)
		{
			delete[] websocket_framing.payload_buf;
			return -1;
		}
	}
	else
	{
        int i = 0;
		for (i = 0; i < websocket_framing.payload_len; i++)
			websocket_framing.payload_buf[i] = data->buf[i];
        
        websocket_framing.payload_buf[i] = '\0';
        //printf("%s\n", websocket_framing.payload_buf);
	}
	
	if(!websocket_framing.payload_buf)
	{
		return 0;
	}
	if(m_ssl)
        res = SSLWrite(m_sockfd, m_ssl, (char*)websocket_framing.payload_buf, websocket_framing.payload_len);
    else
        res = _Send_(m_sockfd, (char*)websocket_framing.payload_buf, websocket_framing.payload_len);
	
	delete[] websocket_framing.payload_buf;
	
	return res;
}

int WebSocket::Recv(WebSocket_Buffer * data)
{
	int res = -1;
	WebSocket_Base_Framing websocket_framing;
    if(m_ssl)
        res = SSLRead(m_sockfd, m_ssl, (char*)&websocket_framing.base_hdr, sizeof(WebSocket_Base_Header));
    else
        res = _Recv_(m_sockfd, (char*)&websocket_framing.base_hdr, sizeof(WebSocket_Base_Header));
	if(res == -1)
		return -1;
	
	websocket_framing.base_hdr.h.huint16 = ntohs(websocket_framing.base_hdr.h.huint16);
	
	//printf("%04x %d\n", websocket_framing.base_hdr.h.huint16, websocket_framing.base_hdr.h.hstruct.payload_len_7bit);
	websocket_framing.payload_len = 0;
	if(websocket_framing.base_hdr.h.hstruct.payload_len_7bit < 126)
	{
		websocket_framing.payload_len = websocket_framing.base_hdr.h.hstruct.payload_len_7bit;
	}
	else if(websocket_framing.base_hdr.h.hstruct.payload_len_7bit == 126)
	{
		unsigned short data_len;
        if(m_ssl)
            res = SSLRead(m_sockfd, m_ssl, (char*)&data_len, sizeof(unsigned short));
        else
            res = _Recv_(m_sockfd, (char*)&data_len, sizeof(unsigned short));
		if(res == -1)
			return -1;
		websocket_framing.payload_len = ntohs(data_len); //16 bit	
	}
	else if(websocket_framing.base_hdr.h.hstruct.payload_len_7bit == 127)
	{
		unsigned long long data_len;
        if(m_ssl)
            res = SSLRead(m_sockfd, m_ssl, (char*)&data_len, sizeof(unsigned long long));
        else
            res = _Recv_(m_sockfd, (char*)&data_len, sizeof(unsigned long long));
		if(res == -1)
			return -1;
		websocket_framing.payload_len = be64toh(data_len); //64 bit
	}
	
	//printf("websocket_framing.payload_len = %d\n", websocket_framing.payload_len);
	
	if(websocket_framing.base_hdr.h.hstruct.mask == 1)
	{
        if(m_ssl)
            res = SSLRead(m_sockfd, m_ssl, (char*)websocket_framing.masking_key, 4);
        else
            res = _Recv_(m_sockfd, (char*)websocket_framing.masking_key, 4);
		if(res == -1)
			return -1;
		//printf("websocket_framing.masking_key = %02x%02x%02x%02x\n", websocket_framing.masking_key[0],  websocket_framing.masking_key[1],  websocket_framing.masking_key[2],  websocket_framing.masking_key[3]);
	}
	
    if(websocket_framing.payload_len > 0)
    {
        websocket_framing.payload_buf = new unsigned char[websocket_framing.payload_len + 1];
        if(!websocket_framing.payload_buf)
            return -1;
        
        if(websocket_framing.payload_buf != NULL)
        {
            if(m_ssl)
                res = SSLRead(m_sockfd, m_ssl, (char*)websocket_framing.payload_buf, websocket_framing.payload_len);
            else
                res = _Recv_(m_sockfd, (char*)websocket_framing.payload_buf, websocket_framing.payload_len);
            if(res == -1)
            {
                delete[] websocket_framing.payload_buf;
                return -1;
            }
            websocket_framing.payload_buf[res] = '\0';

            data->buf = (char*)websocket_framing.payload_buf;
            data->len = websocket_framing.payload_len;
            for (int i = 0; i < websocket_framing.payload_len && websocket_framing.base_hdr.h.hstruct.mask == 1; i++)
                websocket_framing.payload_buf[i] ^= websocket_framing.masking_key[i%4];
            
            return res;
        }
    }
	return 0;
}


int WebSocket::Close()
{
	int res = -1;
	WebSocket_Base_Framing websocket_framing;
	websocket_framing.base_hdr.h.hstruct.fin = 1;
	websocket_framing.base_hdr.h.hstruct.rsv1 = 0;
	websocket_framing.base_hdr.h.hstruct.rsv2 = 0;
	websocket_framing.base_hdr.h.hstruct.rsv3 = 0;
	websocket_framing.base_hdr.h.hstruct.opcode = OPCODE_CLOSE;
	websocket_framing.base_hdr.h.hstruct.mask = 0;
    
	websocket_framing.base_hdr.h.hstruct.payload_len_7bit = 0;
	
	unsigned short hunit16 = htons(websocket_framing.base_hdr.h.huint16);
    
    if(m_ssl)
        res = SSLWrite(m_sockfd, m_ssl, (char*)&hunit16, sizeof(unsigned short));
	else
        res = _Send_(m_sockfd, (char*)&hunit16, sizeof(unsigned short));
    
	return res;
}

int WebSocket::Ping()
{
	int res = -1;
	WebSocket_Base_Framing websocket_framing;
	websocket_framing.base_hdr.h.hstruct.fin = 1;
	websocket_framing.base_hdr.h.hstruct.rsv1 = 0;
	websocket_framing.base_hdr.h.hstruct.rsv2 = 0;
	websocket_framing.base_hdr.h.hstruct.rsv3 = 0;
	websocket_framing.base_hdr.h.hstruct.opcode = OPCODE_PING;
	websocket_framing.base_hdr.h.hstruct.mask = 0;
    
	websocket_framing.base_hdr.h.hstruct.payload_len_7bit = 0;
	
	unsigned short hunit16 = htons(websocket_framing.base_hdr.h.huint16);
    
    if(m_ssl)
        res = SSLWrite(m_sockfd, m_ssl, (char*)&hunit16, sizeof(unsigned short));
	else
        res = _Send_(m_sockfd, (char*)&hunit16, sizeof(unsigned short));
    
	return res;
}

int WebSocket::Pong()
{
	int res = -1;
	WebSocket_Base_Framing websocket_framing;
	websocket_framing.base_hdr.h.hstruct.fin = 1;
	websocket_framing.base_hdr.h.hstruct.rsv1 = 0;
	websocket_framing.base_hdr.h.hstruct.rsv2 = 0;
	websocket_framing.base_hdr.h.hstruct.rsv3 = 0;
	websocket_framing.base_hdr.h.hstruct.opcode = OPCODE_PONG;
	websocket_framing.base_hdr.h.hstruct.mask = 0;
    
	websocket_framing.base_hdr.h.hstruct.payload_len_7bit = 0;
	
	unsigned short hunit16 = htons(websocket_framing.base_hdr.h.huint16);
    
    if(m_ssl)
        res = SSLWrite(m_sockfd, m_ssl, (char*)&hunit16, sizeof(unsigned short));
	else
        res = _Send_(m_sockfd, (char*)&hunit16, sizeof(unsigned short));
    
	return res;
}