/*
	Copyright (c) openheap, uplusware
	uplusware@gmail.com
*/
#ifndef _SERVICE_OBJ_MAP_H_
#define _SERVICE_OBJ_MAP_H_

#include <pthread.h>
#include <map>
#include "serviceobj.h"

using namespace std;

/**********************************************************************/
/* The ServiceObject's inherit must use 'new' to create the instance  */
/* It's for service scope object, such as DB connection.              */
/**********************************************************************/

class ServiceObjMap
{
public:
    ServiceObjMap()
    {
        pthread_rwlock_init(&m_lock, NULL);
    }
    virtual ~ServiceObjMap()
    {
        pthread_rwlock_destroy(&m_lock);
    }
        
    void SetObj(const char* objname, IServiceObj* objptr)
    {
        pthread_rwlock_wrlock(&m_lock);
        m_objmap[objname] = objptr;
        pthread_rwlock_unlock(&m_lock);
    }
    
    IServiceObj* GetObj(const char* objname)
    {
         pthread_rwlock_rdlock(&m_lock);
        if(m_objmap.find(objname) == m_objmap.end())
        {
           pthread_rwlock_unlock(&m_lock);
           return NULL;
        }
        else
        {
           IServiceObj* obj = m_objmap[objname];
           pthread_rwlock_unlock(&m_lock);
           return obj;
        }
    }
	
	void ReleaseAll()
	{
		map<string, IServiceObj*>::iterator it;
		for(it = m_objmap.begin(); it != m_objmap.end(); ++it)
		{
			IServiceObj* obj = it->second;
			if(obj)
			{
				obj->Destroy();
			}
		}
	}
private:
    pthread_rwlock_t m_lock;
    map<string, IServiceObj*> m_objmap;   
};

#endif /* _SERVICE_OBJ_MAP_H_ */