#ifndef _HPACK_H_
#define _HPACK_H_

#include "util/general.h"
#include "util/huffman.h"
#include <string>
using namespace std;

/*
Indexed Header Field
     0   1   2   3   4   5   6   7
   +---+---+---+---+---+---+---+---+
   | 1 |        Index (7+)         |
   +---+---------------------------+

Literal Header Field with Incremental Indexing -- Indexed Name
     0   1   2   3   4   5   6   7
   +---+---+---+---+---+---+---+---+
   | 0 | 1 |      Index (6+)       |
   +---+---+-----------------------+
   | H |     Value Length (7+)     |
   +---+---------------------------+
   | Value String (Length octets)  |
   +-------------------------------+
   
Literal Header Field with Incremental Indexing -- New Name
     0   1   2   3   4   5   6   7
   +---+---+---+---+---+---+---+---+
   | 0 | 1 |           0           |
   +---+---+-----------------------+
   | H |     Name Length (7+)      |
   +---+---------------------------+
   |  Name String (Length octets)  |
   +---+---------------------------+
   | H |     Value Length (7+)     |
   +---+---------------------------+
   | Value String (Length octets)  |
   +-------------------------------+

Literal Header Field without Indexing -- Indexed Name
     0   1   2   3   4   5   6   7
   +---+---+---+---+---+---+---+---+
   | 0 | 0 | 0 | 0 |  Index (4+)   |
   +---+---+-----------------------+
   | H |     Value Length (7+)     |
   +---+---------------------------+
   | Value String (Length octets)  |
   +-------------------------------+

Literal Header Field without Indexing -- New Name
     0   1   2   3   4   5   6   7
   +---+---+---+---+---+---+---+---+
   | 0 | 0 | 0 | 0 |       0       |
   +---+---+-----------------------+
   | H |     Name Length (7+)      |
   +---+---------------------------+
   |  Name String (Length octets)  |
   +---+---------------------------+
   | H |     Value Length (7+)     |
   +---+---------------------------+
   | Value String (Length octets)  |
   +-------------------------------+

Literal Header Field Never Indexed -- Indexed Name
     0   1   2   3   4   5   6   7
   +---+---+---+---+---+---+---+---+
   | 0 | 0 | 0 | 1 |  Index (4+)   |
   +---+---+-----------------------+
   | H |     Value Length (7+)     |
   +---+---------------------------+
   | Value String (Length octets)  |
   +-------------------------------+

Literal Header Field Never Indexed -- New Name
     0   1   2   3   4   5   6   7
   +---+---+---+---+---+---+---+---+
   | 0 | 0 | 0 | 1 |       0       |
   +---+---+-----------------------+
   | H |     Name Length (7+)      |
   +---+---------------------------+
   |  Name String (Length octets)  |
   +---+---------------------------+
   | H |     Value Length (7+)     |
   +---+---------------------------+
   | Value String (Length octets)  |
   +-------------------------------+
*/

typedef struct 
{
	uint_8 len : 7;
	uint_8 h : 1;
	char str[0];
} HTTP2_Header_String;

typedef struct 
{
	union 
	{
		struct
		{
			uint_8 index : 7;
			uint_8 code : 1;
		} indexed;
		struct
		{
			uint_8 index : 6;
			uint_8 code : 2;
			
		} indexing;
		struct
		{
			uint_8 index : 4;
			uint_8 code : 4;
		} without_indexing;
        struct
		{
			uint_8 index : 4;
			uint_8 code : 4;
		} never_indexed;
		uint_8 type;
	} tag;
	HTTP2_Header_String header[0];
} HTTP2_Header_Field;

enum index_type_e
{
    type_indexed = 0,
    type_indexing_indexed_name,
    type_indexing_new_name,
    type_without_indexing_indexed_name,
    type_without_indexing_new_name,
    type_never_indexed_indexed_name,
    type_never_indexed_new_name
};

typedef struct 
{
    index_type_e index_type;
    uint_32 index;
    string name;
    string value;
} HTTP2_Header;

class hpack
{
public:
    hpack(const HTTP2_Header_Field* field, int len);
    virtual ~hpack();
    
    vector<HTTP2_Header> m_decoded_headers;
private:
    const HTTP2_Header_Field* m_field;
    int m_len;
};
#endif /* _HPACK_H_ */