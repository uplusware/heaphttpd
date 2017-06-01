/*
	Copyright (c) openheap, uplusware
	uplusware@gmail.com
*/

#ifndef _SERVICE_OBJ_H_
#define _SERVICE_OBJ_H_

class IServiceObj
{
public:
    IServiceObj() { };
    void Destroy() { delete this; };
protected:
    virtual ~IServiceObj() = 0;
};

#endif /* _SERVICE_OBJ_H_ */