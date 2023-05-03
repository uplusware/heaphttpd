/*
        Copyright (c) openheap, uplusware
        uplusware@gmail.com
*/

#ifndef _HTTP_BALANCER_H_
#define _HTTP_BALANCER_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "heapapi.h"
#include "htdoc.h"
#include "http.h"
#include "webdoc.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

void* ext_request(CHttp* http_session,
                  const char* name,
                  const char* description,
                  const char* parameters,
                  BOOL* skip);

#ifdef __cplusplus
};
#endif /* __cplusplus */

#endif /* _HTTP_BALANCER_H_ */
