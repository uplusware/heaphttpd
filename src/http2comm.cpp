/*
	Copyright (c) openheap, uplusware
	uplusware@gmail.com
*/
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <endian.h>
#include <time.h>
#include "http2comm.h"

void http2_set_length(HTTP2_Frame* http2_frame, unsigned int length)
{
    http2_frame->length.len24 = htonl(length) >> 8;
}

void http2_set_indentifier(HTTP2_Frame* http2_frame, unsigned int indentifier)
{
    http2_frame->indentifier = htonl(indentifier) >> 1;
}

void http2_get_length(HTTP2_Frame* http2_frame, unsigned int &length)
{
    length = htonl(http2_frame->length.len24 << 8);
}

void http2_get_indentifier(HTTP2_Frame* http2_frame, unsigned int &indentifier)
{
    indentifier = htonl(http2_frame->indentifier << 1);
}
