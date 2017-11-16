/*
	Copyright (c) openheap, uplusware
	uplusware@gmail.com
*/


#include "httpreverse.h"

void* reverse_delivery(const char * name, const char * description, const char * parameters, const char* input_url, string * output_url)
{
	/*
	Reverse proxy, HA proxy and load balance server hook code position.
	Add http://x.x.x.x before the relative resource path.
	*/
	
	/* sample code without any url changing */
    *output_url = input_url;
}