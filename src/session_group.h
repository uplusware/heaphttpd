/*
	Copyright (c) openheap, uplusware
	uplusware@gmail.com
*/

#ifndef _SESSION_GROUP_H_
#define _SESSION_GROUP_H_

#include "session.h"
#include <sys/epoll.h>
#include <map>
using namespace std;

class Session_Group
{
public:
    Session_Group();
    virtual ~Session_Group();
    
    void Append(int fd, Session* s);
    void Remove(int fd);

    void Processing();
    
    int GetSessionCount() { return m_session_list.size(); }
    
    int Get_epoll_fd() { return m_epoll_fd; }
private:
    map<int, Session*> m_session_list;
    
    int m_epoll_fd;
    struct epoll_event * m_epoll_events;
};

#endif /* _SESSION_GROUP_H_ */