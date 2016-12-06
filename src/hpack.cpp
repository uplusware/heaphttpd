#include "hpack.h"

#define PRE_FIELDS_BUF_SIZE 1024*4

int64_t decode_integer(uint32_t &dst, const uint8_t *buf_start, const uint8_t *buf_end, uint8_t n)
{
  if (buf_start >= buf_end) {
    return -1;
  }

  const uint8_t *p = buf_start;

  dst = (*p & ((1 << n) - 1));
  if (dst == static_cast<uint32_t>(1 << n) - 1) {
    int m = 0;
    do {
      if (++p >= buf_end)
        return -1;

      uint32_t added_value = *p & 0x7f;
      if ((UINT32_MAX >> m) < added_value) {
        // Excessively large integer encodings - in value or octet
        // length - MUST be treated as a decoding error.
        return -1;
      }
      dst += added_value << m;
      m += 7;
    } while (*p & 0x80);
  }

  return p - buf_start + 1;
}

int64_t encode_integer(uint8_t *buf_start, const uint8_t *buf_end, uint32_t value, uint8_t n)
{
  if (buf_start >= buf_end)
    return -1;

  uint8_t *p = buf_start;

  if (value < (static_cast<uint32_t>(1 << n) - 1)) {
    *(p++) = value;
  } else {
    *(p++) = (1 << n) - 1;
    value -= (1 << n) - 1;
    while (value >= 128) {
      if (p >= buf_end) {
        return -1;
      }
      *(p++) = (value & 0x7F) | 0x80;
      value = value >> 7;
    }
    if (p + 1 >= buf_end) {
      return -1;
    }
    *(p++) = value;
  }
  return p - buf_start;
}

int encode_http2_header_string(HTTP2_Header_String* header_string_buf, int length, char** string_ptr)
{
    int integer_len = encode_integer((uint8_t *)header_string_buf, (uint8_t *)header_string_buf + MAX_BYTES_OF_LENGTH, length, 7);
    *string_ptr = integer_len < 0 ? NULL : (char*)header_string_buf + integer_len;
    
    return integer_len;
}

int decode_http2_header_string(HTTP2_Header_String* header_string_buf, int* length, char** string_ptr)
{
    
    uint32_t dst = 0;
    int integer_len = decode_integer(dst, (uint8_t *)header_string_buf, (uint8_t *)header_string_buf + MAX_BYTES_OF_LENGTH, 7);
    *length = dst;
    *string_ptr = integer_len < 0 ? NULL : (char*)header_string_buf + integer_len;
    
    return integer_len;
}

int encode_http2_header_index(char* header_index_buf, int prefix, int length)
{
    int integer_len = encode_integer((uint8_t *)header_index_buf, (uint8_t *)header_index_buf + MAX_BYTES_OF_LENGTH, length, prefix);
    return integer_len;
}

int decode_http2_header_index(char* header_index_buf, int prefix, int* length)
{
    
    uint32_t dst = 0;
    int integer_len = decode_integer(dst, (uint8_t *)header_index_buf, (uint8_t *)header_index_buf + MAX_BYTES_OF_LENGTH, prefix);
    *length = dst;
    
    return integer_len;
}

hpack::hpack()
{
    m_field = NULL;
    m_len = 0;
    m_need_free = FALSE;
}

hpack::hpack(HTTP2_Header_Field* field, int len)
{
    parse(field, len);
}

hpack::~hpack()
{
    if(m_need_free && m_field)
    {
        free(m_field);
        //printf("******** FREE %p\n", m_field);
    }
}

static const char* index_type_names[] = {
    "type_indexed",
    "type_with_indexing_indexed_name",
    "type_with_indexing_new_name",
    "type_without_indexing_indexed_name",
    "type_without_indexing_new_name",
    "type_never_indexed_indexed_name",
    "type_never_indexed_new_name",
    "type_error_index"
};

HTTP2_Header_Field* hpack::get_field()
{
    return m_field;
}

int hpack::get_length()
{
    return m_len;
}   
  
