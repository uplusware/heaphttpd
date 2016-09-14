/*
	Copyright (c) openheap, uplusware
	uplusware@gmail.com
*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include "util/huffman.h"
#include <time.h>
#include "http2.h"
#include "hpack.h"

#define PRE_MALLOC_SIZE 1024

static void* http2_send_handler(void* arg)
{
	pthread_attr_t tattr;
	size_t size;
	int ret;

	/* getting the stack size */
	ret = pthread_attr_getstacksize(&tattr, &size);

	pthread_exit(0);
}

void CHttp2::ParseHeaders(uint_32 stream_ind, hpack* hdr)
{
    m_curr_stream_ind = stream_ind;
    m_pHttp1->HelloWorld(stream_ind);
    string str_line;
    for(int x = 0; x < hdr->m_decoded_headers.size(); x++)
    {
        if(hdr->m_decoded_headers[x].index_type == type_indexing_new_name)
        {
            m_header_dynamic_table[hdr->m_decoded_headers[x].index] = make_pair(hdr->m_decoded_headers[x].name, hdr->m_decoded_headers[x].value);
        }
        
        printf("################ %u\n", hdr->m_decoded_headers[x].index);
        map<int, pair<string, string> >::iterator it = m_header_static_table.find(hdr->m_decoded_headers[x].index);
        if(it != m_header_static_table.end())
        {
            printf("%u, %s %s [%s] [%s]\n", hdr->m_decoded_headers[x].index_type, 
                it->second.first.c_str(),  it->second.second.c_str(),
                hdr->m_decoded_headers[x].name.c_str(), hdr->m_decoded_headers[x].value.c_str());
                if(strcasecmp(it->second.first.c_str(), ":method") == 0)
                {
                    if(hdr->m_decoded_headers[x].index_type == type_indexed)
                        m_method = it->second.second;
                    else if(hdr->m_decoded_headers[x].index_type == type_indexing_indexed_name
                        || hdr->m_decoded_headers[x].index_type == type_without_indexing_indexed_name
                        || hdr->m_decoded_headers[x].index_type == type_never_indexed_indexed_name)
                        m_method = hdr->m_decoded_headers[x].value.c_str();
                    printf("!!!!!!!!!!!!! %s\n", m_method.c_str());
                    if(m_method != "" && m_path != "")
                    {
                        str_line = m_method;
                        str_line += " ";
                        str_line += m_path;
                        str_line += " HTTP/2\r\n";
                        printf(str_line.c_str());
                        m_pHttp1->LineParse(str_line.c_str());
                    }
                }
                else if(strcasecmp(it->second.first.c_str(), ":scheme") == 0)
                {
                    
                }
                else if(strcasecmp(it->second.first.c_str(), ":path") == 0)
                {
                    if(hdr->m_decoded_headers[x].index_type == type_indexed)
                        m_path = it->second.second;
                    else if(hdr->m_decoded_headers[x].index_type == type_indexing_indexed_name
                        || hdr->m_decoded_headers[x].index_type == type_without_indexing_indexed_name
                        || hdr->m_decoded_headers[x].index_type == type_never_indexed_indexed_name)
                        m_path = hdr->m_decoded_headers[x].value.c_str();
                    
                    printf("!!!!!!!!!!!!! %s\n", m_path.c_str());
                    
                    if(m_method != "" && m_path != "")
                    {
                        str_line = m_method;
                        str_line += " ";
                        str_line += m_path;
                        str_line += " HTTP/2\r\n";
                        printf(str_line.c_str());
                        m_pHttp1->LineParse(str_line.c_str());
                    }
                }
                else if(strcasecmp(it->second.first.c_str(), ":authority") == 0)
                {
                    if(hdr->m_decoded_headers[x].index_type == type_indexed)
                        m_authority = it->second.second;
                    else if(hdr->m_decoded_headers[x].index_type == type_indexing_indexed_name
                        || hdr->m_decoded_headers[x].index_type == type_without_indexing_indexed_name
                        || hdr->m_decoded_headers[x].index_type == type_never_indexed_indexed_name)
                        m_authority = hdr->m_decoded_headers[x].value.c_str();
                }
                else
                {
                    str_line = it->second.first.c_str();
                    str_line += ": ";
                    if(hdr->m_decoded_headers[x].index_type == type_indexed)
                        str_line += it->second.second;
                    else if(hdr->m_decoded_headers[x].index_type == type_indexing_indexed_name
                        || hdr->m_decoded_headers[x].index_type == type_without_indexing_indexed_name
                        || hdr->m_decoded_headers[x].index_type == type_never_indexed_indexed_name)
                        str_line += hdr->m_decoded_headers[x].value.c_str();
                    str_line += "\r\n";
                    m_pHttp1->LineParse(str_line.c_str());
                    printf(str_line.c_str());
                }
        }
        else
        {
            if(strcasecmp(hdr->m_decoded_headers[x].name.c_str(), ":method") == 0)
            {
                m_method = hdr->m_decoded_headers[x].value.c_str();
                
                if(m_method != "" && m_path != "")
                {
                    str_line = m_method;
                    str_line += " ";
                    str_line += m_path;
                    str_line += " HTTP/2\r\n";
                    printf(str_line.c_str());
                    m_pHttp1->LineParse(str_line.c_str());
                }
            }
            else if(strcasecmp(hdr->m_decoded_headers[x].name.c_str(), ":scheme") == 0)
            {
                
            }
            else if(strcasecmp(hdr->m_decoded_headers[x].name.c_str(), ":path") == 0)
            {
                m_path = hdr->m_decoded_headers[x].value.c_str();
                
                if(m_method != "" && m_path != "")
                {
                    str_line = m_method;
                    str_line += " ";
                    str_line += m_path;
                    str_line += " HTTP/2\r\n";
                    printf(str_line.c_str());
                    m_pHttp1->LineParse(str_line.c_str());
                }
            }
            else if(strcasecmp(hdr->m_decoded_headers[x].name.c_str(), ":authority") == 0)
            {
                m_authority = hdr->m_decoded_headers[x].value.c_str();
            }
            else
            {
                str_line = hdr->m_decoded_headers[x].name.c_str();
                str_line += ": ";
                str_line += hdr->m_decoded_headers[x].value.c_str();
                str_line += "\r\n";
                m_pHttp1->LineParse(str_line.c_str());
                printf(str_line.c_str());
            }
        }
    }
    str_line = "\r\n";
    m_pHttp1->LineParse(str_line.c_str());
    printf(str_line.c_str());
}

