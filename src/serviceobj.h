/*
	Copyright (c) openheap, uplusware
	uplusware@gmail.com
*/

#ifndef _SERVICE_OBJ_H_
#define _SERVICE_OBJ_H_
#include <pthread.h>
#include <map>
using namespace std;

/**********************************************************************/
/* The ServiceObject's inherit must use 'new' to create the instance  */
/* It's for service scope object, such as DB connection.              */
/**********************************************************************/
#define SERVICE_OBJECT_HANDLE void*

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
        
    void SetObj(const char* objname, SERVICE_OBJECT_HANDLE objptr)
    {
        pthread_rwlock_wrlock(&m_lock);
        m_objmap[objname] = objptr;
        pthread_rwlock_unlock(&m_lock);
    }
    
    void* GetObj(const char* objname)
    {
         pthread_rwlock_rdlock(&m_lock);
        if(m_objmap.find(objname) == m_objmap.end())
        {
           pthread_rwlock_unlock(&m_lock);
           return NULL;
        }
        else
        {
           SERVICE_OBJECT_HANDLE obj = m_objmap[objname];
           pthread_rwlock_unlock(&m_lock);
           return obj;
        }
    }
	
	void ReleaseAll()
	{
		map<string, void*>::iterator it;
		for(it = m_objmap.begin(); it != m_objmap.end(); ++it)
		{
			void* obj = it->second;
			if(obj)
			{
				delete obj;
			}
		}
	}
private:
    pthread_rwlock_t m_lock;
    map<string, void*> m_objmap;   
};

#endif /* _SERVICE_OBJ_H_ */