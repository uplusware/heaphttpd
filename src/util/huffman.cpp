#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "huffman.h"
#include "tables.h"

#define HEX_TO_HF_CODE(hex)		huffman_codes[hex]
#define HEX_TO_HF_CODE_LEN(hex) huffman_code_len[hex]

static NODE * node_create(){
	NODE *nnode = (NODE*)malloc(1*sizeof(NODE));
	memset(nnode, 0, sizeof(NODE));
	return nnode;
}

static int _hf_add_node(NODE* h_node, unsigned char sym, int code, int code_len){
	NODE *cur		= h_node;
	unsigned char i	= 0;
	int j			= 0;
	int shift		= 0;
	int start		= 0;
	int end			= 0;
	
	for( ; code_len > 8 ; ){
		code_len -= 8;
		i = (unsigned char)(code >> code_len);
		if(cur->children[i] == NULL ){
			cur->children[i] = node_create();
		}
		cur = cur->children[i];
	}	

	shift = (8-code_len);
	start = (unsigned char)(code<<shift);
	end	  = (1 << shift);

	for(j = start; j < start+end ;j++){
		if( cur->children[j] == NULL){
			cur->children[j] = node_create();
		}
		cur->children[j]->code = code;
		cur->children[j]->sym = sym;
		cur->children[j]->code_len = code_len;
		cur->size++;
	}

	return 0;
}

int hf_init(NODE** h_node){
	int i   = 0;
	*h_node    = node_create();
	for(i = 0; i < 256; i++){
		_hf_add_node(*h_node, i, huffman_codes[i], huffman_code_len[i]);
	}
	return 0;
}

void hf_finish(NODE* h_node){
	free(h_node);
}

int hf_byte_encode(unsigned char ch, int remain, unsigned char *buff){
	unsigned char t = 0;
	int i			= 0;
	int codes		= HEX_TO_HF_CODE(ch);
	int nbits		= HEX_TO_HF_CODE_LEN(ch);
	//printf("'%c'|codes(%d)|len(%d)\n", ch, codes, nbits );
	for(;;){
		if( remain > nbits){
			t = (unsigned char)(codes << (remain-nbits));
			buff[i++] |= t;
			return remain-nbits;
		}else{
			t = (unsigned char )(codes >> (nbits-remain));
			buff[i++] |= t;
			nbits -= remain;
			remain = 8;
		}
		buff[i] = 0;
		if(nbits == 0) return remain;
	}

}

int hf_string_encode(char *buff_in, int size, int prefix, unsigned char *buff_out, int *size_out){
	int i		= 0;
	int remain	= (8-prefix);
	int j		= 0;		  //j is instead currently index of buff_out and it is size of buff_out after it has been done.
	int nbytes  = 0;

	for(i = 0; i < size; i++){
		
		if( remain > HEX_TO_HF_CODE_LEN((int)buff_in[i]) ){
			nbytes = (remain - HEX_TO_HF_CODE_LEN((int)buff_in[i])) / 8;
		}else{
			nbytes = ((HEX_TO_HF_CODE_LEN((int)buff_in[i]) - remain) / 8)+1;
		}
		remain = hf_byte_encode( buff_in[i], remain, &buff_out[j]);
		j += nbytes;
	}

	// Special EOS sybol
	if( remain < 8 ){
		unsigned int codes = 0x3fffffff;
		int nbits = (char)30;
		buff_out[j++] |= (unsigned char )(codes >> (nbits-remain));
	}

	*size_out = j;
	return 0;
}

int hf_string_decode(NODE* h_node, unsigned char *enc, int enc_sz, char *out_buff, int out_sz){
	NODE *n			  = h_node;
	unsigned int cur  = 0;
	int	nbits		  = 0;
	int i			  = 0;	
	int idx			  = 0;
	int at			  = 0;
	for(i=0; i < enc_sz ; i++){
		cur = (cur<<8)|enc[i];
		nbits += 8;
		for( ;nbits >= 8; ){
			idx = (unsigned char)(cur >> (nbits-8));
			n = n->children[idx];
			if( n == NULL ){
				printf("invalid huffmand code\n");
				return -1; //invalid huffmand code
			}
			//printf("n->sym : %c , n->size = %d\n", n->sym, n->size);	
			//if( n->children == NULL){
			if( n->size == 0){
				if( out_sz > 0 && at > out_sz){
					printf("out of length\n");
					return -2; // lenght out of bound
				}
				out_buff[at++] = (char) n->sym; 
				nbits -= n->code_len;
				n = h_node;
			}else{
				nbits -= 8;
			}
		}
	}

	for( ;nbits > 0; ){
		n = n->children[ (unsigned char)( cur<<(8-nbits) ) ];
		if( n->size != 0 || n->code_len  > nbits){
			break;
		}

		out_buff[at++] = (char) n->sym;
		nbits -= n->code_len;
		n = h_node;
	}

	return at;
}

int hf_integer_encode(unsigned int enc_binary, int nprefix, unsigned char *buff){
	int i = 0;
	unsigned int ch	    = enc_binary;
	unsigned int ch2    = 0;
	unsigned int prefix = (1 << nprefix) - 1;

    if( ch < prefix  && (ch < 0xff) ){
		buff[i++] = ch & prefix;
	}else{
		buff[i++] = prefix;
		ch -= prefix;
		while(ch > 128)
		{
			ch2 = (ch % 128);
			ch2 += 128;
			buff[i++] = ch2;
			ch = ch/128;
		}
		buff[i++] = ch;
	}
	return i;
}

int hf_integer_decode(char *enc_buff, int nprefix , char *dec_buff){
	int i				= 0;
	int j				= 0;
	unsigned int M		= 0;
	unsigned int B		= 0;
	unsigned int ch	    = enc_buff[i++];
	unsigned int prefix = (1 << nprefix) - 1;

	if( ch < prefix ){
		dec_buff[j++] = ch;
	}else{
		M = 0;
		do{
			B = enc_buff[i++];
			ch = ch + ((B & 127) * (1 << M));
			M = M + 7;
		}
		while(B & 128);
		dec_buff[j] = ch;
	}
	return HM_RETURN_SUCCESS;
}

int hf_string_encode_len(unsigned char *enc, int enc_sz){
    int i       = 0;
    int len     = 0;
    for(i = 0; i < enc_sz; i++){
        len += huffman_code_len[(int)enc[i]];
    }
    
    return (len+7)/8;
}
    
void hf_print_hex(unsigned char *buff, int size){
	static char hex[] = {'0','1','2','3','4','5','6','7',
								'8','9','a','b','c','d','e','f'};
	int i = 0;
	for( i = 0;i < size; i++){
		unsigned char ch = buff[i];
		printf("(%u)%c", ch,hex[(ch>>4)]);
		printf("%c ", hex[(ch&0x0f)]);
	}
	printf("\n");
}


