/*
	Copyright (c) openheap, uplusware
	uplusware@gmail.com
*/
#include <stdio.h>
#include "stgengine.h"

StorageEngine::StorageEngine(const char * host, const char* username, const char* password, const char* database,
    const char* unix_socket, unsigned short port, const char* encoding, const char* private_path)
{
    m_host = host;
	m_username = username;
	m_password = password;
	m_database = database;
	
	m_encoding = encoding;
	m_private_path = private_path;
	m_unix_socket = unix_socket;
    m_port = port;
    
    m_storage = new DatabaseStorage(m_encoding.c_str(), m_private_path.c_str());
	pthread_mutex_init(&m_engineLock, NULL);
}

StorageEngine::~StorageEngine()
{
	pthread_mutex_lock(&m_engineLock);
    if(m_storage)
        delete m_storage;
	pthread_mutex_unlock(&m_engineLock);
    
	pthread_mutex_destroy(&m_engineLock);
}

DatabaseStorage* StorageEngine::Acquire()
{
    pthread_mutex_lock(&m_engineLock);
    if(m_storage)
    {
        if(m_storage->Ping() != 0)
        {
            if(m_storage->Connect(m_host.c_str(), m_username.c_str(), m_password.c_str(), m_database.c_str(), m_unix_socket.c_str(), 0) == 0)
            {
                return m_storage;
            }
            else
            {
                return NULL;
            }
        }
        else 
            return m_storage;
    }
    else
        return NULL;
}

void StorageEngine::Release()
{
    pthread_mutex_unlock(&m_engineLock);
}
	
