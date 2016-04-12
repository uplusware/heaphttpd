#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "storage.h"
#include <string.h>

static const char *server_args[] = {
	"this_program", /* this string is not used */
	"--datadir=.",
	"--key_buffer_size=32M"
};

static const char *server_groups[] = {
	"embedded",
	"server",
	"this_program_SERVER",
	(char *)NULL
	};

void DBStorage::SqlSafetyString(string& strInOut)
{
	char * szOut = new char[strInOut.length()* 2 + 1];
	mysql_escape_string(szOut, strInOut.c_str(), strInOut.length());
	strInOut = szOut;
	delete szOut;
}

DBStorage::DBStorage(const char* encoding, const char* private_path)
{
    m_encoding = encoding;
    m_private_path = private_path;
    
    
	m_bOpened = FALSE;
	if(mysql_library_init(sizeof(server_args)/sizeof(char *), (char**)server_args, (char**)server_groups))
	{
		exit(1);
	}
	
    pthread_mutex_init(&m_thread_pool_mutex, NULL);
}

DBStorage::~DBStorage()
{
	Close();
	mysql_library_end();
    pthread_mutex_destroy(&m_thread_pool_mutex);
}

int DBStorage::Connect(const char * host, const char* username, const char* password, const char* database)
{
	if(m_bOpened)
	{
		return 0;
	}
	else
	{
		mysql_thread_init();
                m_hMySQL = mysql_init(NULL);
		if(mysql_real_connect(m_hMySQL, host, username, password, database, 0 ,NULL ,0) != NULL)
		{
			m_host = host;
			m_username = username;
			m_password = password;
			if(database != NULL)
				m_database = database;
			
			mysql_set_character_set(m_hMySQL, m_encoding.c_str());
			m_bOpened = TRUE;
			return 0;
		}
		else
		{
			m_bOpened = FALSE;
			m_hMySQL = NULL;
			printf("mysql_real_connect %s\n", mysql_error(m_hMySQL));
			return -1;	
		}
	}
}

void DBStorage::Close()
{
	if(m_bOpened)
	{
		mysql_close(m_hMySQL);
		mysql_thread_end();
		m_bOpened = FALSE;
		m_bOpened = FALSE;
	}
}

int DBStorage::Ping()
{
	if(m_bOpened)
	{
		return mysql_ping(m_hMySQL);
	}
	else
	{
		return -1;
	}
}

void DBStorage::KeepLive()
{
	if(Ping() != 0)
	{
		Close();
		Connect(m_host.c_str(), m_username.c_str(), m_password.c_str(), m_database.c_str());
	}
}

void DBStorage::EntryThread()
{
	mysql_thread_init();
}

void DBStorage::LeaveThread()
{
	mysql_thread_end();
}

int DBStorage::ShowDatabases(string &databases)
{
	char sqlcmd[1024];
    //Transaction begin
	mysql_autocommit(m_hMySQL, 0);
	sprintf(sqlcmd,"show databases");
	mysql_thread_real_query(m_hMySQL, sqlcmd, strlen(sqlcmd));
    MYSQL_RES *qResult;
	MYSQL_ROW row;
	qResult = mysql_store_result(m_hMySQL);
    if(qResult)
    {			
        while((row = mysql_fetch_row(qResult)))
        {
            databases += row[0];
            databases += " ";
        }
        mysql_free_result(qResult);
		return 0;
    }
    
    databases = mysql_error(m_hMySQL);
    
	return -1;
        
}
