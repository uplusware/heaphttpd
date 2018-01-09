/*
	Copyright (c) openheap, uplusware
	uplusware@gmail.com
*/

#ifndef _HTTP_REQUEST_H_
#define _HTTP_REQUEST_H_

#include "http.h"

class http_request
{
public:
    http_request(CHttp* session);
    virtual ~http_request();
    
    const char* get_postdata_var(const char* key);
    const char* get_querystring_var(const char* key);    
    const char* get_cookie_var(const char* key);
    
    const char* get_session_var(const char* key, string& val);
    const char* get_server_var(const char* key, string& val);
    
    void set_cookie_var(const char* key, const char* val,
        int max_age = -1, const char* expires = NULL, const char* path = NULL, const char* domain = NULL,
        BOOL secure = FALSE, BOOL httponly = FALSE);
        
    void set_session_var(const char* key, const char* val);
    void set_server_var(const char* key, const char* val);
    
    void set_service_obj(const char * name, IServiceObj* obj);
    
    int get_multipart_formdata(const char* content_name, string& content_filename, string& content_filetype, const char* &content_valbuf, int& content_vallen);
    
    Http_Method get_method();
    
private:
    CHttp* m_session;
};

#endif /* _HTTP_REQUEST_H_ */