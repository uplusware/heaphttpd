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

#define FILE_MAX_SIZE 	(1024*1024*4)
#define MAX_CACHE_SIZE	(1024*1024*512)

#define FILE_ETAG_LEN 32

using namespace std;

typedef struct {
    unsigned char* buf;
    unsigned int len;
    unsigned char etag[FILE_ETAG_LEN + 1];
    time_t t_modify;
    time_t t_access;
} CACHE_DATA;

class file_cache
{
private:
    CACHE_DATA m_cache_data;
    pthread_rwlock_t m_cache_lock;  
    
public:   
    file_cache(unsigned char* buf, unsigned int len, time_t t_modify, char* etag)
    {
        pthread_rwlock_init(&m_cache_lock, NULL);
        
        m_cache_data.buf = (unsigned char*)malloc(len);
        memcpy(buf, m_cache_data.buf, len);
        
        m_cache_data.len = len;
        memcpy( m_cache_data.etag, etag, FILE_ETAG_LEN + 1);
        
        m_cache_data.t_modify = t_modify;
        m_cache_data.t_access = time(NULL);
    }
    
    virtual ~file_cache()
    {
        pthread_rwlock_wrlock(&m_cache_lock);
        if(m_cache_data.buf)
            free(m_cache_data.buf);
        pthread_rwlock_unlock(&m_cache_lock);   
             
        pthread_rwlock_destroy(&m_cache_lock);
    }
    
    CACHE_DATA* cache_lock()
    {
        pthread_rwlock_rdlock(&m_cache_lock);        
        return &m_cache_data;
    }
    
    void cache_update_access()
    {
        m_cache_data.t_access = time(NULL);
    }
    
    void cache_unlock()
    {
        pthread_rwlock_unlock(&m_cache_lock);
    }  
};

class memory_cache
{
public:
	
	memory_cache();
	virtual ~memory_cache();

	map<string, file_cache*> m_file_cache;
	unsigned long long m_file_cache_size;
	
	map<string, string> m_type_table;
	map<string, Cookie> m_cookies;
	
	void load(const char* dirpath);
	void unload();
	
	void push_cookie(const char * name, Cookie & ck);
	void pop_cookie(const char * name);
	int  get_cookie(const char * name, Cookie & ck);
	
	void push_file(const char * name, unsigned char* buf, unsigned int len, time_t t_modify, char* etag);
	void pop_file(const char * name);
	
	file_cache* lock_file(const char * name, CACHE_DATA ** cache_data);
    void unlock_file(file_cache* fc);
	
	string m_dirpath;
private:
    map<string, file_cache *>::iterator _find_oldest_file_();
    
    pthread_rwlock_t m_cookie_rwlock;
    pthread_rwlock_t m_file_rwlock;
};

#endif /* _CACHE_H_ */
