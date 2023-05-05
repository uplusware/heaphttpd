/*
        Copyright (c) openheap, uplusware
        uplusware@gmail.com
*/

#ifndef _SESSION_GROUP_H_
#define _SESSION_GROUP_H_

#include "backend_session.h"
#include "session.h"

#include <sys/epoll.h>
#include <map>
using namespace std;

class Session_Group {
 public:
  Session_Group();
  virtual ~Session_Group();

  void Append(int fd, Session* s);
  void Remove(int fd);

  void Processing();

  unsigned int Count();

  int get_epoll_fd() { return m_epoll_fd; }

  map<int, backend_session*>* get_backend_list() { return &m_backend_list; }

 private:
  map<int, Session*> m_session_list;
  map<int, backend_session*> m_backend_list;

  int m_epoll_fd;
  struct epoll_event* m_epoll_events;
};

#endif /* _SESSION_GROUP_H_ */