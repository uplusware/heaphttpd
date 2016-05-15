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
#include "httpsessionvar.h"
#include "httpservervar.h"

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
	
	memory_cache(const char* service_name, int process_seq, const char* dirpath);
	virtual ~memory_cache();

	map<string, file_cache*> m_file_cache;
	volatile unsigned long long m_file_cache_size;
	
	map<string, string> m_type_table;
	
	void load();
	void unload();
	
	void _save_cookies_();
	void _reload_cookies_();
	
	void reload_cookies();
	void save_cookies();
	void clear_cookies();
	void access_cookie(const char* name);
	
	void push_cookie(const char * name, Cookie & ck);
	void pop_cookie(const char * name);
	int  get_cookie(const char * name, Cookie & ck);
	
	void _save_session_vars_();
	void _reload_session_vars_();
	void reload_session_vars();
	void save_session_vars();
	void push_session_var(const char* uid, const char* name, const char* value);
	int  get_session_var(const char* uid, const char* name, string& value);
	void clear_session_vars();
	
    void _save_server_vars_();
    void _reload_server_vars_();
    void save_server_vars();
	void reload_server_vars();
	void push_server_var(const char* name, const char* value);
	int  get_server_var(const char* name, string& value);
	void clear_server_vars();
	
	file_cache* lock_file(const char * name, CACHE_DATA ** cache_data);
    void unlock_file(file_cache* fc);
    void clear_files();
    
	bool _push_file_(const char * name, char* buf, unsigned int len,
	    time_t t_modify, unsigned char* etag, file_cache** fout);
	    
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
private:
    int m_process_seq;
    string m_service_name;
    string m_dirpath;
    map<string, file_cache *>::iterator _find_oldest_file_();
    map<string, Cookie> m_cookies;
	map<session_var_key, session_var*> m_session_vars;
	map<string, server_var*> m_server_vars;
	
    pthread_rwlock_t m_cookie_rwlock;
    pthread_rwlock_t m_session_var_rwlock;
    pthread_rwlock_t m_server_var_rwlock;
    pthread_rwlock_t m_file_rwlock;
    map<string, unsigned long> m_server_vars_file_versions;
    //map<string, unsigned long> m_session_vars_file_versions;
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

