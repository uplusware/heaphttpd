/*
	Copyright (c) openheap, uplusware
	uplusware@gmail.com
*/
#ifndef _MAILSYS_H_
#define _MAILSYS_H_

#define CONFIG_FILTER_PATH	"/etc/niuhttpd/mfilter.xml"
#define CONFIG_FILE_PATH	"/etc/niuhttpd/niuhttpd.conf"
#define PERMIT_FILE_PATH	"/etc/niuhttpd/permit.list"
#define REJECT_FILE_PATH	"/etc/niuhttpd/reject.list"

#include <openssl/rsa.h>     
#include <openssl/crypto.h>
#include <openssl/x509.h>
#include <openssl/pem.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include <openssl/rsa.h>
#include <openssl/engine.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <mqueue.h>
#include <semaphore.h>
#include <fstream>
#include <list>
#include <map>
#include <pthread.h>

#include "util/general.h"
#include "util/base64.h"
#include "extension.h"

using namespace std;

#define ERISE_DES_KEY	"1001101001"

#define MAX_USERNAME_LEN	16
#define MAX_PASSWORD_LEN	16
#define MAX_EMAIL_LEN	5000 //about 5M attachment file

#define MSG_EXIT				0xFF
#define MSG_GLOBAL_RELOAD		0xFE
#define MSG_REJECT_APPEND   	0xFC
#define MSG_ACCESS_RELOAD		0xFB
#define MSG_EXTENSION_RELOAD	0xFA

typedef struct
{
	unsigned char aim;
	unsigned char cmd;
	union
	{
		char spool_uid[256];
		char reject_ip[256];
	} data;
}stQueueMsg;

typedef struct
{
	string ip;
	time_t expire;
}stReject;

class linesock
{
public:
	linesock(int fd)
	{
		dbufsize = 0;
		sockfd = fd;
		dbuf = (char*)malloc(4096);
		if(dbuf)
		{
			dbufsize = 4096;
		}
		dlen = 0;
	}
	
	virtual ~linesock()
	{
		if(dbuf)
			free(dbuf);
	}

	int drecv(char* pbuf, int blen)
	{
		int rlen = 0;
		if(blen <= dlen)
		{
			memcpy(pbuf, dbuf, blen);

			memmove(dbuf, dbuf + blen , dlen - blen);
			dlen = dlen - blen;
			
			rlen = blen;
		}
		else
		{
			
			memcpy(pbuf, dbuf, dlen);
			rlen = dlen;
			dlen = 0;
			
			int len = _Recv_(sockfd, pbuf + rlen, blen - rlen);
			if(len > 0)
			{
				rlen = rlen + len;	
			}
		}

		return rlen;
	}
	
	int lrecv(char* pbuf, int blen)
	{
		int taketime = 0;
		int res;
		fd_set mask; 
		struct timeval timeout; 
		char* p = NULL;
		int len;
		unsigned int nRecv = 0;

		int left;
		int right;
		p = dlen > 0 ? (char*)memchr(dbuf, '\n', dlen) : NULL;
		if(p != NULL)
		{
			left = p - dbuf + 1;
			right = dlen - left;
		
			if(blen >= left)
			{
				memcpy(pbuf, dbuf, left);
				memmove(dbuf, p + 1, right);
				dlen = right;
				pbuf[left] = '\0';
				return left;
			}
			else
			{
				memcpy(pbuf, dbuf, blen);
				memmove(dbuf, dbuf + blen, dlen - blen);
				dlen = dlen - blen;
				return -2;
			}
		}
		else
		{
			if(blen >= dlen)
			{
				memcpy(pbuf, dbuf, dlen);
				nRecv = dlen;
				dlen = 0;
			}
			else
			{
				memcpy(pbuf, dbuf, blen);
				memmove(dbuf, dbuf + blen, dlen - blen);
				dlen = dlen - blen;
				return -2;
			}
		}

		p = NULL;
		FD_ZERO(&mask);		
		while(1)
		{
			if(nRecv >= blen)
				return -2;
			
			timeout.tv_sec = 1; 
			timeout.tv_usec = 0;
					
			FD_SET(sockfd, &mask);
			res = select(sockfd + 1, &mask, NULL, NULL, &timeout);
			//printf("%d %d\n", sockfd, res);
			if( res == 1) 
			{
				taketime = 0;
				len = recv(sockfd, pbuf + nRecv, blen - nRecv, 0);
				//printf("len: %d\n", len);
                if(len == 0)
                {
                    close(sockfd);
                    return -1;
                }
				else if(len < 0)
				{
                    if( errno == EAGAIN)
                        continue;
					close(sockfd);
					return -1;
				}
				nRecv = nRecv + len;
				p = (char*)memchr(pbuf, '\n', nRecv);
				if(p != NULL)
				{
					left = p - pbuf + 1;
					right = nRecv - left;
				
					if(right > dbufsize)
					{
						if(dbuf)
							free(dbuf);
						dbuf = (char*)malloc(right);
						dbufsize = right;
					}
					memcpy(dbuf, p + 1, right);
					dlen = right;
					nRecv = left;
					pbuf[nRecv] = '\0';
					break;
				}
			}
			else if(res == 0)
			{
				taketime = taketime + 1;
				if(taketime > MAX_TRY_TIMEOUT)
				{
					close(sockfd);
					return -1;
				}
				continue;
			}
			else
			{
                //printf("%p: closed\n", this);
				return -1;
			}
			
		}
		return nRecv;
	}

private:
	int sockfd;
public:
	char* dbuf;
	int dlen;
	int dbufsize;
};

