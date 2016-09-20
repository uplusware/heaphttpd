/*
	Copyright (c) openheap, uplusware
	uplusware@gmail.com
*/

#ifndef _HTTP2COMM_H_
#define _HTTP2COMM_H_

#include "util/general.h"

/*
   HTTP frame
   All frames begin with a fixed 9-octet header followed by a variable-
   length payload.

    +-----------------------------------------------+
    |                 Length (24)                   |
    +---------------+---------------+---------------+
    |   Type (8)    |   Flags (8)   |
    +-+-------------+---------------+-------------------------------+
    |R|                 Stream Identifier (31)                      |
    +=+=============================================================+
    |                   Frame Payload (0...)                      ...
    +---------------------------------------------------------------+
*/

#define HTTP2_FRAME_TYPE_DATA             0x00
#define HTTP2_FRAME_TYPE_HEADERS          0x01
#define HTTP2_FRAME_TYPE_PRIORITY         0x02
#define HTTP2_FRAME_TYPE_RST_STREAM       0x03
#define HTTP2_FRAME_TYPE_SETTINGS         0x04
#define HTTP2_FRAME_TYPE_PUSH_PROMISE     0x05
#define HTTP2_FRAME_TYPE_PING             0x06
#define HTTP2_FRAME_TYPE_GOAWAY           0x07
#define HTTP2_FRAME_TYPE_WINDOW_UPDATE    0x08
#define HTTP2_FRAME_TYPE_CONTINUATION     0x09

#define HTTP2_NO_ERROR                    0x00
#define HTTP2_PROTOCOL_ERROR              0x01
#define HTTP2_INTERNAL_ERROR              0x02
#define HTTP2_FLOW_CONTROL_ERROR          0x03
#define HTTP2_SETTINGS_TIMEOUT            0x04
#define HTTP2_STREAM_CLOSED               0x05
#define HTTP2_FRAME_SIZE_ERROR            0x06
#define HTTP2_REFUSED_STREAM              0x07
#define HTTP2_CANCEL                      0x08
#define HTTP2_COMPRESSION_ERROR           0x09
#define HTTP2_CONNECT_ERROR               0x0a
#define HTTP2_ENHANCE_YOUR_CALM           0x0b
#define HTTP2_INADEQUATE_SECURITY         0x0c
#define HTTP2_HTTP_1_1_REQUIRED           0x0d


#define HTTP2_FRAME_FLAG_UNSET            0x00
#define HTTP2_FRAME_FLAG_END_STREAM	      0x01
#define HTTP2_FRAME_FLAG_SETTING_ACK	  0x01
#define HTTP2_FRAME_FLAG_PING_ACK	      0x01
#define HTTP2_FRAME_FLAG_END_HEADERS      0x04
#define HTTP2_FRAME_FLAG_PADDED	          0x08
#define HTTP2_FRAME_FLAG_PRIORITY	      0x20

#define HTTP2_FRAME_R_UNSET               0x0
#define HTTP2_FRAME_R_SET                 0x1

#define HTTP2_FRAME_INDENTIFIER_WHOLE     0x00

#define HTTP2_SETTINGS_HEADER_TABLE_SIZE        0x01
#define HTTP2_SETTINGS_ENABLE_PUSH              0x02
#define HTTP2_SETTINGS_MAX_CONCURRENT_STREAMS   0x03
#define HTTP2_SETTINGS_INITIAL_WINDOW_SIZE      0x04
#define HTTP2_SETTINGS_MAX_FRAME_SIZE           0x05
#define HTTP2_SETTINGS_MAX_HEADER_LIST_SIZE     0x06

#pragma pack(push)
#pragma pack(1)

typedef struct
{
	union{
	uint_8  len3b[3];
	uint_32 len24 : 24;
	} length;
	uint_8 type;
	uint_8 flags;
	uint_32 r : 1;
	uint_32 indentifier : 31;
	char payload[0];
} HTTP2_Frame;

/* Maxximn padding length */
#define MAX_PADDING_LEN     255
/*
    DATA frames
    +---------------+
    |Pad Length? (8)|
    +---------------+-----------------------------------------------+
    |                            Data (*)                         ...
    +---------------------------------------------------------------+
    |                           Padding (*)                       ...
    +---------------------------------------------------------------+
*/

typedef struct
{
	uint_8 pad_length;
	char data_padding[0];
} HTTP2_Frame_Data1;

typedef struct
{
	char data[0];
} HTTP2_Frame_Data2;

/*
	HEADERS Frame Payload
    +---------------+
    |Pad Length? (8)|
    +-+-------------+-----------------------------------------------+
    |E|                 Stream Dependency? (31)                     |
    +-+-------------+-----------------------------------------------+
    |  Weight? (8)  |
    +-+-------------+-----------------------------------------------+
    |                   Header Block Fragment (*)                 ...
    +---------------------------------------------------------------+
    |                           Padding (*)                       ...
    +---------------------------------------------------------------+
*/

