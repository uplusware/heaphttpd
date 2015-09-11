#ifndef _STORAGE_H_
#define _STORAGE_H_

#include <mysql.h>
#include <pthread.h>
#include <string>

#define BOOL    unsigned int
#define TRUE    1
#define FALSE   0

using namespace std; 

class DBStorage
{
public:
	DBStorage(const char* encoding, const char* private_path);
	virtual ~DBStorage();

	//system
	int Connect(const char * host, const char* username, const char* password, const char* database);
	void Close();
	int Ping();
	
	void KeepLive();
	
	void EntryThread();

	void LeaveThread();
	
	int ShowDatabases(string& databases);
	
protected:
	MYSQL* m_hMySQL;

	BOOL m_bOpened;
	void SqlSafetyString(string& strInOut);

	string m_host;
	string m_username;
	string m_password;
	string m_database;
    
    string m_encoding;
    string m_private_path;
    
    pthread_mutex_t m_thread_pool_mutex;
    
private:
    int mysql_thread_real_query(MYSQL *mysql, const char *stmt_str, unsigned long length)
    {
        pthread_mutex_lock(&m_thread_pool_mutex);
        int ret = mysql_real_query(mysql, stmt_str, length);
        pthread_mutex_unlock(&m_thread_pool_mutex);
        return ret;
    }
};
	
#endif /* _STORAGE_H_ */

