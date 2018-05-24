/*
	Copyright (c) openheap, uplusware
	uplusware@gmail.com
*/

#ifndef _BACKEND_SESSION_H_
#define _BACKEND_SESSION_H_

class backend_session
{
public:
    backend_session();
    virtual ~backend_session();
    
    virtual int processing() = 0;
    virtual int async_processing() = 0;
    virtual int backend_async_recv() = 0;
    virtual int backend_async_flush() = 0;
};
#endif /* _BACKEND_SESSION_H_ */