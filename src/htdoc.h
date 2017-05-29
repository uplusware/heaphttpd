/*
	Copyright (c) openheap, uplusware
	uplusware@gmail.com
*/

#ifndef _HTDOC_H_
#define _HTDOC_H_

#include <mqueue.h>
#include <semaphore.h>
#include <dlfcn.h>
#include "fstring.h"
#include "http.h"
#include "util/base64.h"
#include "util/qp.h"
#include "util/escape.h"
#include "cache.h"

typedef struct
{
	string tmpname;
	string filename;
	string filetype1;
	string filetype2;
	unsigned int size;

} AttInfo;

class Htdoc
{
protected:
	CHttp* m_session; 
	string m_work_path;
	string m_php_mode;
    cgi_socket_t m_fpm_socktype;
    string m_fpm_sockfile;
	string m_fpm_addr;
	unsigned short m_fpm_port;

    map<string, cgi_cfg_t>* m_cgi_list;
    vector<string>* m_default_webpages;
    
	string m_phpcgi_path;
	
public:
	Htdoc(CHttp* session, const char* work_path, vector<string>* default_webpages, const char* php_mode, 
        cgi_socket_t fpm_socktype, const char* fpm_sockfile, 
        const char* fpm_addr, unsigned short fpm_port, const char* phpcgi_path,
        map<string, cgi_cfg_t>* cgi_list)
	{
		m_session = session;
        m_work_path = work_path;
        m_default_webpages = default_webpages;
		m_php_mode = php_mode;
        m_fpm_socktype = fpm_socktype;
        m_fpm_sockfile = fpm_sockfile;
		m_fpm_addr = fpm_addr;
		m_fpm_port = fpm_port;
        m_phpcgi_path = phpcgi_path;
        
        m_cgi_list = cgi_list;
	}
	virtual ~Htdoc()
	{
		
	}
	void Response();	
};
#endif /* _HTDOC_H_ */

