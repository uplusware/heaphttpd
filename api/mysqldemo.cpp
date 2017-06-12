/*
	Copyright (c) openheap, uplusware
	uplusware@gmail.com
*/
#include <stdio.h>
#include "mysqldemo.h"

//API sample
#define MYSQL_SERVICE_OBJ_NAME  "root$mysql$heaphttpd@"

void ApiMySQLDemo::Response()
{
    string strObjName = MYSQL_SERVICE_OBJ_NAME;
    strObjName += "localhost";
	StorageEngine* stg_engine = (StorageEngine*)m_session->GetServiceObject(strObjName.c_str());
	if(stg_engine == NULL)
	{
		stg_engine = new StorageEngine("localhost", "root", "123456", "", "/var/run/mysqld/mysqld.sock", 0, "UTF-8", "/var/heaphttpd/private");
		m_session->SetServiceObject(strObjName.c_str(), stg_engine);		
	}
    
    
    CHttpResponseHdr header;
	string strDatabases;
    DatabaseStorage* database_instance = stg_engine->Acquire();
    
    if(database_instance && database_instance->ShowDatabases(strDatabases) == 0)
    {
        strDatabases = "<html><head><title>CGI sample</title></head><h1>heaphttpd web server/0.3</h1>Show databases: " + strDatabases;
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
    
    stg_engine->Release();
    
    m_session->SendHeader(header.Text(), header.Length());
	m_session->SendContent(strDatabases.c_str(), strDatabases.length());

}

void* api_mysqldemo_response(CHttp* session, const char* html_path)
{
	ApiMySQLDemo *pDoc = new ApiMySQLDemo(session, html_path);
	pDoc->Response();
	delete pDoc;
}
