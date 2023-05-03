/*
        Copyright (c) openheap, uplusware
        uplusware@gmail.com
*/

#ifndef _HTTP2_CLIENT_H_
#define _HTTP2_CLIENT_H_

#include "http2comm.h"
#include "http_client.h"

class http2_client : public http_client {
 public:
  http2_client(memory_cache* cache,
               const char* http_url,
               http_tunneling* tunneling);
  virtual ~http2_client();

  virtual bool processing(const char* buf, int buf_len);

 private:
  http1_1_client* m_http1_1_client;

  HTTP2_Frame* m_frame_hdr;
  int m_frame_hdr_valid_len;

  char* m_payload;
  uint_32 m_payload_valid_len;

  Http2_State m_http2_state;
};

#endif /* _HTTP2_CLIENT_H_ */
