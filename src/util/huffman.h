#ifndef __HUFFMAN_H
#define __HUFFMAN_H

enum HM_RETURN{
    HM_RETURN_SUCCESS     = 0,
    HM_RETURN_UNIMPLEMENT = -100,
};

struct node{
	struct node *children[256];
	unsigned char sym;
	unsigned int code;
	int code_len;
	int size;
};

typedef struct node NODE;
extern NODE *ROOT;

int hf_init(NODE** h_node);
void hf_finish(NODE* h_node);
int hf_byte_encode(unsigned char ch, int remain, unsigned char *buff);
int hf_integer_encode(unsigned int enc_binary,int nprefix, unsigned char *buff);
int hf_integer_decode(char *enc_buff, int nprefix , char *dec_buff);
int hf_string_encode(char *buff_in, int size, int prefix, unsigned char *buff_out, int *size_out);
int hf_string_decode(NODE* h_node, unsigned char *enc, int enc_sz, char *out_buff, int out_sz);
void hf_print_hex(unsigned char *buff, int size);
int hf_string_encode_len(unsigned char *enc, int enc_sz);

#endif
