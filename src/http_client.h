/*
        Copyright (c) openheap, uplusware
        uplusware@gmail.com
*/

#ifndef _HTTP_CLIENT_H_
#define _HTTP_CLIENT_H_

#include <arpa/inet.h>
#include <netdb.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h> /* See NOTES */
#include "base.h"
#include "cache.h"
#include "util/general.h"

#include <arpa/inet.h>

#define HTTP_CLIENT_BUF_LEN 4096

using namespace std;

enum HTTP_Client_Parse_State {
  HTTP_Client_Parse_State_Header = 0,
  HTTP_Client_Parse_State_Content,
  HTTP_Client_Parse_State_Chunk_Header,
  HTTP_Client_Parse_State_Chunk_Content,
  HTTP_Client_Parse_State_Chunk_Footer,
};

class http_tunneling;

class http_chunk {
 public:
  http_chunk(http_tunneling* tunneling);
  virtual ~http_chunk();
  bool processing(char*& derived_buf, int& derived_buf_used_len);

 protected:
  unsigned int m_chunk_len;
  unsigned int m_sent_chunk;
  HTTP_Client_Parse_State m_state;

  string m_line_text;

  http_tunneling* m_http_tunneling;
};

class http_client {
 public:
  http_client(memory_cache* cache,
              const char* http_url,
              http_tunneling* tunneling);
  virtual ~http_client();
  bool parse(const char* text);

  bool processing(const char* buf, int buf_len);

  long long get_content_length() { return m_content_length; }

  bool is_chunked() { return m_is_chunked; }
  bool has_content_length() { return m_has_content_length; }

  int get_buf_used_len() { return m_buf_used_len; }

 protected:
  bool m_out_of_cache_header_scope;
  bool m_has_via;

  string m_line_text;
  bool m_has_content_length;
  long long m_content_length;
  bool m_is_200_ok;
  bool m_use_cache;
  bool m_cache_completed;
  int m_cache_max_age;
  int m_cache_shared_max_age;

  string m_cache_control;
  string m_content_type;
  string m_allow;
  string m_encoding;
  string m_language;
  string m_etag;
  string m_last_modified;
  string m_expires;
  string m_server;
  string m_via;

  bool m_is_chunked;

  int m_chunk_length;

  HTTP_Client_Parse_State m_state;

  char* m_buf;
  int m_buf_len;
  int m_buf_used_len;

  long long m_sent_content_length;

  http_chunk* m_chunk;
  string m_http_tunneling_url;
  memory_cache* m_cache;

  char* m_cache_buf;
  unsigned int m_cache_data_len;

  string m_header;

  http_tunneling* m_http_tunneling;
};
#endif /* _HTTP_CLIENT_H_ */