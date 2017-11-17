/*
	Copyright (c) openheap, uplusware
	uplusware@gmail.com
*/

#ifndef _HTTPREVERSE_H_
#define _HTTPREVERSE_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
using namespace std;

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */
    void* reverse_delivery(const char * name, const char * description, const char * parameters, const char* clientip, const char* input_method_url, string * output_method_url, int * skip);
#ifdef __cplusplus
};
#endif /* __cplusplus */

#endif /* _HTTPREVERSE_H_ */
