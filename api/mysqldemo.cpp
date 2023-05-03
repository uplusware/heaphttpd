/*
        Copyright (c) openheap, uplusware
        uplusware@gmail.com
*/
#include "mysqldemo.h"
#include <stdio.h>
#include "version.h"

// API sample
#define MYSQL_SERVICE_OBJ_NAME "root$mysql$heaphttpd@"

void ApiMySQLDemo::Response() {
  string strObjName = MYSQL_SERVICE_OBJ_NAME;
  strObjName += "localhost";
  StorageEngine* stg_engine =
      (StorageEngine*)m_request->get_service_obj(strObjName.c_str());
  if (stg_engine == NULL) {
    stg_engine = new StorageEngine("localhost", "root", "123456", "",
                                   "/var/run/mysqld/mysqld.sock", 0, "UTF-8",
                                   "/var/heaphttpd/private");
    m_request->set_service_obj(strObjName.c_str(), stg_engine);
  }

  CHttpResponseHdr header;
  string strDatabases;
  DatabaseStorage* database_instance = stg_engine->Acquire();

  if (database_instance &&
      database_instance->ShowDatabases(strDatabases) == 0) {
    strDatabases =
        "<html><head><title>CGI sample</title></head><h1>" VERSION_STRING
        "</h1><hr>Show databases: " +
        strDatabases;
    strDatabases += "</body></html>";

    header.SetStatusCode(SC200);
    header.SetField("Content-Type", "text/html");
    header.SetField("Content-Length", strDatabases.length());
  } else {
    strDatabases = header.GetDefaultHTML();
    header.SetStatusCode(SC500);
    header.SetField("Content-Type", "text/html");
    header.SetField("Content-Length", strDatabases.length());
  }

  stg_engine->Release();

  m_response->send_header(header.Text(), header.Length());
  m_response->send_content(strDatabases.c_str(), strDatabases.length());
}

void* api_mysqldemo_response(http_request* request, http_response* response) {
  ApiMySQLDemo* api_inst = new ApiMySQLDemo(request, response);
  api_inst->Response();
  delete api_inst;
}
