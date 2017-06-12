/*
	Copyright (c) openheap, uplusware
	uplusware@gmail.com
*/

#include "webcgi.h"

WebCGI::WebCGI()
{
     m_meta_var.clear();
     m_meta_var.insert(map<string, string>::value_type("AUTH_TYPE", ""));
     m_meta_var.insert(map<string, string>::value_type("CONTENT_LENGTH", ""));
     m_meta_var.insert(map<string, string>::value_type("CONTENT_TYPE", ""));
     m_meta_var.insert(map<string, string>::value_type("GATEWAY_INTERFA", "CGI/1.1"));
     m_meta_var.insert(map<string, string>::value_type("PATH_INFO", ""));
     m_meta_var.insert(map<string, string>::value_type("PATH_TRANSLATED", ""));
     m_meta_var.insert(map<string, string>::value_type("QUERY_STRING", ""));
     m_meta_var.insert(map<string, string>::value_type("REMOTE_ADDR", ""));
     m_meta_var.insert(map<string, string>::value_type("REMOTE_HOST", ""));
     m_meta_var.insert(map<string, string>::value_type("REMOTE_IDENT", ""));
     m_meta_var.insert(map<string, string>::value_type("REMOTE_USER", ""));
     m_meta_var.insert(map<string, string>::value_type("REQUEST_METHOD", ""));
     m_meta_var.insert(map<string, string>::value_type("SCRIPT_NAME", ""));
     m_meta_var.insert(map<string, string>::value_type("SERVER_NAME", ""));
     m_meta_var.insert(map<string, string>::value_type("SERVER_PORT", ""));
     m_meta_var.insert(map<string, string>::value_type("SERVER_PROTOCOL", "HTTP/1.1"));
     m_meta_var.insert(map<string, string>::value_type("SERVER_SOFTWARE", "Heaphttpd web server/1.0"));
}
void WebCGI::SetMeta(const char* varname, const char* varvalue)
{
    m_meta_var[varname] = varvalue;
}

void WebCGI::SetData(const char* data, unsigned int len)
{
	m_data = data;
	m_data_len = len;
}