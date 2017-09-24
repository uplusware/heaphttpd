/*
	Copyright (c) openheap, uplusware
	uplusware@gmail.com
*/
#include <stdio.h>
#include <time.h>
#include "perf.h"

//API sample
void ApiPerf::Response()
{
    CHttpResponseHdr header;
	string strResp;
    char szTmp[128];
    sprintf(szTmp, "%lu", time(NULL));
    header.SetStatusCode(SC200);
    header.SetField("Content-Type", "text/plain");
    header.SetField("Cache-Control", "max-age=0");
    strResp = szTmp;
    header.SetField("Content-Length", strResp.length());
    
    m_session->SendHeader(header.Text(), header.Length());
	m_session->SendContent(strResp.c_str(), strResp.length());

}

void* api_perf_response(CHttp* session, const char* html_path)
{
	ApiPerf *pDoc = new ApiPerf(session, html_path);
	pDoc->Response();
	delete pDoc;
}
