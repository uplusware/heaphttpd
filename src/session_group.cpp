#include "session_group.h"

Session_Group::Session_Group()
{
    m_session_count = 0;
    
    m_epoll_fd = epoll_create1(0);
    if (m_epoll_fd == -1)  
    {
        fprintf(stderr, "%s %u# epoll_create1: %s\n", __FILE__, __LINE__, strerror(errno));
        return;  
    }
    m_epoll_events = new struct epoll_event[65535*2];
    memset(m_epoll_events, 0, sizeof(struct epoll_event) * 65535*2);
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

unsigned int Session_Group::Count()
{
    return m_session_count;
}

void Session_Group::Append(int fd, Session* s)
{
    //printf("append %d\n", fd);
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
	event.events = EPOLLIN; 
    int r = epoll_ctl(m_epoll_fd, EPOLL_CTL_ADD, fd, &event);    
    if (r == -1)  
    {  
        fprintf(stderr, "%s %u# epoll_ctl: %s\n", __FILE__, __LINE__, strerror(errno));
        return;
    }
    m_session_count++;
}

void Session_Group::Remove(int fd)
{
    //printf("remove %d\n", fd);
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
        m_session_count--;
    }
}

void Session_Group::Processing()
{
    int n, i;
    n = epoll_wait (m_epoll_fd, m_epoll_events, 65535*2, 0);
    
    if(n == 0 || (n == -1 && errno == EINTR))
    {
        for(map<int, Session*>::iterator iter = m_session_list.begin(); iter != m_session_list.end(); iter++)
        {
            if(iter->second)
            {
                Http_Connection r = iter->second->AsyncProcessing();
                if(r == httpClose || r == httpKeepAlive)
                {
                    Remove(iter->second->get_sock_fd());
                }
            }
        }
    }
    
    for (i = 0; i < n; i++)  
    {
        if (m_epoll_events[i].events & EPOLLOUT)
        {
            //printf("EPOLLOUT\n");
            map<int, Session*>::iterator iter = m_session_list.find(m_epoll_events[i].data.fd);
            if(iter != m_session_list.end() && iter->second)
            {
                if(iter->second->AsyncSend() < 0)
                {
                    Remove(m_epoll_events[i].data.fd);
                }
            }
        }
        else if (m_epoll_events[i].events & EPOLLIN)
        {
            //printf("EPOLLIN\n");
            map<int, Session*>::iterator iter = m_session_list.find(m_epoll_events[i].data.fd);
            if(iter != m_session_list.end() && iter->second)
            {
                if(iter->second->AsyncRecv() < 0 || iter->second->AsyncProcessing() == httpClose)
                {
                    Remove(m_epoll_events[i].data.fd);
                }
            }
            
        }
        else if (m_epoll_events[i].events & EPOLLHUP || m_epoll_events[i].events & EPOLLERR)
        {
            //printf("EPOLLHUP EPOLLERR\n");
            map<int, Session*>::iterator iter = m_session_list.find(m_epoll_events[i].data.fd);
            if(iter != m_session_list.end() && iter->second)
            {
                Remove(m_epoll_events[i].data.fd);
            }
        }
    }
}