class linessl
{
public:
	linessl(int fd, SSL* ssl)
	{
		dbufsize = 0;
		sockfd = fd;
		sslhd = ssl;
		dbuf = (char*)malloc(4096);
		if(dbuf)
		{
			dbufsize = 4096;
		}
		dlen = 0;
	}
	
	virtual ~linessl()
	{
		if(dbuf)
			free(dbuf);
	}

	int drecv(char* pbuf, int blen)
	{
		int rlen = 0;
		
		if(blen <= dlen)
		{
			memcpy(pbuf, dbuf, blen);

			memmove(dbuf, dbuf + blen, dlen - blen);
			dlen = dlen - blen;
			
			rlen = blen;
		}
		else
		{
			
			memcpy(pbuf, dbuf, dlen);
			rlen = dlen;
			dlen = 0;
			
			int len = SSLRead(sockfd, sslhd, pbuf + rlen, blen - rlen);
			if(len > 0)
			{
				rlen = rlen + len;	
			}
		}

		return rlen;
	}
	
	int lrecv(char* pbuf, int blen)
	{
		int taketime = 0;
		int res;
		fd_set mask; 
		struct timeval timeout; 
		char* p = NULL;
		int len;
		unsigned int nRecv = 0;
		int ret;
		
		int left;
		int right;
		p = dlen > 0 ? (char*)memchr(dbuf, '\n', dlen) : NULL;
		if(p != NULL)
		{
			left = p - dbuf + 1;
			right = dlen - left;
		
			if(blen >= left)
			{
				memcpy(pbuf, dbuf, left);
				memmove(dbuf, p + 1, right);
				dlen = right;
				pbuf[left] = '\0';
				return left;
			}
			else
			{
				memcpy(pbuf, dbuf, blen);
				memmove(dbuf, dbuf + blen, dlen - blen);
				dlen = dlen - blen;
				return -2;
			}
		}
		else
		{
			if(blen >= dlen)
			{
				memcpy(pbuf, dbuf, dlen);
				nRecv = dlen;
				dlen = 0;
			}
			else
			{
				memcpy(pbuf, dbuf, blen);
				memmove(dbuf, dbuf + blen, dlen - blen);
				dlen = dlen - blen;
				return -2;
			}
		}

		p = NULL;

#if	1	
        FD_ZERO(&mask);
		while(1)
		{
			if(nRecv >= blen)
				return -2;
			
			timeout.tv_sec = 1; 
			timeout.tv_usec = 0;
					
			FD_SET(sockfd, &mask);
			res = select(sockfd + 1, &mask, NULL, NULL, &timeout);
            
			if( res == 1) 
			{
				taketime = 0;
				len = SSL_read(sslhd, pbuf + nRecv, blen - nRecv);
                if(len == 0)
				{
					ret = SSL_get_error(sslhd, len);
					if(ret == SSL_ERROR_ZERO_RETURN)
					{
						printf("SSL_read: shutdown by the peer\n");
					}
                    close(sockfd);
                    return -1;
                }
				else if(len < 0)
				{
					ret = SSL_get_error(sslhd, len);
					if(ret == SSL_ERROR_WANT_READ || ret == SSL_ERROR_WANT_WRITE)
					{
						continue;
					}
					else
					{
						close(sockfd);
						return -1;
					}
				}
				nRecv = nRecv + len;
				p = (char*)memchr(pbuf, '\n', nRecv);
				if(p != NULL)
				{
					left = p - pbuf + 1;
					right = nRecv - left;
				
					if(right > dbufsize)
					{
						if(dbuf)
							free(dbuf);
						dbuf = (char*)malloc(right);
						dbufsize = right;
					}
					memcpy(dbuf, p + 1, right);
					dlen = right;
					nRecv = left;
					pbuf[nRecv] = '\0';
					break;
				}
			}
			else if(res == 0)
			{
				taketime = taketime + 1;
                //printf("%p: %d %d\n", this, sockfd, taketime);
				if(taketime > MAX_TRY_TIMEOUT)
				{
					close(sockfd);
					return -1;
				}
				continue;
			}
			else
			{
                //printf("%p: closed\n", this);
				return -1;
			}
			
		}
#else
		while(1)
		{
			if(nRecv >= blen)
				return -2;
			
			len = SSL_read(sslhd, pbuf + nRecv, blen - nRecv);
			if(len <= 0)
			{
				ret = SSL_get_error(sslhd, len);
				if(ret == SSL_ERROR_ZERO_RETURN)
				{
					printf("SSL_read: shutdown by the peer\n");
				}
				close(sockfd);
				return -1;
			}
			nRecv = nRecv + len;
			p = (char*)memchr(pbuf, '\n', nRecv);
			if(p != NULL)
			{
				left = p - pbuf + 1;
				right = nRecv - left;
			
				if(right > dbufsize)
				{
					if(dbuf)
						free(dbuf);
					dbuf = (char*)malloc(right);
					dbufsize = right;
				}
				memcpy(dbuf, p + 1, right);
				dlen = right;
				nRecv = left;
				pbuf[nRecv] = '\0';
				break;
			}
		}
#endif
		return nRecv;
	}

private:
	int sockfd;
	SSL* sslhd;
	
public:
	char* dbuf;
	int dlen;
	int dbufsize;
};