void CHttp2::PopMsg(http2_msg* msg)
{
    pthread_mutex_lock(&m_frame_queue_mutex);    
    memcpy(msg, &(m_frame_queue.front()), sizeof(http2_msg));
    m_frame_queue.pop();
    pthread_mutex_unlock(&m_frame_queue_mutex);
}

void CHttp2::init_header_table()
{
	m_header_static_table[1] = make_pair(string(":authority"), string(""));
    m_header_static_table[2] = make_pair(string(":method"), string("GET"));
	m_header_static_table[3] = make_pair(string(":method"), string("POST"));
	m_header_static_table[4] = make_pair(string(":path"), string("/")); 
	m_header_static_table[5] = make_pair(string(":path"), string("/index.html"));
	m_header_static_table[6] = make_pair(string(":scheme"), string("http"));
	m_header_static_table[7] = make_pair(string(":scheme"), string("https")); 
	m_header_static_table[8] = make_pair(string(":status"), string("200"));
	m_header_static_table[9] = make_pair(string(":status"), string("204"));
	m_header_static_table[10] = make_pair(string(":status"), string("206"));
	m_header_static_table[11] = make_pair(string(":status"), string("304"));
	m_header_static_table[12] = make_pair(string(":status"), string("400"));
	m_header_static_table[13] = make_pair(string(":status"), string("404"));
	m_header_static_table[14] = make_pair(string(":status"), string("500"));
	m_header_static_table[15] = make_pair(string("accept-charset"), string(""));
	m_header_static_table[16] = make_pair(string("accept-encoding"), string("gzip, deflate"));
	m_header_static_table[17] = make_pair(string("accept-language"), string(""));
	m_header_static_table[18] = make_pair(string("accept-ranges"), string(""));
	m_header_static_table[19] = make_pair(string("accept"), string(""));
	m_header_static_table[20] = make_pair(string("access-control-allow-origin"), string(""));
	m_header_static_table[21] = make_pair(string("age"), string(""));
	m_header_static_table[22] = make_pair(string("allow"), string("")); 
	m_header_static_table[23] = make_pair(string("authorization"), string(""));
	m_header_static_table[24] = make_pair(string("cache-control"), string(""));
	m_header_static_table[25] = make_pair(string("content-disposition"), string(""));
	m_header_static_table[26] = make_pair(string("content-encoding"), string(""));
	m_header_static_table[27] = make_pair(string("content-language"), string(""));
	m_header_static_table[28] = make_pair(string("content-length"), string("")); 
	m_header_static_table[29] = make_pair(string("content-location"), string(""));
	m_header_static_table[30] = make_pair(string("content-range"), string(""));
	m_header_static_table[31] = make_pair(string("content-type"), string(""));
	m_header_static_table[32] = make_pair(string("cookie"), string(""));
	m_header_static_table[33] = make_pair(string("date"), string(""));
	m_header_static_table[34] = make_pair(string("etag"), string(""));
	m_header_static_table[35] = make_pair(string("expect"), string(""));
	m_header_static_table[36] = make_pair(string("expires"), string(""));
	m_header_static_table[37] = make_pair(string("from"), string(""));
	m_header_static_table[38] = make_pair(string("host"), string(""));
	m_header_static_table[39] = make_pair(string("if-match"), string("")); 
	m_header_static_table[40] = make_pair(string("if-modified-since"), string("")); 
	m_header_static_table[41] = make_pair(string("if-none-match"), string(""));
	m_header_static_table[42] = make_pair(string("if-range"), string("")); 
	m_header_static_table[43] = make_pair(string("if-unmodified-since"), string(""));
	m_header_static_table[44] = make_pair(string("last-modified"), string(""));
	m_header_static_table[45] = make_pair(string("link"), string(""));
	m_header_static_table[46] = make_pair(string("location"), string("")); 
	m_header_static_table[47] = make_pair(string("max-forwards"), string(""));
	m_header_static_table[48] = make_pair(string("proxy-authenticate"), string(""));
	m_header_static_table[49] = make_pair(string("proxy-authorization"), string(""));
	m_header_static_table[50] = make_pair(string("range"), string("")); 
	m_header_static_table[51] = make_pair(string("referer"), string(""));
	m_header_static_table[52] = make_pair(string("refresh"), string(""));
	m_header_static_table[53] = make_pair(string("retry-after"), string("")); 
	m_header_static_table[54] = make_pair(string("server"), string(""));
	m_header_static_table[55] = make_pair(string("set-cookie"), string(""));
	m_header_static_table[56] = make_pair(string("strict-transport-security"), string(""));
	m_header_static_table[57] = make_pair(string("transfer-encoding"), string("")); 
	m_header_static_table[58] = make_pair(string("user-agent"), string(""));
	m_header_static_table[59] = make_pair(string("vary"), string(""));
	m_header_static_table[60] = make_pair(string("via"), string(""));
	m_header_static_table[61] = make_pair(string("www-authenticate"), string(""));
}