int hpack::build(const char* http1_headrs, int len, map<int, pair<string, string> > & header_static_table)
{
    char* fields_buf = (char*)malloc(PRE_FIELDS_BUF_SIZE);
    uint_32 response_len = 0;
    uint_32 fields_buff_size = PRE_FIELDS_BUF_SIZE;
    
    string line_text = http1_headrs;
    std::size_t new_line;
    while((new_line = line_text.find('\n')) != std::string::npos)
    {
        string strtext;
        
        strtext = line_text.substr(0, new_line + 1);
        line_text = line_text.substr(new_line + 1);

        strtrim(strtext);
        
        // Skip empty line
        if(strtext == "")
            continue;
        
        HTTP2_Header_Field field;
        
        int hdr_field_tag_len = 0;
        HTTP2_Header_String* hdr_string = NULL;
        uint_32 hdr_string_len = 0;
        
        index_type_e index_type = type_never_indexed_new_name;
        
        hdr_field_tag_len = encode_http2_header_index((char*)&(field.tag.type), 4, 0);
        if(hdr_field_tag_len < 0)
            break;
        field.tag.never_indexed.code = 1;
                    
        if(strncasecmp(strtext.c_str(), "HTTP/1.1", 8) == 0)
        {
            string strtextlow;
            lowercase(strtext.c_str(), strtextlow);
            char status_code[16];
            memset(status_code, 0, 16);
            sscanf(strtextlow.c_str(), "http/1.1%*[^0-9]%[0-9]%*[^0-9]", status_code);
#ifdef _http2_debug_            
            printf("  :status %s\n", status_code);
#endif /* _http2_debug_ */            
                    
            for(map<int, pair<string, string> >::iterator it = header_static_table.begin(); it != header_static_table.end(); ++it)
            {
                if(strcasecmp(it->second.first.c_str(), ":status") == 0)
                {
                    index_type = type_with_indexing_indexed_name;
                    
                    hdr_field_tag_len = encode_http2_header_index((char*)&(field.tag.type), 6, it->first);
                    if(hdr_field_tag_len < 0)
                        break;
                    field.tag.with_indexing.code = 1;
                    
                    if(strcasecmp(it->second.second.c_str(), status_code) == 0)
                    {
                        index_type = type_indexed;
                        hdr_field_tag_len = encode_http2_header_index((char*)&(field.tag.type), 7, it->first);
                        if(hdr_field_tag_len < 0)
                            break;
                        field.tag.indexed.code = 1;
                        
                        break;
                    }
                }
            }
            if(index_type == type_with_indexing_indexed_name)
            {                
                const char* string_buf = status_code;
                int string_len = strlen(status_code);
                
                hdr_string = (HTTP2_Header_String*)malloc(sizeof(HTTP2_Header_String) + MAX_BYTES_OF_LENGTH + MAX_HUFFMAN_BUFF_LEN(string_len));
                
                int out_len = 0;
                unsigned char* out_buff = (unsigned char*)malloc(MAX_HUFFMAN_BUFF_LEN(string_len));
                memset(out_buff, 0, MAX_HUFFMAN_BUFF_LEN(string_len));
                
                NODE* h_node;
                hf_init(&h_node);
                hf_string_encode(string_buf, string_len, 0, out_buff, &out_len);                
                hf_finish(h_node);

                char* string_ptr = NULL;
                int integer_len = encode_http2_header_string(hdr_string, out_len, &string_ptr);
                
                hdr_string->h = 1;
                
                memcpy(string_ptr, out_buff, out_len);
                
                free(out_buff);
                
                hdr_string_len = integer_len + out_len;
            }
        }
        else
        {
            string strName, strValue;
            if(strtext[0] == ':')
            {
                strcut(strtext.c_str() + 1, NULL, ":", strName);
                strcut(strtext.c_str() + 1, ":", NULL, strValue);
                strName = ":" + strName;
            }
            else
            {
                strcut(strtext.c_str(), NULL, ":", strName);
                strcut(strtext.c_str(), ":", NULL, strValue);
            }
            strtrim(strName);
            strtrim(strValue);
            
            /* printf("%s | %s\n", strName.c_str(), strValue.c_str()); */
            string strNameLow;
            lowercase(strName.c_str(), strNameLow);

            for(map<int, pair<string, string> >::iterator it = header_static_table.begin(); it != header_static_table.end(); ++it)
            {
                if(strcasecmp(it->second.first.c_str(), strNameLow.c_str()) == 0)
                {
                    index_type = type_with_indexing_indexed_name;
                    
                    hdr_field_tag_len = encode_http2_header_index((char*)&(field.tag.type), 6, it->first);
                    if(hdr_field_tag_len < 0)
                        break;
                    field.tag.with_indexing.code = 1;
                    if(strcasecmp(it->second.second.c_str(), strValue.c_str()) == 0)
                    {
                        index_type = type_indexed;
                        hdr_field_tag_len = encode_http2_header_index((char*)&(field.tag.type), 7, it->first);
                        if(hdr_field_tag_len < 0)
                            break;
                        field.tag.indexed.code = 1;
                        break;
                    }
                    
                }
            }
            
            if(index_type == type_with_indexing_indexed_name)
            {
                const char*  string_buf = strValue.c_str();
                int string_len = strValue.length();

                hdr_string = (HTTP2_Header_String*)malloc(sizeof(HTTP2_Header_String) + MAX_BYTES_OF_LENGTH + MAX_HUFFMAN_BUFF_LEN(string_len));
                
                int out_len = 0;
                unsigned char* out_buff = (unsigned char*)malloc(MAX_HUFFMAN_BUFF_LEN(string_len));
                memset(out_buff, 0, MAX_HUFFMAN_BUFF_LEN(string_len));
                
                NODE* h_node;
                hf_init(&h_node);
                hf_string_encode(string_buf, string_len, 0, out_buff, &out_len);                
                hf_finish(h_node);

                char* string_ptr = NULL;
                int integer_len = encode_http2_header_string(hdr_string, out_len, &string_ptr);
                
                hdr_string->h = 1;
                
                memcpy(string_ptr, out_buff, out_len);
                
                free(out_buff);
                
                hdr_string_len = integer_len + out_len;
            }
            else if(index_type == type_never_indexed_new_name)
            {
                const char*  string_buf = strNameLow.c_str();
                int string_len = strNameLow.length();
                 
                const char*  string_buf2 = strValue.c_str();
                int string_len2 = strValue.length();
                
                hdr_string = (HTTP2_Header_String*)malloc(sizeof(HTTP2_Header_String) + MAX_BYTES_OF_LENGTH + MAX_HUFFMAN_BUFF_LEN(string_len) + MAX_HUFFMAN_BUFF_LEN(string_len2));
                
                int out_len = 0;
                unsigned char* out_buff = (unsigned char*)malloc(MAX_HUFFMAN_BUFF_LEN(string_len));
                memset(out_buff, 0, MAX_HUFFMAN_BUFF_LEN(string_len));
                
                NODE* h_node;
                hf_init(&h_node);
                hf_string_encode(string_buf, string_len, 0, out_buff, &out_len);                
                hf_finish(h_node);

                char* string_ptr = NULL;
                int integer_len = encode_http2_header_string(hdr_string, out_len, &string_ptr);
                
                hdr_string->h = 1;
                
                memcpy(string_ptr, out_buff, out_len);
                
                free(out_buff);
                
                hdr_string_len = integer_len + out_len;

                
                HTTP2_Header_String* hdr_string2 = (HTTP2_Header_String*)((char*)hdr_string + integer_len + out_len);
                
                int out_len2 = 0;
                unsigned char* out_buff2 = (unsigned char*)malloc(MAX_HUFFMAN_BUFF_LEN(string_len2));
                memset(out_buff2, 0, MAX_HUFFMAN_BUFF_LEN(string_len2));
                
                NODE* h_node2;
                hf_init(&h_node2);
                hf_string_encode(string_buf2, string_len2, 0, out_buff2, &out_len2);                
                hf_finish(h_node2);

                char* string_ptr2 = NULL;
                int integer_len2 = encode_http2_header_string(hdr_string2, out_len2, &string_ptr2);
                
                hdr_string2->h = 1;
                
                memcpy(string_ptr2, out_buff2, out_len2);
                
                free(out_buff2);
                
                hdr_string_len += integer_len2 + out_len2;
            }
        }
        
        if(fields_buff_size < response_len + hdr_field_tag_len + hdr_string_len)
        {
            fields_buff_size = response_len + hdr_field_tag_len + hdr_string_len + PRE_FIELDS_BUF_SIZE;
            char* fields_buf_swap = (char*)malloc(fields_buff_size);
            
            memcpy(fields_buf_swap, fields_buf, response_len);
            free(fields_buf);
            fields_buf = fields_buf_swap;
        }
        memcpy(fields_buf + response_len, &field, hdr_field_tag_len);
        response_len += hdr_field_tag_len;
        if(hdr_string && hdr_string_len > 0)
        {
            memcpy(fields_buf + response_len, hdr_string, hdr_string_len);
            response_len += hdr_string_len;
        }
        if(hdr_string)
            free(hdr_string);
    }
    
    m_field = (HTTP2_Header_Field*)fields_buf;
    m_len = response_len;
    m_need_free = TRUE;
}   
         