enum cgi_socket_t{
        inet_socket = 0,
        unix_socket,
        unknow_socket
};

enum cgi_t{
        none_e = 0,
        fastcgi_e,
        scgi_e,
};

typedef struct {
    string   cgi_name;
    cgi_t    cgi_type;
    string   cgi_pgm;
    cgi_socket_t   cgi_socktype;
	string	 cgi_addr;
	unsigned short cgi_port;
    string   cgi_sockfile;
} cgi_cfg_t;
    
class CHttpBase
{
public:
	static string	m_sw_version;
    static BOOL     m_close_stderr;
    
	static string	m_private_path;
	static string 	m_work_path;
	static string   m_ext_list_file;

	static vector<stExtension> m_ext_list;

	static string	m_localhostname;
	static string	m_encoding;
	static string	m_hostip;

	static BOOL		m_enablehttp;
	static unsigned short	m_httpport;
	
	static BOOL		m_enablehttps;
	static unsigned short	m_httpsport;
	static BOOL     m_enablehttp2;
	static string   m_https_cipher;
    static string   m_http2_tls_cipher;
    
	static string 	m_www_authenticate;
	static BOOL 	m_client_cer_check;
	static string	m_ca_crt_root;
	static string	m_ca_crt_server;
	static string	m_ca_key_server;
	static string	m_ca_crt_client;
	static string	m_ca_key_client;

	static string	m_ca_password;
	
    static string   m_phpcgi_path;
	static string	m_php_mode;
    static cgi_socket_t   m_fpm_socktype;
	static string	m_fpm_addr;
	static unsigned short m_fpm_port;
    static string   m_fpm_sockfile;
    
	static map<string, cgi_cfg_t> m_cgi_list;
    
	static unsigned int 	m_relaytasknum;
	
	static unsigned int	m_max_instance_num;
	static unsigned int	m_max_instance_thread_num;
	static BOOL m_instance_prestart;
    static string m_instance_balance_scheme;
	static unsigned int	m_runtime;

	static string	m_config_file;
	static string	m_permit_list_file;
	static string	m_reject_list_file;
	

	static vector<stReject> m_reject_list;
	static vector<string> m_permit_list;

#ifdef _WITH_MEMCACHED_
    static map<string, int> m_memcached_list;
#endif /* _WITH_MEMCACHED_ */

public:	
	CHttpBase();
	virtual ~CHttpBase();
	
	static void SetConfigFile(const char* config_file, const char* permit_list_file, const char* reject_list_file);

	static BOOL LoadConfig();
	static BOOL UnLoadConfig();

	static BOOL LoadAccessList();
	static BOOL LoadExtensionList();
	
	/* Pure virual function	*/
	virtual BOOL LineParse(char* text) = 0;
	virtual int ProtRecv(char* buf, int len) = 0;

private:
	static void _load_permit_();
	static void _load_reject_();
	static void _load_ext_();
};

#endif /* _MAILSYS_H_ */