CHttp2::CHttp2(CHttp *pHttp1)
{
    m_curr_stream_ind = 0;
    
    m_path = "";
    m_method = "";
    m_authority = "";
    m_scheme = "";
    
    init_header_table();
    
	m_pHttp1 = pHttp1;
    
    int ret = m_pHttp1->HttpRecv(m_preface, HTTP2_PREFACE_LEN);
	m_preface[HTTP2_PREFACE_LEN] = '\0';
	printf("Client preface: %d [%s]\n", ret,  m_preface);
    
    char * server_preface = (char*)malloc(sizeof(HTTP2_Frame) + sizeof(HTTP2_Setting));
    
	HTTP2_Frame* preface_frame = (HTTP2_Frame*)server_preface;
	preface_frame->length.len3b[0] = 0x00;
    preface_frame->length.len3b[1] = 0x00;
    preface_frame->length.len3b[2] = 0x06; //length is 6
	preface_frame->type = HTTP2_FRAME_TYPE_SETTINGS;
	preface_frame->flags = HTTP2_FRAME_FLAG_UNSET;
	preface_frame->r = HTTP2_FRAME_R_UNSET;
	preface_frame->indentifier = HTTP2_FRAME_INDENTIFIER_WHOLE;
	
    HTTP2_Setting* preface_setting = (HTTP2_Setting*)(server_preface + sizeof(HTTP2_Frame));
    preface_setting->identifier = 0;
    preface_setting->value = 0;
	printf("server_preface: %d\n", GetHttp1()->HttpSend(server_preface, sizeof(HTTP2_Frame) + sizeof(HTTP2_Setting)));
    free(server_preface);
	
	HTTP2_Frame client_setting_frame;
	memset(&client_setting_frame, 0, sizeof(HTTP2_Frame));
	ret = m_pHttp1->HttpRecv((char*)&client_setting_frame, sizeof(HTTP2_Frame));
	if(ret == sizeof(HTTP2_Frame))
	{
		uint_32 len = client_setting_frame.length.len24;
        len = ntohl(len << 8);
        HTTP2_Setting* client_setting = (HTTP2_Setting*)malloc(len);
        ret = m_pHttp1->HttpRecv((char*)client_setting, len);
        for(int x = 0; x < len/sizeof(HTTP2_Setting); x++)
        {
            printf("client_setting(%d): %d %d\n", x, ntohs(client_setting[x].identifier), ntohl(client_setting[x].value));
        }
        free(client_setting);
        
        HTTP2_Frame setting_ack;
        setting_ack.length.len3b[0] = 0x00;
        setting_ack.length.len3b[1] = 0x00;
        setting_ack.length.len3b[2] = 0x00; //length is 0
        setting_ack.type = HTTP2_FRAME_TYPE_SETTINGS;
        setting_ack.flags = HTTP2_FRAME_FLAG_SETTING_ACK;
        setting_ack.r = HTTP2_FRAME_R_UNSET;
        setting_ack.indentifier = client_setting_frame.indentifier;
        
        printf("setting_ack.indentifier: %d\n", setting_ack.indentifier);
        m_pHttp1->HttpSend((const char*)&setting_ack, sizeof(HTTP2_Frame));
                
	}
    
	pthread_mutex_init(&m_frame_queue_mutex, NULL);
    
    /*
    pthread_attr_t send_pthread_attr;
	pthread_attr_init(&send_pthread_attr);
	pthread_attr_setdetachstate (&send_pthread_attr, PTHREAD_CREATE_DETACHED);
    
	if(pthread_create(&m_send_pthread_id, &send_pthread_attr, http2_send_handler, this) != 0)
	{
		printf("CHttp2 pthread_create:");
	}
    else
    {
        m_thread_running = 1;
    }*/
}

