/*
	Copyright (c) openheap, uplusware
	uplusware@gmail.com
*/

#include "pool.h"
#include "base.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

ThreadPool::ThreadPool(unsigned int size, void(*init_pthread_handler)(), void*(*pthread_handler)(void*), void* arg, int arg_len, void(*exit_pthread_handler)())
{
    m_init_pthread_handler = init_pthread_handler;
    m_pthread_handler = pthread_handler;
	m_exit_pthread_handler = exit_pthread_handler;
    
	if(m_init_pthread_handler)
		(*m_init_pthread_handler)();
    
	m_size = size;
    
    if(arg && arg_len > 0)
    {
        m_arg = malloc(arg_len);
        memcpy(m_arg, arg, arg_len);
    }
    else
    {
        m_arg = NULL;
    }
}

void ThreadPool::More()
{
    
    for(int t = 0; t < CHttpBase::m_thread_increase_step; t++)
    {
        pthread_t pthread_instance;
        pthread_attr_t pthread_instance_attr;
    
        PoolArg* pArg = new PoolArg;
        pArg->data = m_arg;
        
        pthread_attr_init(&pthread_instance_attr);
        pthread_attr_setdetachstate (&pthread_instance_attr, PTHREAD_CREATE_DETACHED);
        
        if(pthread_create(&pthread_instance, &pthread_instance_attr, m_pthread_handler, pArg) != 0)
        {
            fprintf(stderr, "Fail to create thread worker: %d/%d\n", pthread_instance, m_size);
        }
        
        pthread_attr_destroy (&pthread_instance_attr);
    }
}

ThreadPool::~ThreadPool()
{
	if(m_exit_pthread_handler)
		(*m_exit_pthread_handler)();
    
    if(m_arg)
        free(m_arg);
}
