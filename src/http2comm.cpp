/*
        Copyright (c) openheap, uplusware
        uplusware@gmail.com
*/
#include "http2comm.h"
#include <arpa/inet.h>
#include <endian.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

void http2_set_length(HTTP2_Frame* http2_frame, unsigned int length) {
  http2_frame->length.len24 = htonl(length) >> 8;
}

void http2_set_identifier(HTTP2_Frame* http2_frame, unsigned int identifier) {
  http2_frame->identifier = htonl(identifier) >> 1;
}

void http2_get_length(HTTP2_Frame* http2_frame, unsigned int& length) {
  length = htonl(http2_frame->length.len24 << 8);
}

void http2_get_identifier(HTTP2_Frame* http2_frame, unsigned int& identifier) {
  identifier = htonl(http2_frame->identifier << 1);
}