void CHttp2::PushMsg(http2_msg_type type, HTTP2_Frame* frame, char* payload)
{
    http2_msg msg;
    msg.msg_type = type;
    msg.frame = frame;
    msg.payload = payload;
    
    pthread_mutex_lock(&m_frame_queue_mutex);    
    m_frame_queue.push(msg);
    pthread_mutex_unlock(&m_frame_queue_mutex);
}

CHttp2::~CHttp2()
{
    pthread_mutex_lock(&m_frame_queue_mutex);
    while (!m_frame_queue.empty())  
    {  
        http2_msg msg = m_frame_queue.front();
        if(msg.msg_type != http2_msg_quit)
        {
            if(msg.frame)
                free(msg.frame);
            if(msg.payload)
                free(msg.payload);
        }
        m_frame_queue.pop();
    }
    pthread_mutex_unlock(&m_frame_queue_mutex);
    
    while(m_thread_running == 1)
    {
        usleep(100);
    }
    pthread_mutex_destroy(&m_frame_queue_mutex);
}

int CHttp2::ProtRecv()
{
    HTTP2_Frame* frame_hdr = (HTTP2_Frame*)malloc(sizeof(HTTP2_Frame));
	memset(frame_hdr, 0, sizeof(HTTP2_Frame));
	
	int ret = m_pHttp1->HttpRecv((char*)frame_hdr, sizeof(HTTP2_Frame));
	if(ret == sizeof(HTTP2_Frame))
	{
		uint_32 len = frame_hdr->length.len24;
        len = ntohl(len << 8);
        
		char* payload = (char*)malloc(len + MAX_PADDING_LEN); /*Maximun pad length*/
        memset(payload, 0, len + MAX_PADDING_LEN);
		ret = m_pHttp1->HttpRecv(payload, len);
        
        // there are more data which needs to be recieved.
        printf("TYPE: %02x, FLAGS: %02x PAYLOAD LEN: %u\n", frame_hdr->type, frame_hdr->flags, len);
        if(frame_hdr->type == HTTP2_FRAME_TYPE_HEADERS && (frame_hdr->flags & HTTP2_FRAME_FLAG_PADDED) == HTTP2_FRAME_FLAG_PADDED)
        {
            HTTP2_Frame_Header* header = (HTTP2_Frame_Header*)payload;
            printf("Frame header has padding(%d)\n", header->pad_length);
            if(header->pad_length > 255)
            {
                goto END_SESSION;
            }
            else if(header->pad_length != 0)
            {
                ret = m_pHttp1->HttpRecv(payload + len, header->pad_length);
            }
        }
        else if(frame_hdr->type == HTTP2_FRAME_TYPE_GOAWAY)
        {
            printf("HTTP2_FRAME_TYPE_GOAWAY\n");
            HTTP2_Frame_Goaway* frm_goaway = (HTTP2_Frame_Goaway*)payload;
            uint_32 last_stream_id = frm_goaway->last_stream_id;
            last_stream_id = htonl(last_stream_id << 31);
            uint_32 error_code = frm_goaway->error_code;
            error_code = htonl(error_code);
            
            printf("GOAWAY: %u %u", last_stream_id, error_code);
            goto END_SESSION;
        }
        
        len = frame_hdr->length.len24;
        len = ntohl(len << 8);
        uint_32 stream_ind = frame_hdr->indentifier;
        stream_ind = ntohl(stream_ind << 1);
		printf("FRAME(%d)[%d]: type: %d\n", ntohl(stream_ind << 1), len, frame_hdr->type);
        if(frame_hdr->type == HTTP2_FRAME_TYPE_PRIORITY)
        {
            HTTP2_Frame_Priority * prority = (HTTP2_Frame_Priority *)payload;
            uint_32 dep = ntohl(prority->dependency << 1);
            printf("    %d, %d %d\n", prority->e, dep, prority->weight);
        }
        else if(frame_hdr->type == HTTP2_FRAME_TYPE_HEADERS)
        {
            uint_32 padding_len = 0;
            uint_32 dep = 0;
            uint_8 weight;
            int offset = 0;
            if( (frame_hdr->flags & HTTP2_FRAME_FLAG_PADDED) == HTTP2_FRAME_FLAG_PADDED)
            {
                HTTP2_Frame_Header1 * header1 = (HTTP2_Frame_Header1* )payload;
                padding_len = header1->pad_length;
                offset += sizeof(HTTP2_Frame_Header1);
            }
            if((frame_hdr->flags & HTTP2_FRAME_FLAG_PRIORITY) == HTTP2_FRAME_FLAG_PRIORITY)
            {
                HTTP2_Frame_Header2 * header2 = (HTTP2_Frame_Header2*)(payload + offset);
                dep = ntohl(header2->dependency << 1);
                weight = header2->weight;
                offset += sizeof(HTTP2_Frame_Header2);
            }
            printf("offset, %d\n", offset);
            HTTP2_Frame_Header3 * header3 = (HTTP2_Frame_Header3*)(payload + offset);
            
            int fragment_len = len - offset - padding_len;
            
			for(int x = 0; x < fragment_len; x++)
            {
                printf("%02x", header3->block_fragment_padding[x]);
            }
            printf("\n    %d, %d %d\n", padding_len, dep, weight);
			
            hpack* http_hdr = new hpack((HTTP2_Header_Field*)header3->block_fragment_padding, fragment_len);

            if((frame_hdr->flags & HTTP2_FRAME_FLAG_END_HEADERS) == HTTP2_FRAME_FLAG_END_HEADERS)
            {
                printf("HTTP2_FRAME_FLAG_END_HEADERS\n");
                ParseHeaders(stream_ind, http_hdr);
            }
            if((frame_hdr->flags & HTTP2_FRAME_FLAG_END_STREAM) == HTTP2_FRAME_FLAG_END_STREAM)
            {
                
            }
            delete http_hdr;
        }
        else if(frame_hdr->type == HTTP2_FRAME_TYPE_SETTINGS)
        {
            printf("%u\n", (frame_hdr->flags));
            HTTP2_Setting* client_setting = (HTTP2_Setting*)payload;
            for(int x = 0; x < len/sizeof(HTTP2_Setting); x++)
            {
                printf("    client_setting(%d): %d %d\n", x, ntohs(client_setting[x].identifier), ntohl(client_setting[x].value));
            }
            if((frame_hdr->flags & HTTP2_FRAME_FLAG_SETTING_ACK) != HTTP2_FRAME_FLAG_SETTING_ACK) //non setting ack
            {
                HTTP2_Frame setting_ack;
                setting_ack.length.len3b[0] = 0x00;
                setting_ack.length.len3b[1] = 0x00;
                setting_ack.length.len3b[2] = 0x00; //length is 0
                setting_ack.type = HTTP2_FRAME_TYPE_SETTINGS;
                setting_ack.flags = HTTP2_FRAME_FLAG_SETTING_ACK;
                setting_ack.r = HTTP2_FRAME_R_UNSET;
                setting_ack.indentifier = frame_hdr->indentifier;
                
                printf("setting_ack.indentifier: %d\n", setting_ack.indentifier);
                GetHttp1()->HttpSend((const char*)&setting_ack, sizeof(HTTP2_Frame));
            }
        }
        else if(frame_hdr->type == HTTP2_FRAME_TYPE_DATA)
        {
            int offset = 0;
            uint_32 padding_len = 0;
            if( (frame_hdr->flags & HTTP2_FRAME_FLAG_PADDED) == HTTP2_FRAME_FLAG_PADDED)
            {
                HTTP2_Frame_Data1 * header1 = (HTTP2_Frame_Data1* )payload;
                padding_len = header1->pad_length;
                offset += sizeof(HTTP2_Frame_Data1);
            }
            else
            {
                HTTP2_Frame_Data2 * header2 = (HTTP2_Frame_Data2* )payload;
                offset += sizeof(HTTP2_Frame_Data2);
            }
        }
        else if(frame_hdr->type == HTTP2_FRAME_TYPE_WINDOW_UPDATE)
        {
            HTTP2_Frame_Window_Update* win_update = (HTTP2_Frame_Window_Update*)payload;
            uint_32 win_size = ntohl(win_update->win_size << 1);
            printf("Win Size: %u\n", win_size);
        }
		return ret;
	}
END_SESSION:	
    return -1;
}

