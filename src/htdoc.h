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
    string m_fpm_socktype;
    string m_fpm_sockfile;
	string m_fpm_addr;
	unsigned short m_fpm_port;
	
	string m_fastcgi_name;
    string m_fastcgi_pgm;
	string m_fastcgi_socktype;
    string m_fastcgi_sockfile;
	string m_fastcgi_addr;
	unsigned short m_fastcgi_port;
	
	string m_phpcgi_path;
	
public:
	Htdoc(CHttp* session, const char* work_path, const char* php_mode, 
        const char* fpm_socktype, const char* fpm_sockfile, 
        const char* fpm_addr, unsigned short fpm_port, const char* phpcgi_path,
        const char* fastcgi_name, const char* fastcgi_pgm, 
        const char* fastcgi_socktype, const char* fastcgi_sockfile, 
        const char* fastcgi_addr, unsigned short fastcgi_port)
	{
		m_session = session;
        m_work_path = work_path;
		m_php_mode = php_mode;
        m_fpm_socktype = fpm_socktype;
        m_fpm_sockfile = fpm_sockfile;
		m_fpm_addr = fpm_addr;
		m_fpm_port = fpm_port;
        m_phpcgi_path = phpcgi_path;
        
        m_fastcgi_name = fastcgi_name;
        m_fastcgi_pgm = fastcgi_pgm;
        m_fastcgi_socktype = fastcgi_socktype;
        m_fastcgi_sockfile = fastcgi_sockfile;
		m_fastcgi_addr = fastcgi_addr;
		m_fastcgi_port = fastcgi_port;
	}
	virtual ~Htdoc()
	{
		
	}
	void Response();	
};
#endif /* _HTDOC_H_ */

