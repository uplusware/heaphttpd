#include "session_group.h"

Session_Group::Session_Group()
{
    m_epoll_fd = epoll_create1(0);
    if (m_epoll_fd == -1)  
    {
        fprintf(stderr, "%s %u# epoll_create1: %s\n", __FILE__, __LINE__, strerror(errno));
        return;  
    }
    m_epoll_events = new struct epoll_event[1024]; 
}

Session_Group::~Session_Group()
{
    map<int, Session*>::iterator iter;
    
    for(iter = m_session_list.begin(); iter != m_session_list.end(); iter++)
    {
        if(iter->second)
            delete iter->second;
    }
    
    if(m_epoll_events)
        delete [] m_epoll_events;
    
    close(m_epoll_fd);
}

void Session_Group::Append(int fd, Session* s)
{
    int flags = fcntl(fd, F_GETFL, 0); 
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
                    
    map<int, Session*>::iterator iter = m_session_list.find(fd);
    if(iter != m_session_list.end())
    {
        if(iter->second)
        {
            delete iter->second;
        }
        iter->second = s;
    }
    else
    {
        m_session_list.insert(map<int, Session*>::value_type(fd, s));
    }
    
    struct epoll_event event; 
    event.data.fd = fd;  
	event.events = EPOLLIN | EPOLLHUP | EPOLLERR; 
    epoll_ctl (m_epoll_fd, EPOLL_CTL_ADD, fd, &event);
    
}

void Session_Group::Remove(int fd)
{
    map<int, Session*>::iterator iter = m_session_list.find(fd);
    if(iter != m_session_list.end())
    {
        if(iter->second)
        {
            delete iter->second;
            iter->second = NULL;
        }
        
        struct epoll_event event; 
        event.data.fd = fd;  
        event.events = EPOLLIN | EPOLLOUT | EPOLLHUP | EPOLLERR; 
        epoll_ctl (m_epoll_fd, EPOLL_CTL_DEL, fd, &event);
        
        close(fd);
    }
}

void Session_Group::Processing()
{
    int n, i;  
	fprintf(stderr, "Processing1 %d \n", n);
    n = epoll_wait (m_epoll_fd, m_epoll_events, 1024, 1000);
    
	fprintf(stderr, "Processing2 %d \n", n);
	
    for (i = 0; i < n; i++)  
    {
        if (m_epoll_events[i].events & EPOLLIN)
        {
            map<int, Session*>::iterator iter = m_session_list.find(m_epoll_events[i].data.fd);
            if(iter != m_session_list.end() && iter->second)
            {
                if(iter->second->AsyncRecv() < 0 || iter->second->AsyncProcessing(m_epoll_fd) == httpClose)
                {
                    Remove(m_epoll_events[i].data.fd);
                }
            }
            
        }
        else if (m_epoll_events[i].events & EPOLLOUT)
        {
            map<int, Session*>::iterator iter = m_session_list.find(m_epoll_events[i].data.fd);
            if(iter != m_session_list.end() && iter->second)
            {
                if(iter->second->AsyncSend() < 0 || iter->second->AsyncProcessing(m_epoll_fd) == httpClose)
                {
                    Remove(m_epoll_events[i].data.fd);
                }
            }
        }
        else if (m_epoll_events[i].events & EPOLLHUP || m_epoll_events[i].events & EPOLLERR)
        {
            map<int, Session*>::iterator iter = m_session_list.find(m_epoll_events[i].data.fd);
            if(iter != m_session_list.end() && iter->second)
            {
                Remove(m_epoll_events[i].data.fd);
            }
        }
    }
}