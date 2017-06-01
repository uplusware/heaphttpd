/*
	Copyright (c) openheap, uplusware
	uplusware@gmail.com
*/
#ifndef _STORAGE_ENGINE_H_
#define _STORAGE_ENGINE_H_
#include "storage.h"
#include "serviceobj.h"

class StorageEngine : public IServiceObj
{
public:
	StorageEngine(const char * host, const char* username, const char* password, const char* database,
        const char* unix_socket, unsigned short port, const char* encoding, const char* private_path);

	DatabaseStorage* Acquire();
    
	void Release();
protected:
    virtual ~StorageEngine();
    
private:
    string m_host;
	string m_username;
	string m_password;
	string m_database;
	unsigned short m_port;
    string m_unix_socket;
    
	string m_encoding;
	string m_private_path;
    
    DatabaseStorage* m_storage;
	
	pthread_mutex_t m_engineLock;
	
};

#endif /* _STORAGE_ENGINE_H_ */
