/*
        Copyright (c) openheap, uplusware
        uplusware@gmail.com
*/

#ifndef _WEBCGI_H_
#define _WEBCGI_H_

#include <map>
#include <string>
using namespace std;

class WebCGI {
 public:
  WebCGI();
  virtual ~WebCGI() {}

  void SetMeta(const char* varname, const char* varvalue);
  void SetData(const char* data, unsigned int len);

  map<string, string> m_meta_var;

  const char* GetData() { return m_data; }
  unsigned int GetDataLen() { return m_data_len; }

 private:
  const char* m_data;
  unsigned int m_data_len;
};

#endif /* _WEBCGI_H_ */