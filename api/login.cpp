/*
	Copyright (c) openheap, uplusware
	uplusware@gmail.com
*/
#include <stdio.h>
#include "login.h"
#include "util/security.h"

//API sample
void ApiLogin::Response()
{
	
	StorageEngine* stg_engine = (StorageEngine*)m_session->GetServiceObject("root@mysql_database_connection");
	if(stg_engine == NULL)
	{
		StorageEngine* stg_engine = new StorageEngine("localhost", "root", "123456", "erisemail_db", "UTF-8", "/var/erisemail/private", 8);
		m_session->SetServiceObject("root@mysql_database_connection", stg_engine);		
	}
	
	database db_conn(stg_engine);
	
    CHttpResponseHdr header;
	string strResp;
	string username, password;		
    username = m_session->_POST_VARS_["USER_NAME"];
    password = m_session->_POST_VARS_["USER_PWD"];
	if( username == "" || password == "" )
	{
       
        header.SetStatusCode(SC200);
		header.SetField("Content-Type", "text/html");
		strResp = "<?xml version='1.0' encoding='UTF-8'?>";
		strResp += "<erisemail>"
				"<response errno=\"1\" reason=\"Authenticate Failed\"></response>"
				"</erisemail>";		
		header.SetField("Content-Length", strResp.length());
	}
	else
	{    
		header.SetStatusCode(SC200);
		header.SetField("Content-Type", "text/html");
		if(db_conn.m_mailStg && db_conn.m_mailStg->CheckLogin(username.c_str(), password.c_str()) == 0)
		{
			string name_and_pwd = username;
			name_and_pwd += ":";
			name_and_pwd += password;
			
			string strCookie, strOut;
			Security::Encrypt(name_and_pwd.c_str(), name_and_pwd.length(), strCookie);
			
			header.SetCookie("AUTH_TOKEN", strCookie.c_str(), "/");
			
			strResp = "<?xml version='1.0' encoding='UTF-8'?>";
			strResp += "<erisemail>"
				"<response errno=\"0\" reason=\"\"></response>"
				"</erisemail>";
		}
		else
		{
			strResp = "<?xml version='1.0' encoding='UTF-8'?>";
			strResp += "<erisemail>"
				"<response errno=\"1\" reason=\"Authenticate Failed\"></response>"
				"</erisemail>";
		}
	}
    m_session->HttpSend(header.Text(), header.Length());
	m_session->HttpSend(strResp.c_str(), strResp.length());

}

void* api_login_response(CHttp* session, const char* html_path)
{
	ApiLogin *pDoc = new ApiLogin(session, html_path);
	pDoc->Response();
	delete pDoc;
}
