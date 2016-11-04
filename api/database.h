/*
	Copyright (c) openheap, uplusware
	uplusware@gmail.com
*/
#ifndef _DATABASE_H_
#define _DATABASE_H_

#include "stgengine.h"

class database
{
public:
	database(StorageEngine* stgEngine)
	{	
		m_stgengine_instance = new StorageEngineInstance(stgEngine, &m_stg);
		if(!m_stgengine_instance)
		{
			m_stg = NULL;
		}
	}
	virtual ~database()
	{
		if(m_stgengine_instance)
			delete m_stgengine_instance;
	}
	
	DBStorage* storage() { return m_stg; }
protected:
	StorageEngineInstance* m_stgengine_instance;
    DBStorage* m_stg;
};

#endif /* _DATABASE_H_ */
