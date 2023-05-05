/*
        Copyright (c) openheap, uplusware
        uplusware@gmail.com
*/
#ifndef _EXTENSION_H_
#define _EXTENSION_H_

typedef struct {
  void* handle;
  string name;
  string description;
  string parameters;
} http_extension_t;

enum http_ext_tunneling_continuing {
  http_ext_tunneling_continuing_yes = 0,
  http_ext_tunneling_continuing_no
};

#endif /* _EXTENSION_H_ */