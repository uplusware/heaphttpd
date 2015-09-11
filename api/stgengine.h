#ifndef _STORAGE_ENGINE_H_
#define _STORAGE_ENGINE_H_
#include "storage.h"

typedef struct
{
	DBStorage* storage;
	BOOL opened;
	BOOL inUse;
	unsigned long cTime; //Cteate Time
	unsigned long lTime; //Last using Time
	unsigned long usedCount;
	pthread_t owner;
}stStorageEngine;


class StorageEngine
{
public:
	StorageEngine(const char * host, const char* username, const char* password, const char* database, 
		const char* encoding, const char* private_path, int maxConn);
	virtual ~StorageEngine();

	DBStorage* WaitEngine(int &index);
	void PostEngine(int index);

	
	string m_host;
	int m_realConn;
	
private:
	string m_username;
	string m_password;
	string m_database;
	
	string m_encoding;
	string m_private_path;
	
	int m_maxConn;
	stStorageEngine* m_engine;
	
	pthread_mutex_t m_engineMutex;
	
};

class StorageEngineInstance
{
public:
	StorageEngineInstance(StorageEngine* engine, DBStorage** storage)
	{
		m_engine = engine;
		m_storage = m_engine->WaitEngine(m_engineIndex);
		*storage = m_storage;
        m_isReleased = FALSE;
	}
	
	virtual ~StorageEngineInstance()
	{
		Release();
	}

	void Release()
	{
        if(!m_isReleased)
        {
            m_engine->PostEngine(m_engineIndex);
            m_isReleased = TRUE;
        }
	}
private:
	StorageEngine* m_engine;
	DBStorage* m_storage;
	int m_engineIndex;
	BOOL m_isReleased;
};
#endif /* _STORAGE_ENGINE_H_ */
