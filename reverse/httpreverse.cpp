/*
	Copyright (c) openheap, uplusware
	uplusware@gmail.com
*/


#include "httpreverse.h"

void* reverse_delivery(const char * name, const char * description, const char * parameters, const char* clientip, const char* input_method_url, string * output_method_url, int * skip)
{
	/*
	Reverse proxy, HA proxy and load balance server hook code position.
	Add http://x.x.x.x before the relative resource path.
	*/
	
	/* sample code without any url changing */
    
    int buf_len = strlen(input_method_url);
    
    char* sz_method = (char*)malloc(buf_len + 1);
    char* sz_resource = (char*)malloc(buf_len + 1);
    char* sz_querystring = (char*)malloc(buf_len + 1);
    
    memset(sz_method, 0, buf_len + 1);
    memset(sz_resource, 0, buf_len + 1);
    memset(sz_querystring, 0, buf_len + 1);

    sscanf(input_method_url, "%[A-Z \t]", sz_method);    
    sscanf(input_method_url, "%*[^/]%[^? \t\r\n]?%[^ \t\r\n]", sz_resource, sz_querystring);
    
    printf("METHOD: %s, URL: %s\n", sz_method, sz_resource);
    
    free(sz_method);
    free(sz_resource);
    free(sz_querystring);
            
    *skip = 0; // zero is not skip, non-zero is skip
    *output_method_url = input_method_url;
    
    printf("Original URL: [%s], Reverse URL: [%s] from %s\n", input_method_url, output_method_url->c_str(), clientip);
}