/*
	Copyright (c) openheap, uplusware
	uplusware@gmail.com
*/
#include <stdio.h>
#include "mongodemo.h"

//API sample
void ApiMongoDemo::Response()
{
	StorageEngine* stg_engine = (StorageEngine*)m_session->GetServiceObject("root@mysql_database_connection");
	if(stg_engine == NULL)
	{
		stg_engine = new StorageEngine("localhost", "", "", "test", "UTF-8", "/var/niuhttpd/private", 8);
		m_session->SetServiceObject("root@mongo_database_connection", stg_engine);		
	}
    
    CHttpResponseHdr header;
	database db_conn(stg_engine);
	string strDatabases;
    if(db_conn.storage() && db_conn.storage()->ShowDatabases(strDatabases) == 0)
    {
        strDatabases = "<html><head><title>CGI sample</title></head><h1>niuhttpd web server/0.3</h1>Show databases: " + strDatabases;
        strDatabases += "</body></html>";
        
        header.SetStatusCode(SC200);
		header.SetField("Content-Type", "text/html");
		header.SetField("Content-Length", strDatabases.length());
    }
    else
    {
        strDatabases = header.GetDefaultHTML();
        header.SetStatusCode(SC500);
		header.SetField("Content-Type", "text/html");
		header.SetField("Content-Length", strDatabases.length());
    }
    
    m_session->HttpSend(header.Text(), header.Length());
	m_session->HttpSend(strDatabases.c_str(), strDatabases.length());

}

void* api_mongodemo_response(CHttp* session, const char* html_path)
{
	ApiMongoDemo *pDoc = new ApiMongoDemo(session, html_path);
	pDoc->Response();
	delete pDoc;
}