typedef struct
{
	uint_8 pad_length;
	uint_32 e : 1;
	uint_32 dependency : 31;
	uint_8 weight;
	char block_fragment_padding[0];
} HTTP2_Frame_Header;

typedef struct
{
	uint_8 pad_length;
	char bottom[0];
} HTTP2_Frame_Header_Pad;

typedef struct
{
	uint_32 e : 1;
	uint_32 dependency : 31;
	uint_8 weight;
    char block_fragment_padding[0];
} HTTP2_Frame_Header_Weight;

typedef struct
{
	unsigned char block_fragment[0];
    char padding[0];
} HTTP2_Frame_Header_Fragment;

/*
	Setting Format
	+-------------------------------+
	| Identifier (16)               |
	+-------------------------------+-------------------------------+
	| Value (32)                                                    |
	+---------------------------------------------------------------+
*/

#define HTTP2_SETTINGS_HEADER_TABLE_SIZE         0x01
#define HTTP2_SETTINGS_ENABLE_PUSH               0x02
#define HTTP2_SETTINGS_MAX_CONCURRENT_STREAMS    0x03
#define HTTP2_SETTINGS_INITIAL_WINDOW_SIZE       0x04
#define HTTP2_SETTINGS_MAX_FRAME_SIZE            0x05
#define HTTP2_SETTINGS_MAX_HEADER_LIST_SIZE      0x06

typedef struct
{
	uint_16 identifier;
	uint_32 value;
} HTTP2_Setting;

/*
    PRIORITY Frame Payload
    +-+-------------------------------------------------------------+
    |E| Stream Dependency (31)                                      |
    +-+-------------+-----------------------------------------------+
    | Weight (8)    |
    +-+-------------+
*/
typedef struct
{
	uint_32 e : 1;
	uint_32 dependency : 31;
	uint_8 weight;
} HTTP2_Frame_Priority;

/*
    GOAWAY Payload Format
    +-+-------------------------------------------------------------+
    |R| Last-Stream-ID (31)                                         |
    +-+-------------------------------------------------------------+
    | Error Code (32)                                               |
    +---------------------------------------------------------------+
    | Additional Debug Data (*)                                     |
    +---------------------------------------------------------------+
*/
typedef struct
{
	uint_32 r : 1;
	uint_32 last_stream_id : 31;
	uint_32 error_code;
    char debug_data[0];
} HTTP2_Frame_Goaway;

/*
    WINDOW_UPDATE Payload Format
    +-+-------------------------------------------------------------+
    |R| Window Size Increment (31)                                  |
    +-+-------------------------------------------------------------+
*/
typedef struct
{
	uint_32 r : 1;
	uint_32 win_size : 31;
} HTTP2_Frame_Window_Update;

/*
    PING Payload Format
    +---------------------------------------------------------------+
    |                                                               |
    | Opaque Data (64)                                              |
    |                                                               |
    +---------------------------------------------------------------+
*/

typedef struct
{
	uint_8 data[8];
} HTTP2_Frame_Ping;

/*
    CONTINUATION Frame Payload
    +---------------------------------------------------------------+
    | Header Block Fragment (*)                                   ...
    +---------------------------------------------------------------+
*/
typedef struct
{
	unsigned char block_fragment[0];
} HTTP2_Frame_Continuation;

/*
    PUSH_PROMISE Payload Format
    +---------------+
    |Pad Length? (8)|
    +-+-------------+-----------------------------------------------+
    |R| Promised Stream ID (31)                                     |
    +-+-----------------------------+-------------------------------+
    | Header Block Fragment (*)                                   ...
    +---------------------------------------------------------------+
    | Padding (*)                                                 ...
    +---------------------------------------------------------------+ 
*/

typedef struct
{
	uint_8 pad_length;
	uint_32 r : 1;
	uint_32 promised_stream_ind : 31;
	char block_fragment_padding[0];
} HTTP2_Frame_Push_Promise;

typedef struct
{
	uint_32 r : 1;
	uint_32 promised_stream_ind : 31;
	char block_fragment[0];
} HTTP2_Frame_Push_Promise_Without_Pad;

typedef struct
{
	char block_fragment[0];
    char padding[0];
} HTTP2_Frame_Push_Promise_Fragment;

#pragma pack(pop)

void http2_set_length(HTTP2_Frame* http2_frame, unsigned int length);
void http2_set_indentifier(HTTP2_Frame* http2_frame, unsigned int indentifier);
void http2_get_length(HTTP2_Frame* http2_frame, unsigned int& length);
void http2_get_indentifier(HTTP2_Frame* http2_frame, unsigned int& indentifier);

#endif /* _HTTP2COMM_H_ */