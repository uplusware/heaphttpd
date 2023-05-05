/*
        Copyright (c) openheap, uplusware
        uplusware@gmail.com
*/

#ifndef _ECHO_H_
#define _ECHO_H_

#include <openssl/err.h>
#include <openssl/ssl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "websocket.h"

extern "C" void* ws_echo_main(int sockfd, SSL* ssl);

#endif /* _ECHO_H_ */
