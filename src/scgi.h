/*
        Copyright (c) openheap, uplusware
        uplusware@gmail.com
*/

#ifndef _SCGI_H_
#define _SCGI_H_

#include "cgi.h"

#define SCGI_VERSION_1 1

class scgi : public cgi_base {
 public:
  scgi(const char* ipaddr, unsigned short port);
  scgi(const char* sock_file);
  virtual ~scgi();
  virtual int Connect();

  int SendParamsAndData(map<string, string>& params_map,
                        const char* postdata,
                        unsigned int postdata_len);
  int RecvAppData(vector<char>& binaryResponse, BOOL& continue_recv);

 protected:
  bool m_is_connected;
};

#endif /* _SCGI_H_ */