/*
	Copyright (c) openheap, uplusware
	uplusware@gmail.com
*/

#ifndef _CACHE_H_
#define _CACHE_H_

#include <list>
#include <map>
#include <string>
#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include "cookie.h"

using namespace std;

typedef struct 
{
	char* pbuf;
	unsigned long flen;
	time_t lastmodifytime;
    char etag[33];
    time_t pushtime;
} filedata;

class memory_cache
{
public:
	
	memory_cache();
	virtual ~memory_cache();

	map<string, filedata> m_filedata;
	unsigned long long m_filedata_size;
	
	map<string, string> m_type_table;
	map<string, Cookie> m_cookies;
	
	void load(const char* dirpath);
	void unload();
	
	void push_cookie(const char * name, Cookie & ck);
	void pop_cookie(const char * name);
	int get_cookie(const char * name, Cookie & ck);
	
	void push_file(const char * name, filedata& fd);
	void pop_file(const char * name);
	int get_file(const char * name, filedata & fd);

	string m_dirpath;
private:
    pthread_rwlock_t m_cookie_rwlock;
    pthread_rwlock_t m_file_rwlock;
};

#endif /* _CACHE_H_ */