int hpack::parse(HTTP2_Header_Field* field, int len)
{
    m_field = field;
    m_need_free = FALSE;
    m_len = len;
    int parsed = 0;
    
    while(parsed < m_len)
    {
        HTTP2_Header header;
        header.name = "";
        header.value = "";
        header.index_type = type_error;
        HTTP2_Header_Field* curret_field = (HTTP2_Header_Field*)((char*)m_field + parsed);
        
        if(curret_field->tag.indexed.code == 1) /* 1xxx xxxx */
        {
            int f_index = 0;
            char** header_string_ptr = NULL;
            int integer_len = decode_http2_header_index((char*)&(curret_field->tag.type), 7,  &f_index);
            //printf("integer_len: %d\n", integer_len);
            if(integer_len < 0)
                return -1;
            
            parsed += integer_len;
            
            header.index_type = type_indexed;
            //printf("    indexed %d\n", f_index);
            header.index = f_index;
            header.name = "";
            header.value = "";
            m_decoded_headers.push_back(header);
            //printf("%s %u [%s:%s]\n", index_type_names[header.index_type], header.index, header.name.c_str(), header.value.c_str());
        }
        else
        {
            if(curret_field->tag.with_indexing.code == 1)
            {
                int f_index = 0;
                char** header_string_ptr = NULL;
                int integer_len = decode_http2_header_index((char*)&(curret_field->tag.type), 6,  &f_index);
                //printf("integer_len: %d\n", integer_len);
                if(integer_len < 0)
                    return -1;
            
                parsed += integer_len;
            
                header.index_type = curret_field->tag.with_indexing.index > 0 ? type_with_indexing_indexed_name : type_with_indexing_new_name;
                //printf("    with_indexing %d\n", f_index);
                header.index = f_index;//curret_field->tag.with_indexing.index;
            }
            else
            {
                int f_index = 0;
                char** header_string_ptr = NULL;
                int integer_len = decode_http2_header_index((char*)&(curret_field->tag.type), 4,  &f_index);
                //printf("integer_len: %d\n", integer_len);
                if(integer_len < 0)
                    return -1;
            
                parsed += integer_len;
            
                if(curret_field->tag.without_indexing.code == 0)
                {
                    header.index_type = curret_field->tag.without_indexing.index > 0 ? type_without_indexing_indexed_name : type_without_indexing_new_name;
                    //printf("    without_indexing %d\n", curret_field->tag.without_indexing.index);
                    header.index = curret_field->tag.without_indexing.index;
                }
                else if(curret_field->tag.never_indexed.code == 1)
                {
                    header.index_type = curret_field->tag.never_indexed.index > 0 ? type_never_indexed_indexed_name : type_never_indexed_new_name;
                    //printf("    never_indexed %d\n", f_index);
                    header.index = f_index;//curret_field->tag.never_indexed.index;
                }
                else
                {
                    printf("!! wrong index type\n");
                    return -1;
                }
            }
            
            HTTP2_Header_String* header_string = (HTTP2_Header_String*)((char*)m_field + parsed);
            
            int string_len = 0;
            char* string_ptr = NULL;
            int integer_len = decode_http2_header_string(header_string, &string_len, &string_ptr);
            if(integer_len < 0)
                return -1;
            parsed += integer_len + string_len;
            
            if(header.index > 0)
            {
                //printf("STRING 1: I-LEN: %d, LEN: %d, H: %d\n", integer_len, string_len, header_string->h);
                if(header_string->h == 1) //Huffman
                {
                    NODE* h_node;
                    hf_init(&h_node);
                    char* out_buff = (char*)malloc(MAX_HUFFMAN_BUFF_LEN(string_len));
                    memset(out_buff, 0, MAX_HUFFMAN_BUFF_LEN(string_len));
                    int out_size = hf_string_decode(h_node, (unsigned char*)string_ptr, string_len , out_buff, MAX_HUFFMAN_BUFF_LEN(string_len));
                    if(out_size < 0)
                    {
                        free(out_buff);
                        hf_finish(h_node);
                        return -1;
                    }
                    out_buff[out_size] = '\0';
                    
                    //printf("out_buff: %s\n", out_buff);
                    
                    header.name = "";
                    header.value = out_buff;
                    
                    free(out_buff);
                    
                    hf_finish(h_node);
            
                }
                else
                {
                    char* out_buff = (char*)malloc(string_len + 1);
                    memcpy(out_buff, string_ptr, string_len + 1);
                    out_buff[string_len] = '\0';
                    header.name = "";
                    header.value = out_buff;
                    free(out_buff);
                }
            }
            else if(header.index == 0)
            {
                //printf("STRING 2: %d H: %d\n", string_len, header_string->h);
                
                if(header_string->h == 1) //Huffman
                {
                    NODE* h_node;
                    hf_init(&h_node);
                    char* out_buff = (char*)malloc(MAX_HUFFMAN_BUFF_LEN(string_len));
                    memset(out_buff, 0, MAX_HUFFMAN_BUFF_LEN(string_len));
                    int out_size = hf_string_decode(h_node, (unsigned char*)string_ptr, string_len , out_buff, MAX_HUFFMAN_BUFF_LEN(string_len));
                    if(out_size < 0)
                    {
                        free(out_buff);
                        hf_finish(h_node);
                        return -1;
                    }
                    out_buff[out_size] = '\0';
                    
                    //printf("out_buff: %s\n", out_buff);
                    
                    header.name = out_buff;
                    free(out_buff);
                    //printf("------    %s\n", header.name.c_str());
                    hf_finish(h_node);
            
                }
                else
                {
                    char* out_buff = (char*)malloc(string_len + 1);
                    memcpy(out_buff, string_ptr, string_len + 1);
                    out_buff[string_len] = '\0';
                    
                    header.name = out_buff;
                    free(out_buff);
                    
                    //printf("------    %s\n", header.value.c_str());                    
                }
                
                HTTP2_Header_String* header_string2 = (HTTP2_Header_String*)((char*)m_field + parsed);
                
                int string_len2 = 0;
                char* string_ptr2 = NULL;
                int integer_len2 = decode_http2_header_string(header_string2, &string_len2, &string_ptr2);
                if(integer_len2 < 0)
                    return -1;
                parsed += integer_len2 + string_len2;
                
                //printf("STRING 3: %d H: %d\n", string_len2, header_string2->h);
                
                if(header_string2->h == 1) //Huffman
                {
                    NODE* h_node;
                    hf_init(&h_node);
                    char* out_buff = (char*)malloc(MAX_HUFFMAN_BUFF_LEN(string_len2));
                    memset(out_buff, 0, MAX_HUFFMAN_BUFF_LEN(string_len2));
                    int out_size = hf_string_decode(h_node, (unsigned char*)string_ptr2, string_len2, out_buff, MAX_HUFFMAN_BUFF_LEN(string_len2));
                    if(out_size < 0)
                    {
                        free(out_buff);
                        hf_finish(h_node);
                        return -1;
                    }
                    out_buff[out_size] = '\0';
                    
                    //printf("out_buff: %s\n", out_buff);
                    
                    header.value = out_buff;
                    free(out_buff);
                    //printf("    %s\n", header.value.c_str());
                    hf_finish(h_node);
                }
                else
                {
                    char* out_buff = (char*)malloc(string_len2 + 1);
                    memcpy(out_buff, string_ptr2, string_len2 + 1);
                    out_buff[string_len2] = '\0';
                    
                    header.value = out_buff;
                    free(out_buff);
                    //printf("    %s\n", header.value.c_str());                    
                }
            }
            
            //printf("%s %u [%s:%s]\n", index_type_names[header.index_type], header.index, header.name.c_str(), header.value.c_str());
            m_decoded_headers.push_back(header);
        }
    }
    return 0;
}