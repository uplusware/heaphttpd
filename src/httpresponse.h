/*
        Copyright (c) openheap, uplusware
        uplusware@gmail.com
*/

#ifndef _HTTP_RESPONSE_H_
#define _HTTP_RESPONSE_H_

#include "http.h"

class http_response {
 public:
  http_response(CHttp* session);
  virtual ~http_response();

  void send_header(const char* h, int l);
  void send_content(const char* c, int l);

 private:
  CHttp* m_session;
};

#endif /* _HTTP_RESPONSE_H_ */