#include "hpack.h"

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
                printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
                break;
            }
            
            HTTP2_Header_String* header_string = (HTTP2_Header_String*)((char*)m_field + parsed);
            parsed += sizeof(HTTP2_Header_String) + header_string->len;
            if(header.index > 0)
            {
                printf("STRING 1: LEN: %d H: %d\n", header_string->len, header_string->h);
                if(header_string->h == 1) //Huffman
                {
                    NODE* h_node;
                    hf_init(&h_node);
                    char* out_buff = (char*)malloc(header_string->len * 32 + 1);
                    memset(out_buff, 0, header_string->len * 32 + 1);
                    int size = hf_string_decode(h_node, (unsigned char*)header_string->str, header_string->len , out_buff, header_string->len * 32);
                    out_buff[size] = '\0';
                    
                    //printf("out_buff: %s\n", out_buff);
                    header.name = "";
                    header.value = out_buff;
                    free(out_buff);
                    //printf("~~~~~~~~~    %s\n", header.value.c_str());
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
                    //printf("~~~~~~~~~    %s\n", header.value.c_str());               
                }
            }
            else if(header.index == 0)
            {
                printf("STRING 2: %d H: %d\n", header_string->len, header_string->h);
                
                if(header_string->h == 1) //Huffman
                {
                    NODE* h_node;
                    hf_init(&h_node);
                    
                    char* out_buff = (char*)malloc(header_string->len * 32 + 1);
                    memset(out_buff, 0, header_string->len * 32 + 1);
                    int size = hf_string_decode(h_node, (unsigned char*)header_string->str, header_string->len , out_buff, header_string->len * 32);
                    out_buff[size] = '\0';
                    header.name = out_buff;
                    free(out_buff);
                    //printf("------    %s\n", header.name.c_str());
                    hf_finish(h_node);
            
                }
                else
                {
                    char* out_buff = (char*)malloc(header_string->len + 1);
                    memcpy(out_buff, header_string->str, header_string->len);
                    out_buff[header_string->len] = '\0';
                    header.name = out_buff;
                    free(out_buff);
                    //printf("------    %s\n", header.value.c_str());                    
                }
                
                HTTP2_Header_String* header_string2 = (HTTP2_Header_String*)((char*)m_field + parsed);
                parsed += sizeof(HTTP2_Header_String) + header_string2->len;
                
                printf("STRING 3: %d H: %d\n", header_string2->len, header_string2->h);
                
                if(header_string2->h == 1) //Huffman
                {
                    NODE* h_node;
                    hf_init(&h_node);
                    char* out_buff = (char*)malloc(header_string2->len * 8 + 1);
                    memset(out_buff, 0, header_string2->len * 8 + 1);
                    int size = hf_string_decode(h_node, (unsigned char*)header_string2->str, header_string2->len , out_buff, header_string2->len * 8);
                    out_buff[size] = '\0';
                    header.value = out_buff;
                    free(out_buff);
                    //printf("    %s\n", header.value.c_str());
                    hf_finish(h_node);
                }
                else
                {
                    char* out_buff = (char*)malloc(header_string2->len + 1);
                    memcpy(out_buff, header_string2->str, header_string2->len);
                    out_buff[header_string2->len] = '\0';
                    header.value = out_buff;
                    free(out_buff);
                    //printf("    %s\n", header.value.c_str());                    
                }
            }
            
            printf("%s %u [%s:%s]\n", index_type_names[header.index_type], header.index, header.name.c_str(), header.value.c_str());
            m_decoded_headers.push_back(header);
            
            
        }
        else
        {
            break;
        }
        printf("%d %d\n", parsed, m_len);
    }
}