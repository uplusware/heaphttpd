#include "hpack.h"

hpack::hpack(const HTTP2_Header_Field* field, int len)
{
    m_field = field;
    m_len = len;
    int parsed = 0;
    
    while(parsed < m_len)
    {
        HTTP2_Header header;
        HTTP2_Header_Field* curret_field = (HTTP2_Header_Field*)((char*)m_field + parsed);
        parsed++; //tag
        if(curret_field->tag.indexed.code == 1) /* 1xxx xxxx */
        {
            header.index_type = type_indexed;
            header.index = curret_field->tag.indexed.index;
            header.name = "";
            header.value = "";
            m_decoded_headers.push_back(header);
            printf("    indexed %d\n", curret_field->tag.indexed.index);
        }
        else if(curret_field->tag.indexing.code == 1 /* 01xx xxxx */
             || curret_field->tag.without_indexing.code == 0  /* 0000 xxxx */
             || curret_field->tag.never_indexed.code == 1  /* 0001 xxxx */)
        {
            if(curret_field->tag.indexing.code == 1)
            {
                header.index_type = curret_field->tag.indexing.index > 0 ? type_indexing_indexed_name : type_indexing_new_name;
                printf("    indexing %d\n", curret_field->tag.indexing.index);
                header.index = curret_field->tag.indexing.index;
            }
            else if(curret_field->tag.without_indexing.code == 0)
            {
                header.index_type = curret_field->tag.without_indexing.index > 0 ? type_without_indexing_indexed_name : type_without_indexing_new_name;
                printf("    without_indexing %d\n", curret_field->tag.without_indexing.index);
                header.index = curret_field->tag.without_indexing.index;
            }
            else if(curret_field->tag.never_indexed.code == 1)
            {
                header.index_type = curret_field->tag.never_indexed.index > 0 ? type_never_indexed_indexed_name : type_never_indexed_new_name;
                printf("    never_indexed %d\n", curret_field->tag.never_indexed.index);
                header.index = curret_field->tag.never_indexed.index;
            }
            else
                break;
            
            HTTP2_Header_String* header_string = (HTTP2_Header_String*)((char*)m_field + parsed);
            if(header.index > 0)
            {
                printf("    len: %d H: %d\n", header_string->len, header_string->h);
                parsed += header_string->len + 1;
                if(header_string->h == 1) //Huffman
                {
                    NODE* h_node;
                    hf_init(&h_node);
                    char* out_buff = (char*)malloc(header_string->len * 8 + 1);
                    memset(out_buff, 0, header_string->len * 8 + 1);
                    int size = hf_string_decode(h_node, (unsigned char*)header_string->str, header_string->len , out_buff, header_string->len * 8);
                    out_buff[size] = '\0';
                    header.name = "";
                    header.value = out_buff;
                    free(out_buff);
                    printf("%s\n", header.value.c_str());
                    hf_finish(h_node);
            
                }
                else
                {
                    char* out_buff = (char*)malloc(header_string->len + 1);
                    memcpy(out_buff, header_string->str, header_string->len);
                    out_buff[header_string->len] = '\0';
                    header.name = "";
                    header.value = out_buff;
                    free(out_buff);
                    printf("%s\n", header.value.c_str());
                                      
                }
            }
            else if(header.index == 0)
            {
                printf("    %d H: %d\n", header_string->len, header_string->h);
                parsed += header_string->len + 1;
                
                if(header_string->h == 1) //Huffman
                {
                    NODE* h_node;
                    hf_init(&h_node);
                    
                    char* out_buff = (char*)malloc(header_string->len * 8 + 1);
                    memset(out_buff, 0, header_string->len * 8 + 1);
                    int size = hf_string_decode(h_node, (unsigned char*)header_string->str, header_string->len , out_buff, header_string->len * 8);
                    out_buff[size] = '\0';
                    header.name = out_buff;
                    free(out_buff);
                    printf("    %s\n", header.name.c_str());
                    hf_finish(h_node);
            
                }
                else
                {
                    char* out_buff = (char*)malloc(header_string->len + 1);
                    memcpy(out_buff, header_string->str, header_string->len);
                    out_buff[header_string->len] = '\0';
                    header.name = out_buff;
                    free(out_buff);
                    printf("    %s\n", header.value.c_str());                    
                }
                
                header_string = ( HTTP2_Header_String*)((char*)header_string + header_string->len + 1);
                
                parsed += header_string->len + 1;
                
                if(header_string->h == 1) //Huffman
                {
                    NODE* h_node;
                    hf_init(&h_node);
                    char* out_buff = (char*)malloc(header_string->len * 8 + 1);
                    memset(out_buff, 0, header_string->len * 8 + 1);
                    int size = hf_string_decode(h_node, (unsigned char*)header_string->str, header_string->len , out_buff, header_string->len * 8);
                    out_buff[size] = '\0';
                    header.value = out_buff;
                    free(out_buff);
                    printf("    %s\n", header.value.c_str());
                    hf_finish(h_node);
                }
                else
                {
                    char* out_buff = (char*)malloc(header_string->len + 1);
                    memcpy(out_buff, header_string->str, header_string->len);
                    out_buff[header_string->len] = '\0';
                    header.value = out_buff;
                    free(out_buff);
                    printf("    %s\n", header.value.c_str());                    
                }
            }
            m_decoded_headers.push_back(header);
        }
        else
        {
            break;
        }
    }
}

hpack::~hpack()
{
    
}