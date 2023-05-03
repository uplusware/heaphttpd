/*
        Copyright (c) openheap, uplusware
        uplusware@gmail.com
*/

#ifndef _HTTP_UTILITY_H_
#define _HTTP_UTILITY_H_

#include "util/general.h"

class http_utility {
 public:
  http_utility();
  virtual ~http_utility();

  static const char* encode_URI(const char* src, string& dst);
  static const char* decode_URI(const char* src, string& dst);
  static const char* escape_HTML(const char* src, string& dst);
};

#endif /* _HTTP_UTILITY_H_ */