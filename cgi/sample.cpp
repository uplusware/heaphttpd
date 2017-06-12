/*
	Copyright (c) openheap, uplusware
	uplusware@gmail.com
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <iostream>
#include "heapapi.h"
#include "version.h"

using namespace std;

int main(int argc, char* argv[])
{
	string postdata;
	cin>>postdata;
	string abc, def;
    map<string, string> _POST_VARS_;
    NIU_POST_GET_VARS(postdata.c_str(), _POST_VARS_);
    
    int a = atoi(_POST_VARS_["abc"].c_str());
    int d = atoi(_POST_VARS_["def"].c_str());
    int sum = a + d;
    char sz_buf[2048];
    sprintf(sz_buf, "<html><head><title>CGI sample</title></head><h1>%s</h1>%d + %d = %d</body></html>\n", getenv("SERVER_SOFTWARE"), a, d, sum);
	printf("Content-Type: text/html\r\n");
    printf("Content-Length: %u\r\n", strlen(sz_buf));
	printf("\r\n");
    printf(sz_buf);
    return 0;
}
