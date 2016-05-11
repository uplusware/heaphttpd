/*
	Copyright (c) openheap, uplusware
	uplusware@gmail.com
*/
#include <stdio.h>
#include "sample.h"

//API sample
void ApiSample::Response()
{
    CHttpResponseHdr header;
	string strResp;
	string abc, def;
    abc = m_session->_POST_VARS_["abc"];
    def = m_session->_POST_VARS_["def"];
	if( abc == "" || def == "" )
	{
       
        header.SetStatusCode(SC200);
		header.SetField("Content-Type", "text/html");
		strResp = "<html></head><title>API Sample</title></head><body>API Sample: couldn't get the corresponding POST data</body></html>";
        header.SetField("Content-Length", strResp.length());
	}
	else
	{    
		int a = atoi(abc.c_str());
		int d = atoi(def.c_str());
		int sum = a + d;
		
		char szTmp[64];
		sprintf(szTmp, "%d", sum);
        
		header.SetStatusCode(SC200);
		header.SetField("Content-Type", "text/html");
		
		m_session->SetCookie("test1", "value1", 90, NULL, "/", NULL, FALSE, FALSE);
        m_session->SetCookie("test2", "value2", 100, NULL, "/", NULL, TRUE, TRUE);
        m_session->SetCookie("test3", "value3", 120, NULL, "/", ".uplusware.com", TRUE, TRUE);
        m_session->SetCookie("test4", "value4", -1, NULL, "/", NULL);
		
		string strValue1, strValue2, strValue3;
		int r1 = m_session->GetSessionVar("session_var1", strValue1);
		int r2 = m_session->GetSessionVar("session_var2", strValue2);
		int r3 = m_session->GetSessionVar("session_var3", strValue3);
		if(r1 != 0)
    		m_session->SetSessionVar("session_var1", "hello session 1!");
    	if(r2 != 0)
        	m_session->SetSessionVar("session_var2", "hello session 2!");
        if(r3 != 0)
        	m_session->SetSessionVar("session_var3", "hello session 3!");
    	
    	string strServerValue1, strServerValue2, strServerValue3;
    	int s1 = m_session->GetServerVar("server_var1", strServerValue1);
		int s2 = m_session->GetServerVar("server_var2", strServerValue2);
		int s3 = m_session->GetServerVar("server_var3", strServerValue3);
		if(s1 != 0)
    		m_session->SetServerVar("server_var1", "hello server 1!");
    	if(s2 != 0)
        	m_session->SetServerVar("server_var2", "hello server 2!");
        if(s3 != 0)
        	m_session->SetServerVar("server_var3", "hello server 3!");
        	
		strResp = "<html></head><title>API Sample</title></head><body><h1>niuhttpd web server/0.3</h1>API Sample: ";
		strResp += abc;
		strResp += " + ";
		strResp += def;
		strResp += " = ";
		strResp += szTmp;
		strResp += "<p><a href='javascript:alert(document.cookie)'>cookie</a>";
		strResp += "<p>Session Variables: <p>session_var1=";
		strResp += strValue1;
		strResp += "<p>session_var2=";
		strResp += strValue2;
		strResp += "<p>session_var3=";
		strResp += strValue3;
		strResp += "<p>Server Variables: <p>server_var1=";
		strResp += strServerValue1;
		strResp += "<p>server_var2=";
		strResp += strServerValue2;
		strResp += "<p>server_var3=";
		strResp += strServerValue3;
		strResp += "</body></html>";
        header.SetField("Content-Length", strResp.length());
	}
    m_session->SendHeader(header.Text(), header.Length());
	m_session->SendContent(strResp.c_str(), strResp.length());

}

void* api_sample_response(CHttp* session, const char* html_path)
{
	ApiSample *pDoc = new ApiSample(session, html_path);
	pDoc->Response();
	delete pDoc;
}
