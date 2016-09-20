#include "hpack.h"

int64_t
decode_integer(uint32_t &dst, const uint8_t *buf_start, const uint8_t *buf_end, uint8_t n)
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

int64_t
encode_integer(uint8_t *buf_start, const uint8_t *buf_end, uint32_t value, uint8_t n)
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

hpack::hpack()
{
    
}

hpack::hpack(const HTTP2_Header_Field* field, int len)
{
    parse(field, len);
}

hpack::~hpack()
{
    
}

static const char* index_type_names[] = { "type_indexed", "type_indexing_indexed_name", "type_indexing_new_name",
                "type_without_indexing_indexed_name", "type_without_indexing_new_name", "type_never_indexed_indexed_name",
                "type_never_indexed_new_name", "type_error"};
            
            
void hpack::parse(const HTTP2_Header_Field* field, int len)
{
    m_field = field;
    m_len = len;
    int parsed = 0;
    
    while(parsed < m_len)
    {
        HTTP2_Header header;
        header.name = "";
        header.value = "";
        header.index_type = type_error;
        HTTP2_Header_Field* curret_field = (HTTP2_Header_Field*)((char*)m_field + parsed);
        parsed += sizeof(curret_field->tag); //tag
        if(curret_field->tag.indexed.code == 1) /* 1xxx xxxx */
        {
            header.index_type = type_indexed;
            header.index = curret_field->tag.indexed.index;
            header.name = "";
            header.value = "";
            m_decoded_headers.push_back(header);
            //printf("%s %u [%s:%s]\n", index_type_names[header.index_type], header.index, header.name.c_str(), header.value.c_str());
        }
        else if(curret_field->tag.indexing.code == 1 /* 01xx xxxx */
             || curret_field->tag.without_indexing.code == 0  /* 0000 xxxx */
             || curret_field->tag.never_indexed.code == 1  /* 0001 xxxx */)
        {
            if(curret_field->tag.indexing.code == 1)
            {
                header.index_type = curret_field->tag.indexing.index > 0 ? type_indexing_indexed_name : type_indexing_new_name;
                //printf("    indexing %d\n", curret_field->tag.indexing.index);
                header.index = curret_field->tag.indexing.index;
            }
            else if(curret_field->tag.without_indexing.code == 0)
            {
                header.index_type = curret_field->tag.without_indexing.index > 0 ? type_without_indexing_indexed_name : type_without_indexing_new_name;
                //printf("    without_indexing %d\n", curret_field->tag.without_indexing.index);
                header.index = curret_field->tag.without_indexing.index;
            }
            else if(curret_field->tag.never_indexed.code == 1)
            {
                header.index_type = curret_field->tag.never_indexed.index > 0 ? type_never_indexed_indexed_name : type_never_indexed_new_name;
                //printf("    never_indexed %d\n", curret_field->tag.never_indexed.index);
                header.index = curret_field->tag.never_indexed.index;
            }
            else
            {
                printf("! wrong index type\n");
                break;
            }
            
            HTTP2_Header_String* header_string = (HTTP2_Header_String*)((char*)m_field + parsed);
            
            int string_len = 0;
            char* string_ptr = NULL;
            int integer_len = decode_http2_header_string(header_string, &string_len, &string_ptr);
            
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
            
                parsed += integer_len2 + string_len2;
                
                //printf("STRING 3: %d H: %d\n", string_len2, header_string2->h);
                
                if(header_string2->h == 1) //Huffman
                {
                    NODE* h_node;
                    hf_init(&h_node);
                    char* out_buff = (char*)malloc(MAX_HUFFMAN_BUFF_LEN(string_len2));
                    memset(out_buff, 0, MAX_HUFFMAN_BUFF_LEN(string_len2));
                    int out_size = hf_string_decode(h_node, (unsigned char*)string_ptr2, string_len2, out_buff, MAX_HUFFMAN_BUFF_LEN(string_len2));
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
        else
        {
            break;
        }
    }
}