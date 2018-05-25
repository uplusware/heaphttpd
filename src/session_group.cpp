#include "session_group.h"

Session_Group::Session_Group()
{    
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
    return m_session_list.size();
}

void Session_Group::Append(int fd, Session* s)
{
    int flags = fcntl(fd, F_GETFL, 0); 
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    
    struct epoll_event event; 
    event.data.fd = fd;  
	event.events = EPOLLIN | EPOLLHUP | EPOLLERR; 
    int epoll_r = epoll_ctl(m_epoll_fd, EPOLL_CTL_ADD, fd, &event);
    
    if (epoll_r == -1)  
    {  
        fprintf(stderr, "%s %u# epoll_ctl: %s\n", __FILE__, __LINE__, strerror(errno));
    }
    else
    {
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
    }
    
}

void Session_Group::Remove(int fd)
{
    map<int, Session*>::iterator iter = m_session_list.find(fd);
    if(iter != m_session_list.end())
    {
        if(iter->second)
        {
            if(iter->second->get_http_protocol_instance()
                && iter->second->get_http_protocol_instance()->GetHttpTunneling()
                && iter->second->get_http_protocol_instance()->GetHttpTunneling()->get_backend_sockfd() != -1)
            {
                map<int, backend_session*>::iterator iter2 = m_backend_list.find(iter->second->get_http_protocol_instance()->GetHttpTunneling()->get_backend_sockfd());
                if(iter2 != m_backend_list.end())
                {   
                    m_backend_list.erase(iter2);
                }
                close(iter->second->get_http_protocol_instance()->GetHttpTunneling()->get_backend_sockfd());
            }
            
            delete iter->second;    
        }
        
        m_session_list.erase(iter);
        
        epoll_ctl (m_epoll_fd, EPOLL_CTL_DEL, fd, NULL);
    }
}

void Session_Group::Processing()
{
    int n, i;
    n = epoll_wait (m_epoll_fd, m_epoll_events, 65535*2, 0);
    
    for (i = 0; i < n; i++)  
    {
        if (m_epoll_events[i].events & EPOLLOUT)
        {
            
            map<int, Session*>::iterator iter = m_session_list.find(m_epoll_events[i].data.fd);
            if(iter != m_session_list.end() && iter->second)
            {
                if(iter->second->AsyncSend() < 0)
                {
                    Remove(m_epoll_events[i].data.fd);
                    close(m_epoll_events[i].data.fd);
                }
                else
                {
                    iter->second->AsyncProcessing();
                }
            }
            else
            {
                map<int, backend_session*>::iterator iter2 = m_backend_list.find(m_epoll_events[i].data.fd);
                if(iter2 != m_backend_list.end())
                {
                    if(iter2->second)
                    {
                        if(iter2->second->backend_async_flush() < 0)
                        {
                            epoll_ctl (m_epoll_fd, EPOLL_CTL_DEL, m_epoll_events[i].data.fd, NULL);
                            close(m_epoll_events[i].data.fd);
                        }
                        else
                        {
                            iter2->second->async_processing();
                        }
                    }
                    else
                    {
                        epoll_ctl (m_epoll_fd, EPOLL_CTL_DEL, m_epoll_events[i].data.fd, NULL);
                        close(m_epoll_events[i].data.fd);
                    }
                }
            }
        }
        if (m_epoll_events[i].events & EPOLLIN)
        {
            map<int, Session*>::iterator iter = m_session_list.find(m_epoll_events[i].data.fd);
            if(iter != m_session_list.end() && iter->second)
            {
                if(iter->second->AsyncRecv() < 0)
                {
                     Remove(m_epoll_events[i].data.fd);
                     close(m_epoll_events[i].data.fd);
                }
                else
                {
                    iter->second->AsyncProcessing();
                }
            }
            else
            {
                map<int, backend_session*>::iterator iter2 = m_backend_list.find(m_epoll_events[i].data.fd);
                if(iter2 != m_backend_list.end())
                {
                    if(iter2->second)
                    {
                        if(iter2->second->backend_async_recv() < 0)
                        {
                            epoll_ctl (m_epoll_fd, EPOLL_CTL_DEL, m_epoll_events[i].data.fd, NULL);
                            close(m_epoll_events[i].data.fd);
                        }
                        else
                        {
                            iter2->second->async_processing();
                        }
                    }
                    else
                    {
                        epoll_ctl (m_epoll_fd, EPOLL_CTL_DEL, m_epoll_events[i].data.fd, NULL);
                        close(m_epoll_events[i].data.fd);
                    }
                }
            } 
        }
        if (m_epoll_events[i].events & EPOLLHUP || m_epoll_events[i].events & EPOLLERR)
        {
            epoll_ctl (m_epoll_fd, EPOLL_CTL_DEL, m_epoll_events[i].data.fd, NULL);
            close(m_epoll_events[i].data.fd);
            
            map<int, Session*>::iterator iter = m_session_list.find(m_epoll_events[i].data.fd);
            if(iter != m_session_list.end() && iter->second)
            {
                Remove(m_epoll_events[i].data.fd);
            }
            else
            {
                map<int, backend_session*>::iterator iter2 = m_backend_list.find(m_epoll_events[i].data.fd);
                if(iter2 != m_backend_list.end())
                {   
                    m_backend_list.erase(iter2);
                }
            }
        }
    }
}