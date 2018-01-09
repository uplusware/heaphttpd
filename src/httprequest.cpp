/*
	Copyright (c) openheap, uplusware
	uplusware@gmail.com
*/
#include "httprequest.h"

http_request::http_request(CHttp* session)
{
    m_session = session;
}

http_request::~http_request()
{
    
}

Http_Method http_request::get_method()
{
    return m_session->GetMethod();
}
const char* http_request::get_postdata_var(const char* key)
{
    return m_session->GetPostDataVar(key);
}

const char* http_request::get_querystring_var(const char* key)
{
    return m_session->GetQueryStringVar(key);
}

const char* http_request::get_cookie_var(const char* key)
{
    return m_session->GetCookieVar(key);
}

const char* http_request::get_session_var(const char* key, string& val)
{
   if(m_session->GetSessionVar(key, val) < 0)
   {
       return NULL;
   }
   else
   {
       return val.c_str();
   }
}

const char* http_request::get_server_var(const char* key, string& val)
{
   if(m_session->GetServerVar(key, val) < 0)
   {
       return NULL;
   }
   else
   {
       return val.c_str();
   }
}

void http_request::set_cookie_var(const char* key, const char* val,
        int max_age, const char* expires,
        const char* path, const char* domain, 
        BOOL secure, BOOL httponly)
{
    m_session->SetCookie(key, val, max_age, expires, path, domain, secure, httponly);
}

void http_request::set_session_var(const char* key, const char* val)
{
    m_session->SetSessionVar(key, val);
}

void http_request::set_server_var(const char* key, const char* val)
{
    m_session->SetServerVar(key, val);
}

void http_request::set_service_obj(const char * name, IServiceObj* obj)
{
    m_session->SetServiceObject(name, obj);		
}

int http_request::get_multipart_formdata(const char* content_name, string& content_filename, string& content_filetype, const char* &content_valbuf, int& content_vallen)
{
    return m_session->parse_multipart_formdata(content_name, content_filename, content_filetype, content_valbuf, content_vallen);
}