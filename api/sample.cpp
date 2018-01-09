/*
	Copyright (c) openheap, uplusware
	uplusware@gmail.com
*/
#include <stdio.h>
#include "sample.h"
#include "version.h"

//API sample
void ApiSample::Response()
{
    CHttpResponseHdr header;
	string strResp;
	string abc, def;
    abc = m_request->get_postdata_var("abc") ? m_request->get_postdata_var("abc") : "";
    def = m_request->get_postdata_var("def") ? m_request->get_postdata_var("def") : "";
	if( abc == "" || def == "" )
	{
       
        header.SetStatusCode(SC200);
		header.SetField("Content-Type", "text/html");
		strResp = "<html><head><title>API Sample</title></head><body>API Sample: couldn't get the corresponding POST data</body></html>";
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
		
		m_request->set_cookie_var("test1", "value1", 90, NULL, "/", NULL, FALSE, FALSE);
        m_request->set_cookie_var("test2", "value2", 100, NULL, "/", NULL, TRUE, TRUE);
        m_request->set_cookie_var("test3", "value3", 120, NULL, "/", ".uplusware.com", TRUE, TRUE);
        m_request->set_cookie_var("test4", "value4", -1, NULL, "/", NULL);
		
		string strValue1, strValue2, strValue3;
		const char* r1 = m_request->get_session_var("session_var1", strValue1);
		const char* r2 = m_request->get_session_var("session_var2", strValue2);
		const char* r3 = m_request->get_session_var("session_var3", strValue3);
		if(r1 != NULL)
    		m_request->set_session_var("session_var1", "hello session 1! = ] <script>alert(\"aaaa\")</script>");
    	if(r2 != NULL)
        	m_request->set_session_var("session_var2", "hello session 2! = ]");
        if(r3 != NULL)
        	m_request->set_session_var("session_var3", "hello session 3! = ]");
    	
    	string strServerValue1, strServerValue2, strServerValue3;
    	const char* s1 = m_request->get_server_var("server_var1", strServerValue1);
		const char* s2 = m_request->get_server_var("server_var2", strServerValue2);
		const char* s3 = m_request->get_server_var("server_var3", strServerValue3);
		if(s1 != NULL)
    		m_request->set_server_var("server_var1", "hello server 1! = ]");
    	if(s2 != NULL)
        	m_request->set_server_var("server_var2", "hello server 2! = ]");
        if(s3 != NULL)
        	m_request->set_server_var("server_var3", "hello server 3! = ]");
        
        string strEscapedValue1, strEscapedValue2, strEscapedValue3;
        string strEscapedServerValue1, strEscapedServerValue2, strEscapedServerValue3;
        escape_HTML(strValue1.c_str(), strEscapedValue1);
        escape_HTML(strValue2.c_str(), strEscapedValue2);
        escape_HTML(strValue3.c_str(), strEscapedValue3);
        
        escape_HTML(strServerValue1.c_str(), strEscapedServerValue1);
        escape_HTML(strServerValue2.c_str(), strEscapedServerValue2);
        escape_HTML(strServerValue3.c_str(), strEscapedServerValue3);
        
		strResp = "<html></head><title>API Sample</title></head><body><h1>"VERSION_STRING"</h1>API Sample: ";
		strResp += abc;
		strResp += " + ";
		strResp += def;
		strResp += " = ";
		strResp += szTmp;
		strResp += "<p><a href='javascript:alert(document.cookie)'>cookie</a>";
		strResp += "<p>Session Variables: <p>session_var1=";
		strResp += strEscapedValue1;
		strResp += "<p>session_var2=";
		strResp += strEscapedValue2;
		strResp += "<p>session_var3=";
		strResp += strEscapedValue3;
		strResp += "<p>Server Variables: <p>server_var1=";
		strResp += strEscapedServerValue1;
		strResp += "<p>server_var2=";
		strResp += strEscapedServerValue2;
		strResp += "<p>server_var3=";
		strResp += strEscapedServerValue3;
		strResp += "</body></html>";
        header.SetField("Content-Length", strResp.length());
	}
    m_response->send_header(header.Text(), header.Length());
	m_response->send_content(strResp.c_str(), strResp.length());

}

void* api_sample_response(http_request* request, http_response *response)
{
	ApiSample *api_inst = new ApiSample(request, response);
	api_inst->Response();
	delete api_inst;
}
