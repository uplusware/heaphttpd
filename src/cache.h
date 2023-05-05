/*
        Copyright (c) openheap, uplusware
        uplusware@gmail.com
*/

#ifndef _CACHE_H_
#define _CACHE_H_

#include <dirent.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <list>
#include <map>
#include <string>
#ifdef _WITH_MEMCACHED_
#include <libmemcached/memcached.h>
#endif /* _WITH_MEMCACHED_ */
#include "base.h"
#include "cookie.h"
#include "httpservervar.h"
#include "httpsessionvar.h"

#define FILE_ETAG_LEN 32

using namespace std;

typedef struct {
  char* buf;
  volatile unsigned int len;
  unsigned char etag[FILE_ETAG_LEN + 1];
  volatile time_t t_modify;
  volatile time_t t_access;
  volatile time_t t_create;
} FILE_CACHE_DATA;

typedef struct {
  char* buf;
  volatile unsigned int len;
  string type;
  string allow;
  string encoding;
  string language;
  string cache;
  string etag;
  string expires;
  string last_modified;
  string server;
  string via;
  volatile time_t t_access;
} TUNNELING_CACHE_DATA;

class file_cache {
 private:
  FILE_CACHE_DATA m_cache_data;
  pthread_rwlock_t m_cache_lock;
  time_t m_cache_max_time;

 public:
  file_cache(char* buf,
             unsigned int len,
             time_t t_modify,
             unsigned char* etag,
             unsigned int max_age = 900) {
    pthread_rwlock_init(&m_cache_lock, NULL);

    m_cache_data.buf = new char[len + 1];
    memcpy(m_cache_data.buf, buf, len);

    m_cache_data.len = len;
    memcpy(m_cache_data.etag, etag, FILE_ETAG_LEN + 1);

    m_cache_data.t_modify = t_modify;
    m_cache_data.t_access = time(NULL);
    m_cache_data.t_create = time(NULL);

    m_cache_max_time = time(NULL) + max_age;
  }

  virtual ~file_cache() {
    pthread_rwlock_wrlock(&m_cache_lock);

    if (m_cache_data.buf)
      delete[] m_cache_data.buf;
    pthread_rwlock_unlock(&m_cache_lock);

    pthread_rwlock_destroy(&m_cache_lock);
  }

  FILE_CACHE_DATA* file_rdlock() {
    pthread_rwlock_rdlock(&m_cache_lock);
    m_cache_data.t_access = time(NULL);
    return &m_cache_data;
  }

  FILE_CACHE_DATA* file_wrlock() {
    pthread_rwlock_wrlock(&m_cache_lock);
    m_cache_data.t_access = time(NULL);
    return &m_cache_data;
  }

  void file_unlock() { pthread_rwlock_unlock(&m_cache_lock); }

  bool file_fresh() { return time(NULL) > m_cache_max_time ? false : true; }
};

class tunneling_cache {
 private:
  TUNNELING_CACHE_DATA m_cache_data;
  pthread_rwlock_t m_tunneling_lock;
  time_t m_cache_max_time;

 public:
  tunneling_cache(char* buf,
                  unsigned int len,
                  const char* type,
                  const char* cache,
                  const char* allow,
                  const char* encoding,
                  const char* language,
                  const char* etag,
                  const char* last_modified,
                  const char* expires,
                  const char* server,
                  const char* via,
                  unsigned int max_age) {
    pthread_rwlock_init(&m_tunneling_lock, NULL);

    m_cache_data.buf = new char[len + 1];
    memcpy(m_cache_data.buf, buf, len);

    m_cache_data.len = len;
    m_cache_data.type = type;
    m_cache_data.cache = cache;
    m_cache_data.allow = allow;
    m_cache_data.encoding = encoding;
    m_cache_data.language = language;
    m_cache_data.etag = etag;
    m_cache_data.last_modified = last_modified;
    m_cache_data.expires = expires;
    m_cache_data.server = server;
    m_cache_data.via = via;
    m_cache_max_time = time(NULL) + max_age;
  }

  virtual ~tunneling_cache() {
    pthread_rwlock_wrlock(&m_tunneling_lock);

    if (m_cache_data.buf)
      delete[] m_cache_data.buf;
    m_cache_data.buf = NULL;
    pthread_rwlock_unlock(&m_tunneling_lock);

    pthread_rwlock_destroy(&m_tunneling_lock);
  }

  TUNNELING_CACHE_DATA* tunneling_rdlock() {
    pthread_rwlock_rdlock(&m_tunneling_lock);
    m_cache_data.t_access = time(NULL);
    return &m_cache_data;
  }

  TUNNELING_CACHE_DATA* tunneling_wrlock() {
    pthread_rwlock_wrlock(&m_tunneling_lock);
    m_cache_data.t_access = time(NULL);
    return &m_cache_data;
  }

