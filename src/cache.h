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

#define FILE_MAX_SIZE 	(1024*1024*4)   /* 4M */
#define MAX_CACHE_SIZE	(1024*1024*512) /* 512M */

#define FILE_ETAG_LEN 32

using namespace std;

typedef struct {
    char* buf;
    volatile unsigned int len;
    unsigned char etag[FILE_ETAG_LEN + 1];
    volatile time_t t_modify;
    volatile time_t t_access;
    volatile time_t t_create;
} CACHE_DATA;

class file_cache
{
private:
    CACHE_DATA m_cache_data;
    pthread_rwlock_t m_cache_lock;  
    
public:   
    file_cache(char* buf, unsigned int len, time_t t_modify, unsigned char* etag)
    {
        pthread_rwlock_init(&m_cache_lock, NULL);
        
        m_cache_data.buf = new char[len];
        memcpy(m_cache_data.buf, buf, len);
        
        m_cache_data.len = len;
        memcpy( m_cache_data.etag, etag, FILE_ETAG_LEN + 1);
        
        m_cache_data.t_modify = t_modify;
        m_cache_data.t_access = time(NULL);
        m_cache_data.t_create = time(NULL);
    }
    
    virtual ~file_cache()
    {
        pthread_rwlock_wrlock(&m_cache_lock);
        
        if(m_cache_data.buf)
            delete[] m_cache_data.buf;
        pthread_rwlock_unlock(&m_cache_lock);   
             
        pthread_rwlock_destroy(&m_cache_lock);
    }
    
    CACHE_DATA* file_rdlock()
    {
        pthread_rwlock_rdlock(&m_cache_lock);
        m_cache_data.t_access = time(NULL);     
        return &m_cache_data;
    }
    
    CACHE_DATA* file_wrlock()
    {
        pthread_rwlock_wrlock(&m_cache_lock);
        m_cache_data.t_access = time(NULL);     
        return &m_cache_data;
    }
    
    void file_unlock()
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
	volatile unsigned long long m_file_cache_size;
	
	map<string, string> m_type_table;
	map<string, Cookie> m_cookies;
	
	void load(const char* dirpath);
	void unload();
	
	void push_cookie(const char * name, Cookie & ck);
	void pop_cookie(const char * name);
	int  get_cookie(const char * name, Cookie & ck);
	
	bool _push_file_(const char * name, 
	    char* buf, unsigned int len, time_t t_modify, unsigned char* etag,
	    file_cache** fout);
	    
	bool _find_file_(const char * name, file_cache** fc);
	
	void wrlock_cache()
	{
	    pthread_rwlock_wrlock(&m_file_rwlock);
	}
	
	void rdlock_cache()
	{
	    pthread_rwlock_wrlock(&m_file_rwlock);
	}
	
	void unlock_cache()
	{
	    pthread_rwlock_unlock(&m_file_rwlock);
	}
	
	file_cache* lock_file(const char * name, CACHE_DATA ** cache_data);
    void unlock_file(file_cache* fc);
	
	string m_dirpath;
private:
    map<string, file_cache *>::iterator _find_oldest_file_();
    
    pthread_rwlock_t m_cookie_rwlock;
    pthread_rwlock_t m_file_rwlock;
};

class cache_instance
{
private:
    memory_cache* m_cache;
    CACHE_DATA * m_cache_data;
    file_cache* m_file_cache;
public:
    cache_instance(memory_cache* cache, const char * name)
    {
        m_cache = cache;
        m_file_cache = m_cache->lock_file(name, &m_cache_data);
    }
    
    virtual ~cache_instance()
    {
        if(m_file_cache)
            m_cache->unlock_file(m_file_cache);
    }
    
    CACHE_DATA * get_cache_data() { return m_cache_data; }
};
#endif /* _CACHE_H_ */

