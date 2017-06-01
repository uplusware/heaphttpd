/*
	Copyright (c) openheap, uplusware
	uplusware@gmail.com
*/
#include <stdio.h>
#include "mongodemo.h"

//API sample
#define MONGO_SERVICE_OBJ_NAME  "root$mongodb$niuhttpd@"

void ApiMongoDemo::Response()
{
	string strObjName = MONGO_SERVICE_OBJ_NAME;
    strObjName += "localhost";
	StorageEngine* stg_engine = (StorageEngine*)m_session->GetServiceObject(strObjName.c_str());
    
	if(stg_engine == NULL)
	{
		stg_engine = new StorageEngine("localhost", "", "", "test", "/var/run/mongod/mongod.sock", 0, "UTF-8", "/var/niuhttpd/private");
		m_session->SetServiceObject(MONGO_SERVICE_OBJ_NAME, stg_engine);		
	}
    
    CHttpResponseHdr header;
	string strDatabases;
    DatabaseStorage* database_instance = stg_engine->Acquire();
    
    if(database_instance && database_instance->ShowDatabases(strDatabases) == 0)
    {
        strDatabases = "<html><head><title>CGI sample</title></head><h1>niuhttpd web server/0.3</h1>Show databases: " + strDatabases;
        strDatabases += "</body></html>";
        
        header.SetStatusCode(SC200);
		header.SetField("Content-Type", "text/html");
		header.SetField("Content-Length", strDatabases.length());
    }
    else
    {
        header.SetStatusCode(SC500);
        strDatabases = header.GetDefaultHTML();
        
		header.SetField("Content-Type", "text/html");
		header.SetField("Content-Length", strDatabases.length());
    }
    
    stg_engine->Release();
    
    m_session->SendHeader(header.Text(), header.Length());
	m_session->SendContent(strDatabases.c_str(), strDatabases.length());

}

void* api_mongodemo_response(CHttp* session, const char* html_path)
{
	ApiMongoDemo *pDoc = new ApiMongoDemo(session, html_path);
	pDoc->Response();
	delete pDoc;
}
