/*
        Copyright (c) openheap, uplusware
        uplusware@gmail.com
*/
#include "perf.h"
#include <stdio.h>
#include <time.h>

// API sample
void ApiPerf::Response() {
  CHttpResponseHdr header;
  string strResp;
  char szTmp[128];
  sprintf(szTmp, "%lu", time(NULL));
  header.SetStatusCode(SC200);
  header.SetField("Content-Type", "text/plain");
  header.SetField("Cache-Control", "max-age=0");
  strResp = szTmp;
  header.SetField("Content-Length", strResp.length());

  m_response->send_header(header.Text(), header.Length());
  m_response->send_content(strResp.c_str(), strResp.length());
}

void* api_perf_response(http_request* request, http_response* response) {
  ApiPerf* api_inst = new ApiPerf(request, response);
  api_inst->Response();
  delete api_inst;
}