  void tunneling_unlock() { pthread_rwlock_unlock(&m_tunneling_lock); }

  bool tunneling_fresh() {
    return time(NULL) > m_cache_max_time ? false : true;
  }
};

typedef struct {
  string path;
  string full_path;
} http2_push_file_t;

class memory_cache {
 public:
#ifdef _WITH_MEMCACHED_
  memory_cache(const char* service_name,
               int process_seq,
               const char* dirpath,
               map<string, int>& memcached_list);
#else
  memory_cache(const char* service_name, int process_seq, const char* dirpath);
#endif /* _WITH_MEMCACHED_ */
  virtual ~memory_cache();

  map<string, file_cache*> m_file_cache;
  map<string, tunneling_cache*> m_tunneling_cache;
  vector<http2_push_file_t> m_http2_push_list;

  volatile unsigned long long m_file_cache_size;
  volatile unsigned long long m_tunneling_cache_size;

  map<string, string> m_type_table;

  void load();
  void unload();

  void _save_cookies_();
  void _reload_cookies_();

  void reload_cookies();
  void save_cookies();
  void clear_cookies();
  void access_cookie(const char* name);

  void push_cookie(const char* name, Cookie* ck);
  void pop_cookie(const char* name);
  int get_cookie(const char* name, Cookie* ck);

  void _save_session_vars_();
  void _reload_session_vars_();
  void reload_session_vars();

  void save_session_vars();
  void push_session_var(const char* uid, const char* name, const char* value);
  int get_session_var(const char* uid, const char* name, string& value);
  void clear_session_vars();

  void _reload_users_();
  void reload_users();

  void _save_server_vars_();
  void _reload_server_vars_();
  void save_server_vars();
  void reload_server_vars();
  void push_server_var(const char* name, const char* value);
  int get_server_var(const char* name, string& value);
  void clear_server_vars();

  bool _find_user_(const char* id, string& pwd);
  bool find_user(const char* id, string& pwd);

  // file
  file_cache* lock_file(const char* name, FILE_CACHE_DATA** cache_data);
  void unlock_file(file_cache* fc);
  void clear_files();

  bool _push_file_(const char* name,
                   char* buf,
                   unsigned int len,
                   time_t t_modify,
                   unsigned char* etag,
                   file_cache** f_out);

  bool _find_file_(const char* name, file_cache** f_out);

  void wrlock_file_cache() { pthread_rwlock_wrlock(&m_file_rwlock); }

  void rdlock_file_cache() { pthread_rwlock_rdlock(&m_file_rwlock); }

  void unlock_file_cache() { pthread_rwlock_unlock(&m_file_rwlock); }

  // tunneling
  bool _find_tunneling_(const char* name, tunneling_cache** t_out);
  bool _push_tunneling_(const char* name,
                        char* buf,
                        unsigned int len,
                        const char* type,
                        const char* cache,
                        const char* allow,
                        const char* encoding,
                        const char* language,
                        const char* last_modify,
                        const char* etag,
                        const char* expires,
                        const char* server,
                        const char* via,
                        unsigned int max_age,
                        tunneling_cache** t_out);

  // tunneling_cache* lock_tunneling_cache(const char * name,
  // TUNNELING_CACHE_DATA ** cache_data);

  void unlock_tunneling_cache(tunneling_cache* fc);
  void clear_tunnelings();

  void wrlock_tunneling_cache() { pthread_rwlock_wrlock(&m_tunneling_rwlock); }

  void rdlock_tunneling_cache() { pthread_rwlock_rdlock(&m_tunneling_rwlock); }

  void unlock_tunneling_cache() { pthread_rwlock_unlock(&m_tunneling_rwlock); }

  // http2
  void load_http2_push_list(const char* dir_path);

 private:
  int m_process_seq;
  string m_service_name;
  string m_dirpath;
  map<string, file_cache*>::iterator _find_oldest_file_();
  map<string, tunneling_cache*>::iterator _find_oldest_tunneling_();
  map<string, Cookie*> m_cookies;
  map<session_var_key, session_var*> m_session_vars;
  map<string, server_var*> m_server_vars;

  pthread_rwlock_t m_cookie_rwlock;
  pthread_rwlock_t m_session_var_rwlock;
  pthread_rwlock_t m_server_var_rwlock;
  pthread_rwlock_t m_file_rwlock;
  pthread_rwlock_t m_tunneling_rwlock;
  pthread_rwlock_t m_users_rwlock;
  map<string, unsigned long> m_server_vars_file_versions;
  map<string, unsigned long> m_session_vars_file_versions;
  string m_localhostname;

  map<string, string> m_users_list;

#ifdef _WITH_MEMCACHED_
  memcached_st* m_memcached;
  map<pthread_t, memcached_st*> m_memcached_map;
  memcached_server_st* m_memcached_servers;
#endif /* _WITH_MEMCACHED_ */
};

#endif /* _CACHE_H_ */
