/*
	Copyright (c) openheap, uplusware
	uplusware@gmail.com
*/
#include "httpresponse.h"

http_response::http_response(CHttp* session)
{
    m_session = session;
}

http_response::~http_response()
{
    
}

void http_response::send_header(const char* h, int l)
{
    m_session->SendHeader(h, l);
}

void http_response::send_content(const char* c, int l)
{
	m_session->SendContent(c, l);
}