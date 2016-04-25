#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "storage.h"
#include <string.h>

#ifndef _MONGODB_
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

#endif /* _MONGODB_ */

DBStorage::DBStorage(const char* encoding, const char* private_path)
{
    m_encoding = encoding;
    m_private_path = private_path;
    
	m_bOpened = FALSE;
#ifdef _MONGODB_
    mongoc_init();
    m_hMongoDB = NULL;
    m_hDatabase = NULL;
#else
	if(mysql_library_init(sizeof(server_args)/sizeof(char *), (char**)server_args, (char**)server_groups))
	{
		exit(1);
	}
    m_hMySQL = NULL;
#endif /* _MONGODB_*/
    pthread_mutex_init(&m_thread_pool_mutex, NULL);
}

DBStorage::~DBStorage()
{
	Close();
#ifdef _MONGODB_
    mongoc_cleanup();
#else
	mysql_library_end();
#endif /* _MONGODB_ */
    pthread_mutex_destroy(&m_thread_pool_mutex);
}

int DBStorage::Connect(const char * host, const char* username, const char* password, const char* database, unsigned short port)
{
	if(m_bOpened)
	{
		return 0;
	}
	else
	{
	    m_host = host;
        m_port = port;
        m_username = username;
		m_password = password;

        if(database != NULL)
			m_database = database;
			
#ifdef _MONGODB_
        char sz_port[64];
        string mongodb_uri = "mongodb://";
        if(username[0] != '\0' && password[0] != '\0')
        {
            mongodb_uri += username;
            mongodb_uri += ":";
            mongodb_uri += password;
            mongodb_uri += "@";
        }
        mongodb_uri += host;
        if(port != 0)
        {
            sprintf(sz_port, "%d", port);
            mongodb_uri += ":";
            mongodb_uri += sz_port;
        }
        //printf("%s\n", mongodb_uri.c_str());
        
        m_hMongoDB = mongoc_client_new (mongodb_uri.c_str());
        if(!m_hMongoDB)
        {
            return -1;
        }
        if(database != NULL && database[0] != '\0')
        {
            m_hDatabase = mongoc_client_get_database(m_hMongoDB, m_database.c_str());
            if(!m_hDatabase)
            {
                return -1;
            }
        }
        m_bOpened = TRUE;
        return 0;

#else
		mysql_thread_init();
        
        m_hMySQL = mysql_init(NULL);

		if(mysql_real_connect(m_hMySQL, host, username, password, database, port ,NULL ,0) != NULL)
		{			
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
#endif /* _MONGODB_ */
	}
}

void DBStorage::Close()
{
	if(m_bOpened)
	{
#ifdef _MONGODB_
        if(m_hDatabase)
            mongoc_database_destroy (m_hDatabase);
        if(m_hMongoDB)
            mongoc_client_destroy (m_hMongoDB);
        m_hDatabase = NULL;
        m_hMongoDB = NULL;
#else
		//printf("mysql closed\n");
        if(m_hMySQL)
            mysql_close(m_hMySQL);
        m_hMySQL = NULL;
		mysql_thread_end();
#endif /* _MONGODB_ */	
    }
}

int DBStorage::Ping()
{
	if(m_bOpened)
	{
#ifdef _MONGODB_
        bool ret_val;
        bson_error_t error;
        bson_t ping, reply;
        bson_init(&ping);
        bson_append_int32(&ping, "ping", 4, 1);
        
        //printf("database: %s\n", m_database.c_str());
        ret_val = mongoc_client_command_simple (m_hMongoDB, m_database.c_str(), &ping, NULL, &reply, &error);
        if (ret_val)
        {
            bson_iter_t iter;
            const bson_value_t * ok_value;
            if(bson_iter_init (&iter, &reply) && bson_iter_find (&iter, "ok"))
            {
#if 0
                ok_value = bson_iter_value (&iter);
                printf("ok_value->value_type: %d\n", ok_value->value_type);

                if(ok_value && ok_value->value_type == BSON_TYPE_DOUBLE)
                    printf ("OK: %f\n", ok_value->value.v_double);
#endif /* 0 */
                ret_val = true;
            }
            else
            {
                ret_val = false;
            }

#if 0
            char* str = bson_as_json(&reply, NULL);
            fprintf(stdout, "%s\n", str);
            bson_free(str);
#endif /* 0 */
        }
        else
        {
            fprintf(stderr, "Ping failure: %s\n", error.message);
        }

        bson_destroy(&ping);

        return (ret_val ? 0 : -1);
#else
		//printf("mysql ping\n");
		return mysql_ping(m_hMySQL);
#endif /* _MONGODB_ */
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
#ifndef _MONGODB_
	mysql_thread_init();
#endif /* _MONGODB_ */
}

void DBStorage::LeaveThread()
{
#ifndef _MONGODB_
	mysql_thread_end();
#endif /* _MONGODB_ */
}

int DBStorage::ShowDatabases(string &databases)
{
#ifdef _MONGODB_
   bson_error_t error;
   char **str_v;
   unsigned i;

   if((str_v = mongoc_client_get_database_names(m_hMongoDB, &error)))
   {
      for (i = 0; str_v [i]; i++)
      {
         databases += str_v [i];
      }
      bson_strfreev (str_v);
      return 0;
   }
   else
   {
      fprintf (stderr, "mongoc_client_get_database_names: %s\n", error.message);
   }
   return -1;
#else
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
#endif /* _MONGODB_ */
}