Http_Connection CHttp2::Processing()
{
    while(1)
    {
        int result = ProtRecv();
        if(result < 0)
            return httpClose;
        /*else
            return httpContinue;*/
    }
    return httpClose;
}

int CHttp2::ParseHTTP1Header(const char* buf, int len)
{
    char* response_buf = (char*)malloc(sizeof(HTTP2_Frame) + sizeof(HTTP2_Frame) + PRE_MALLOC_SIZE);
    HTTP2_Frame * http2_frm_hdr = (HTTP2_Frame *)response_buf;
    uint_32 response_len = sizeof(HTTP2_Frame);
    uint_32 response_buff_size = sizeof(HTTP2_Frame) + sizeof(HTTP2_Frame) + PRE_MALLOC_SIZE;
    
    string line_text = buf;
    while(1)
    {
        string strtext;
        std::size_t new_line = line_text.find('\n');
        if( new_line == std::string::npos)
        {
            break;
        }
        else
        {
            strtext = line_text.substr(0, new_line + 1);
            line_text = line_text.substr(new_line + 1);
        }

        strtrim(strtext);
        
        // Skip empty line
        if(strtext == "")
            continue;
        
        HTTP2_Header_Field field;
        
        HTTP2_Header_String* hdr_string = NULL;
        uint_32 hdr_string_len = 0;
        
        index_type_e index_type = type_never_indexed_new_name;
        field.tag.never_indexed.code = 0x01;
        field.tag.never_indexed.index = 0x0;
            
        if(strncasecmp(strtext.c_str(), "HTTP/1.1", 8) == 0)
        {
            string strtextlow;
            lowercase(strtext.c_str(), strtextlow);
            char status_code[16];
            memset(status_code, 0, 16);
            sscanf(strtextlow.c_str(), "http/1.1%*[^0-9]%[0-9]%*[^0-9]", status_code);
            printf(":status %s\n", status_code);
                    
            for(map<int, pair<string, string> >::iterator it = m_header_static_table.begin(); it != m_header_static_table.end(); ++it)
            {
                if(strcasecmp(it->second.first.c_str(), ":status") == 0)
                {
                    index_type = type_indexing_indexed_name;
                    field.tag.indexing.code = 0x01;
                    field.tag.indexing.index = it->first;
                    if(strcasecmp(it->second.second.c_str(), status_code) == 0)
                    {
                        index_type = type_indexed;
                        field.tag.indexed.code = 0x01;
                        field.tag.indexed.index = it->first;
                        break;
                    }
                    
                    printf("%d %s\n", it->first, it->second.first.c_str());
                }
            }
            if(index_type == type_indexing_indexed_name)
            {
                hdr_string_len = sizeof(HTTP2_Header_String) + strlen(status_code);
                hdr_string = (HTTP2_Header_String*)malloc(hdr_string_len);
                hdr_string->h = 0;
                hdr_string->len = hdr_string_len - sizeof(HTTP2_Header_String);
                memcpy((char*)hdr_string + sizeof(HTTP2_Header_String), status_code, hdr_string->len);
            }
        }
        else
        {
            string strName, strValue;
            strcut(strtext.c_str(), NULL, ":", strName);
            strcut(strtext.c_str(), ":", NULL, strValue);
            strtrim(strName);
            strtrim(strValue);
            
            string strNameLow;
            lowercase(strName.c_str(), strNameLow);

            for(map<int, pair<string, string> >::iterator it = m_header_static_table.begin(); it != m_header_static_table.end(); ++it)
            {
                if(strcasecmp(it->second.first.c_str(), strNameLow.c_str()) == 0)
                {
                    index_type = type_indexing_indexed_name;
                    field.tag.indexing.code = 0x01;
                    field.tag.indexing.index = it->first;
                    if(strcasecmp(it->second.second.c_str(), strValue.c_str()) == 0)
                    {
                        index_type = type_indexed;
                        field.tag.indexed.code = 0x01;
                        field.tag.indexed.index = it->first;
                        break;
                    }
                    printf("%d %s\n", it->first, it->second.first.c_str());
                }
            }
            
            if(index_type == type_indexing_indexed_name)
            {
                hdr_string = (HTTP2_Header_String*)malloc(sizeof(HTTP2_Header_String) + strValue.length());
                
                int encoded_len = hf_string_encode_len((unsigned char*)strValue.c_str(), strValue.length());
                if(TRUE/*encoded_len >= strValue.length()*/)
                {
                    hdr_string->h = 0;
                    hdr_string->len = strValue.length();
                    memcpy((char*)hdr_string + sizeof(HTTP2_Header_String), strValue.c_str(), hdr_string->len);
                }
                else
                {
                    hdr_string->h = 1;
                    
                    NODE* h_node;
                    hf_init(&h_node);
                    int string_len = strValue.length();
                    unsigned char* out_buff = (unsigned char*)malloc(string_len);
                    memset(out_buff, 0, string_len);
                    
                    hf_string_encode((char*)strValue.c_str(), strValue.length(), 0, out_buff, &encoded_len);
                    
                    printf("hf_string_encode: %u %u\n", strValue.length(), encoded_len);
                    
                    hdr_string->len = encoded_len;
                    memcpy((char*)hdr_string + sizeof(HTTP2_Header_String), out_buff, hdr_string->len);
                    hf_finish(h_node);
                    
                    free(out_buff);
                }
                hdr_string_len = sizeof(HTTP2_Header_String) + hdr_string->len;
            }
            else if(index_type == type_never_indexed_new_name)
            {
                hdr_string = (HTTP2_Header_String*)malloc(2*sizeof(HTTP2_Header_String) + strNameLow.length() + strValue.length());
                
                int encoded_len = hf_string_encode_len((unsigned char*)strNameLow.c_str(), strNameLow.length());
                if( encoded_len >= strNameLow.length() )
                {
                    hdr_string->h = 0;
                    hdr_string->len = strNameLow.length();
                    memcpy((char*)hdr_string + sizeof(HTTP2_Header_String), strNameLow.c_str(), hdr_string->len);
                }
                else
                {
                    hdr_string->h = 1;
                    
                    NODE* h_node;
                    hf_init(&h_node);
                    int string_len = strNameLow.length();
                    unsigned char* out_buff = (unsigned char*)malloc(string_len);
                    memset(out_buff, 0, string_len);
                    
                    hf_string_encode((char*)strNameLow.c_str(), strNameLow.length(), 0, out_buff, &encoded_len);
                    
                    printf("hf_string_encode: %u %u\n", strNameLow.length(), encoded_len);
                    
                    hdr_string->len = encoded_len;
                    memcpy((char*)hdr_string + sizeof(HTTP2_Header_String), out_buff, hdr_string->len);
                    hf_finish(h_node);
                    
                    free(out_buff);
                }
                
                HTTP2_Header_String* hdr_string2 = (HTTP2_Header_String*)((char*)hdr_string + sizeof(HTTP2_Header_String) + hdr_string->len);
                
                encoded_len = hf_string_encode_len((unsigned char*)strValue.c_str(), strValue.length());
                if( encoded_len >= strValue.length() )
                {
                    hdr_string2->h = 0;
                    hdr_string2->len = strValue.length();
                    memcpy((char*)hdr_string2 + sizeof(HTTP2_Header_String), strValue.c_str(), hdr_string2->len);
                }
                else
                {
                    hdr_string2->h = 1;
                    
                    NODE* h_node;
                    hf_init(&h_node);
                    int string_len = strValue.length();
                    unsigned char* out_buff = (unsigned char*)malloc(string_len);
                    memset(out_buff, 0, string_len);
                    
                    hf_string_encode((char*)strValue.c_str(), strValue.length(), 0, out_buff, &encoded_len);
                    
                    printf("hf_string_encode: %u %u\n", strValue.length(), encoded_len);
                    
                    hdr_string2->len = encoded_len;
                    memcpy((char*)hdr_string2 + sizeof(HTTP2_Header_String), out_buff, hdr_string2->len);
                    hf_finish(h_node);
                    
                    free(out_buff);
                }
                
                hdr_string_len = sizeof(HTTP2_Header_String) + hdr_string->len + hdr_string2->len;
            }
        }
        
        if(response_buff_size < response_len + sizeof(HTTP2_Header_Field) + hdr_string_len)
        {
            char* response_buf_swap = (char*)malloc(response_len + sizeof(HTTP2_Header_Field) + hdr_string_len + PRE_MALLOC_SIZE);
            response_buff_size = response_len + sizeof(HTTP2_Header_Field) + hdr_string_len + PRE_MALLOC_SIZE;
            memcpy(response_buf_swap, response_buf, response_len);
            free(response_buf);
            response_buf = response_buf_swap;
        }
        memcpy(response_buf + response_len, &field, sizeof(HTTP2_Header_Field));
        response_len += sizeof(HTTP2_Header_Field);
        printf("hdr_string_len: %u\n", hdr_string_len);
        if(hdr_string && hdr_string_len > 0)
        {
            memcpy(response_buf + response_len, hdr_string, hdr_string_len);
            response_len += hdr_string_len;
        }
    }
    uint_32 frame_len = response_len - sizeof(HTTP2_Frame);
    printf("frame length: %u, m_curr_stream_ind:　%d\n", frame_len, m_curr_stream_ind);
    
    http2_frm_hdr->length.len24 = htonl(frame_len) >> 8;
    printf("len: %02x, %02x, %02x\n", http2_frm_hdr->length.len3b[0], http2_frm_hdr->length.len3b[1], http2_frm_hdr->length.len3b[2]);
    http2_frm_hdr->type = HTTP2_FRAME_TYPE_HEADERS;
    http2_frm_hdr->flags = HTTP2_FRAME_FLAG_END_HEADERS;
    http2_frm_hdr->r = HTTP2_FRAME_R_UNSET;
    
    http2_frm_hdr->indentifier = htonl(m_curr_stream_ind) >> 1;
    
    printf("#1: %u\n", response_len);
    if(response_buff_size < response_len + sizeof(HTTP2_Frame) + sizeof(HTTP2_Frame_Data2) + 2)
    {
        char* response_buf_swap = (char*)malloc(response_len + sizeof(HTTP2_Frame_Data2) + 2 + PRE_MALLOC_SIZE);
        response_buff_size = response_len + sizeof(HTTP2_Frame_Data2) + 2 + PRE_MALLOC_SIZE;
        memcpy(response_buf_swap, response_buf, response_len);
        free(response_buf);
        response_buf = response_buf_swap;
    }
    
    if(m_pHttp1->GetMethod() == hmHead)
    {
        HTTP2_Frame * http2_frm_data = (HTTP2_Frame *)(response_buf + response_len);
        response_len += sizeof(HTTP2_Frame);
        
        http2_frm_data->length.len24 = 0;//htonl(2) >> 8;
        printf("len: %02x, %02x, %02x\n", http2_frm_data->length.len3b[0], http2_frm_data->length.len3b[1], http2_frm_data->length.len3b[2]);
        http2_frm_data->type = HTTP2_FRAME_TYPE_DATA;
        http2_frm_data->flags = HTTP2_FRAME_FLAG_UNSET;
        http2_frm_data->r = HTTP2_FRAME_R_UNSET;
        http2_frm_data->indentifier = htonl(m_curr_stream_ind) >> 1;
        
        printf("m_curr_stream_ind:　%d\n", m_curr_stream_ind);
        
    }
    
    return m_pHttp1->HttpSend(response_buf, response_len);
}

