/*
        Copyright (c) openheap, uplusware
        uplusware@gmail.com
*/
#ifndef _STORAGE_H_
#define _STORAGE_H_

#ifdef _MONGODB_
#include <mongoc.h>
#else
#include <mysql.h>
#endif /* _MONGODB_ */

#include <pthread.h>
#include <string>

#define BOOL unsigned int
#define TRUE 1
#define FALSE 0

using namespace std;

class DatabaseStorage {
 public:
  DatabaseStorage(const char* encoding, const char* private_path);
  virtual ~DatabaseStorage();

  // system
  int Connect(const char* host,
              const char* username,
              const char* password,
              const char* database,
              const char* unix_socket,
              unsigned short port = 0);

  void Close();

  int Ping();

  void KeepLive();

  int ShowDatabases(string& databases);

 protected:
#ifdef _MONGODB_
  mongoc_client_t* m_hMongoDB;
  mongoc_database_t* m_hDatabase;
#else
  MYSQL* m_hMySQL;
#endif /* _MONGODB_ */

  BOOL m_bOpened;
#ifndef _MONGODB_
  void SqlSafetyString(string& strInOut);
#endif /* _MONGODB_ */
  string m_host;
  string m_username;
  string m_password;
  string m_database;
  unsigned short m_port;
  string m_unix_socket;

  string m_encoding;
  string m_private_path;

  pthread_mutex_t m_thread_query_lock;

 private:
#ifndef _MONGODB_
  int mysql_thread_real_query(MYSQL* mysql,
                              const char* stmt_str,
                              unsigned long length) {
    pthread_mutex_lock(&m_thread_query_lock);
    int ret = mysql_real_query(mysql, stmt_str, length);
    pthread_mutex_unlock(&m_thread_query_lock);
    return ret;
  }
#endif /* _MONGODB_*/
};

#endif /* _STORAGE_H_ */