int CHttp2::ParseHTTP1Content(const char* buf, uint_32 len)
{
    int response_len;
    char* response_buf = (char*)malloc(sizeof(HTTP2_Frame) + sizeof(HTTP2_Frame_Data2) + len);
    response_len = sizeof(HTTP2_Frame) + sizeof(HTTP2_Frame_Data2) + len;
    HTTP2_Frame * http2_frm_data = (HTTP2_Frame *)response_buf;
    http2_frm_data->length.len24 = htonl(len) >> 8;
    printf("len: %02x, %02x, %02x\n", http2_frm_data->length.len3b[0], http2_frm_data->length.len3b[1], http2_frm_data->length.len3b[2]);
    http2_frm_data->type = HTTP2_FRAME_TYPE_DATA;
    http2_frm_data->flags = HTTP2_FRAME_FLAG_UNSET;
    http2_frm_data->r = HTTP2_FRAME_R_UNSET;
    
    http2_frm_data->indentifier = htonl(m_curr_stream_ind) >> 1;
    
    printf("m_curr_stream_ind:　%d\n", m_curr_stream_ind);
    
    HTTP2_Frame_Data2* data = (HTTP2_Frame_Data2*)(response_buf + sizeof(HTTP2_Frame));
    memcpy(data->data_padding, buf, len);
    
    return m_pHttp1->HttpSend(response_buf, response_len);
}

int CHttp2::SendContentEOF()
{
    int response_len;
    char* response_buf = (char*)malloc(sizeof(HTTP2_Frame));
    response_len = sizeof(HTTP2_Frame);
    HTTP2_Frame * http2_frm_data = (HTTP2_Frame *)response_buf;
    http2_frm_data->length.len24 = 0;
    printf("len: %02x, %02x, %02x\n", http2_frm_data->length.len3b[0], http2_frm_data->length.len3b[1], http2_frm_data->length.len3b[2]);
    http2_frm_data->type = HTTP2_FRAME_TYPE_DATA;
    http2_frm_data->flags = HTTP2_FRAME_FLAG_END_STREAM;
    http2_frm_data->r = HTTP2_FRAME_R_UNSET;
    
    http2_frm_data->indentifier = htonl(m_curr_stream_ind) >> 1;
    
    printf("SendContentEOF: m_curr_stream_ind:　%d\n", m_curr_stream_ind);
    
    return m_pHttp1->HttpSend(response_buf, response_len);
}