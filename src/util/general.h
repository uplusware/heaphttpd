/*
	Copyright (c) openheap, uplusware
	uplusware@gmail.com
*/

#ifndef _HEAPHTTPD_GENERAL_H_
#define _HEAPHTTPD_GENERAL_H_

#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <ctype.h>
#include <string>
#include <iostream>
#include <vector>
#include <memory>
#include <sys/time.h>
#include <iconv.h>
#include <semaphore.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>
#include <stdint.h>

#include <openssl/rsa.h>     
#include <openssl/crypto.h>
#include <openssl/x509.h>
#include <openssl/pem.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include <openssl/opensslv.h>

#define MAX_MEMORY_THRESHOLD (1024*1024*2)
#define MEMORY_BLOCK_SIZE (1024*64)

using namespace std;

typedef unsigned int BOOL;

#define FALSE   0
#define TRUE    1

typedef  unsigned long long int uint_64;
typedef  unsigned int uint_32;
typedef  unsigned short uint_16;
typedef  unsigned char uint_8;

#define HICH(c) (( (c) >= 'a' )&&((c) <= 'z')) ? ((c)+'A'-'a'):(c)
#define LOCH(c) (( (c) >= 'A' )&&((c) <= 'Z')) ? ((c)-'A'+'a'):(c)

 #if defined  _SOLARIS_OS_ || defined CYGWIN
__inline__ char* strcasestr(const char *s, const char* f)
{
	char c, sc;
	size_t len;
	if ((c = *f++) != 0)
	{
		c = tolower((unsigned char)c);
		len = strlen(f);
		do {
			do
			{
				if ((sc = *s++) == 0)
        			return (NULL);
      		} while ((char)tolower((unsigned char)sc) != c);
    	} while (strncasecmp(s, f, len) != 0);
    	s--;
	}
	return ((char *)s);
}
#endif /* _SOLARIS_OS_ */

int __inline__ checkip(const char *addr)
{
	int q[4];
	char *p;
	int i;
	
	if(sscanf(addr, "%d.%d.%d.%d", &(q[0]), &(q[1]), &(q[2]), &(q[3])) != 4)
	{
		return -1;
	}
	
	if(q[0] > 255 || q[0] < 0 ||
		q[1] > 255 || q[1] < 0 ||
		q[2] > 255 || q[2] < 0 ||
		q[3] > 255 || q[3] < 0)
	{
		return -1;
	}
	
	/* we know there are 3 dots */
	p = (char*)addr;
	if(p != NULL) { p = strchr(p, '.'); p++; }
	if(p != NULL) { p = strchr(p, '.'); p++; }
	if(p != NULL) { p = strchr(p, '.'); }
	for(i=0; *p != '\0' && i<4; i++, p++);
	if(*p != '\0')
	{
		return -1;
	}
	
	return 0;
}

void __inline__ memsplit2str(const char* buf, unsigned int len, const char* sepstr, string& strleft, const char*& pright, unsigned int& right_len)
{
    
    int sep_len = strlen(sepstr);
    
    pright = NULL;
    right_len = 0;
    if(len < sep_len)
    {
        return;
    }
    for(int x = 0; x <= len - sep_len; x++)
    {
        if(memcmp(&buf[x], sepstr, strlen(sepstr)) == 0)
        {
            if(x > 0)
            {
                char* p = (char*)malloc(x + 1);
                memcpy(p, buf, x);
                p[x] = '\0';
                strleft = p;
                free(p);
            }
            pright = &buf[x + sep_len];
            right_len = len - x - sep_len;
            break;
        }
    }
}

void __inline__ strcut(const char* text, const char* begstring, const char* endstring ,string& strDest)
{
	string strText;
	strText = text;
	string::size_type ops1, ops2;
	if(begstring == NULL)
		ops1 = 0;
	else
		ops1 = strText.find(begstring, 0, strlen(begstring));

	if(endstring == NULL)
		ops2 = strText.length();
	else
		ops2 = strText.find(endstring, begstring == NULL ? 0 : ops1 + strlen(begstring), strlen(endstring));

	string::size_type subfirst, sublen;
	if(ops1 == string::npos)
		subfirst = strText.length();
	else
		subfirst = ops1 + (begstring ==  NULL ? 0 : strlen(begstring));
	
	if(ops2 == string::npos)
		sublen = strText.length() - subfirst;
	else
		sublen = ops2- subfirst;
	
	if(sublen <= 0)
		strDest = "";
	else
		strDest = strText.substr(subfirst, sublen);
}

void __inline__ fnfy_strcut(const char* text, const char* begstring, const char* notbegset, const char* endset ,string& strDest)
{
	string strText;
	strText = text;
	string::size_type ops0, ops1, ops2;
	if(begstring == NULL)
		ops0 = 0;
	else
		ops0 = strText.find(begstring, 0, strlen(begstring));

	if(ops0 == string::npos)
	{
		strDest = "";
		return;
	}
	ops1 = strText.find_first_not_of(notbegset, ops0 + (begstring == NULL ? 0: strlen(begstring)));
	ops2 = strText.find_first_of(endset, ops1);

	string::size_type subfirst, sublen;
	
	if(ops1 == string::npos)
		subfirst = strText.length();
	else
		subfirst = ops1;
	
	if(ops2 == string::npos)
		sublen = strText.length() - subfirst;
	else
		sublen = ops2 - subfirst;
	
	if(sublen <= 0)
		strDest = "";
	else
		strDest = strText.substr(subfirst, sublen);
}

void __inline__ fnln_strcut(const char* text, const char* begstring, const char* notbegset, const char* notendset ,string& strDest)
{
	string strText;
	strText = text;
	string::size_type ops0, ops1, ops2;
	if(begstring == NULL)
		ops0 = 0;
	else
		ops0 = strText.find(begstring, 0, strlen(begstring));

	if(ops0 == string::npos)
	{
		strDest = "";
		return;
	}
	
	ops1 = strText.find_first_not_of(notbegset, ops0 + (begstring == NULL ? 0: strlen(begstring)));
	ops2 = strText.find_last_not_of(notendset);

	string::size_type subfirst, sublen;
	
	if(ops1 == string::npos)
		subfirst = strText.length();
	else
		subfirst = ops1;
	
	if(ops2 == string::npos)
		sublen = strText.length() - subfirst;
	else
		sublen = ops2 - subfirst + 1;
	
	if(sublen <= 0)
		strDest = "";
	else
		strDest = strText.substr(subfirst, sublen);
}

bool __inline__ _Recv_Available_(int sockfd)
{
	int res;
	fd_set mask; 
	struct timeval timeout; 
	FD_ZERO(&mask);
	timeout.tv_sec = 0; 
    timeout.tv_usec = 0;

    FD_SET(sockfd, &mask);
    res = select(sockfd + 1, &mask, NULL, NULL, &timeout);
    
    if( res == 1 ) 
    {
        return true;
    }
    else if( res == 0 ) 
    {
        return false;
    }
    else
    {
        close(sockfd);
        return false;
    }
}

int __inline__ _Recv_(int sockfd, char* buf, unsigned int buf_len, unsigned int idle_timeout)
{
	int taketime = 0;
	int res;
	fd_set mask; 
	struct timeval timeout; 
	
	int len;
	unsigned int nRecv = 0;
	FD_ZERO(&mask);
	while(1)
	{
		timeout.tv_sec = idle_timeout; 
		timeout.tv_usec = 0;

		FD_SET(sockfd, &mask);
		res = select(sockfd + 1, &mask, NULL, NULL, &timeout);
		
		if( res == 1) 
		{
			taketime = 0;
			len = recv(sockfd, buf + nRecv, buf_len - nRecv, 0);
			if(len == 0)
            {
                close(sockfd);
                return nRecv; 
            }
			else if(len < 0)
			{
                if( errno == EAGAIN)
                    continue;
				close(sockfd);
				return nRecv;
			}
			nRecv = nRecv + len;
			if(nRecv == buf_len)
				return nRecv;
		}
		else  /* timeout or error */
		{
			close(sockfd);
			return -1;
		}
		
	}
	return nRecv;
}
	
int __inline__ _Send_(int sockfd, const char * buf, unsigned int buf_len, unsigned int idle_timeout)
{	
	if(buf_len == 0)
		return 0;
	int taketime = 0;
	int res;
	fd_set mask; 
	struct timeval timeout; 
	
	unsigned int nSend = 0;
	FD_ZERO(&mask);	
	while(1)
	{
		timeout.tv_sec = idle_timeout; 
		timeout.tv_usec = 0;

		FD_SET(sockfd, &mask);
		
		res = select(sockfd + 1, NULL, &mask, NULL, &timeout);
		if( res == 1) 
		{
			taketime = 0;
			int len = send(sockfd, buf + nSend, buf_len - nSend, 0);
            if(len == 0)
			{
				close(sockfd);
				return -1;
			}
			else if(len <= 0)
			{
                if( errno == EAGAIN)
                    continue;
				close(sockfd);
				return -1;
			}
			nSend = nSend + len;
			if(nSend == buf_len)
				return 0;
		}
		else  /* timeout or error */
		{
			close(sockfd);
			return -1;
		}
	}
	return 0;
}

BOOL __inline__ connect_ssl(int sockfd,  const char* ca_crt_root, const char* ca_crt_client, const char* ca_password, const char* ca_key_client,
        SSL** pp_ssl, SSL_CTX** pp_ssl_ctx, unsigned int idle_timeout, BOOL* isHttp2)
{
    //currently only support http/1.1
    unsigned char protos[] = {
        /* 2, 'h','2', */ 
        8, 'h', 't', 't', 'p', '/', '1', '.', '1'
    };
    unsigned int protos_len = sizeof(protos);

    const unsigned char *data = NULL;
    unsigned int data_len;
                             
    *isHttp2 = FALSE;

    SSL_METHOD* meth = NULL;
#if OPENSSL_VERSION_NUMBER >= 0x010100000L
    meth = (SSL_METHOD*)TLS_client_method();
#else
    meth = (SSL_METHOD*)SSLv23_client_method();
#endif /* OPENSSL_VERSION_NUMBER */
        
    *pp_ssl_ctx = SSL_CTX_new(meth);

    if(!*pp_ssl_ctx)
    {
        
        fprintf(stderr, "SSL_CTX_new: %s\n", ERR_error_string(ERR_get_error(),NULL));
        goto FAIL_CLEAN_SSL_3;
    }

    if(ca_crt_root && ca_crt_client && ca_password && ca_key_client
        && ca_crt_root[0] != '\0' && ca_crt_client[0] != '\0' && ca_password[0] != '\0' && ca_key_client[0] != '\0')
    {
        SSL_CTX_load_verify_locations(*pp_ssl_ctx, ca_crt_root, NULL);
        if(SSL_CTX_use_certificate_file(*pp_ssl_ctx, ca_crt_client, SSL_FILETYPE_PEM) <= 0)
        {
            
            fprintf(stderr, "SSL_CTX_use_certificate_file: %s", ERR_error_string(ERR_get_error(),NULL));
            goto FAIL_CLEAN_SSL_2;
        }

        SSL_CTX_set_default_passwd_cb_userdata(*pp_ssl_ctx, (char*)ca_password);
        if(SSL_CTX_use_PrivateKey_file(*pp_ssl_ctx, ca_key_client, SSL_FILETYPE_PEM) <= 0)
        {
            
            fprintf(stderr, "SSL_CTX_use_PrivateKey_file: %s, %s", ERR_error_string(ERR_get_error(),NULL), ca_key_client);
            goto FAIL_CLEAN_SSL_2;

        }
        
        if(!SSL_CTX_check_private_key(*pp_ssl_ctx))
        {
            
            fprintf(stderr, "SSL_CTX_check_private_key: %s", ERR_error_string(ERR_get_error(),NULL));
            goto FAIL_CLEAN_SSL_2;
        }
    }
    
    *pp_ssl = SSL_new(*pp_ssl_ctx);
    if(!*pp_ssl)
    {
        
        fprintf(stderr, "SSL_new: %s\n", ERR_error_string(ERR_get_error(),NULL));
        goto FAIL_CLEAN_SSL_2;
    }
    
    if(SSL_CTX_set_alpn_protos(*pp_ssl_ctx , protos, protos_len) <= 0)
    {
        
        fprintf(stderr, "SSL_CTX_set_alpn_protos: %s", ERR_error_string(ERR_get_error(),NULL));
        goto FAIL_CLEAN_SSL_2;

    }
    
    if(SSL_set_fd(*pp_ssl, sockfd) <= 0)
    {
        
        fprintf(stderr, "SSL_set_fd: %s\n", ERR_error_string(ERR_get_error(),NULL));
        goto FAIL_CLEAN_SSL_1;
    }
    
    do
    {
        int ssl_rc = SSL_connect(*pp_ssl);
        if(ssl_rc < 0)
        {
            int ret = SSL_get_error(*pp_ssl, ssl_rc);
            if(ret == SSL_ERROR_WANT_READ || ret == SSL_ERROR_WANT_WRITE)
            {
                continue;
            }
            else
            {
                
                fprintf(stderr, "SSL_connect: %s", ERR_error_string(ERR_get_error(),NULL));
                goto FAIL_CLEAN_SSL_2;
            }

        }
        else if(ssl_rc == 0)
        {
            
            fprintf(stderr, "SSL_accept: %s", ERR_error_string(ERR_get_error(),NULL));
                
            goto FAIL_CLEAN_SSL_1;
        }
        else if(ssl_rc == 1)
        {
            break;
        }
    } while(1);
    
    SSL_get0_alpn_selected(*pp_ssl, &data, &data_len);
    if(data && memcmp(data, "http/2", data_len) == 0)
    {
        *isHttp2 = TRUE;
    }
    
    X509* client_cert;
    client_cert = SSL_get_peer_certificate(*pp_ssl);
    if (client_cert != NULL)
    {
        
        X509_free (client_cert);
    }
    else
    {
        fprintf(stderr, "SSL_get_peer_certificate: %s", ERR_error_string(ERR_get_error(),NULL));
        goto FAIL_CLEAN_SSL_1;
    }
        
    return TRUE;

FAIL_CLEAN_SSL_1:
    if(*pp_ssl)
    {
		SSL_free(*pp_ssl);
        *pp_ssl = NULL;
    }
FAIL_CLEAN_SSL_2:
    if(*pp_ssl_ctx)
    {
		SSL_CTX_free(*pp_ssl_ctx);
        *pp_ssl_ctx = NULL;
    }
FAIL_CLEAN_SSL_3:
    return FALSE;
}

void __inline__ close_ssl(SSL* p_ssl, SSL_CTX* p_ssl_ctx)
{
	if(p_ssl)
    {
		SSL_shutdown(p_ssl);
        SSL_free(p_ssl);
    }
    
    if(p_ssl_ctx)
    {
		SSL_CTX_free(p_ssl_ctx);
    }
}

int __inline__ SSLRead(int sockfd, SSL* ssl, char * buf, unsigned int buf_len, unsigned int idle_timeout)
{
	int len, ret;
	unsigned int nRecv = 0;
	while(1)
	{
		len = SSL_read(ssl, buf + nRecv, buf_len - nRecv);
		if(len == 0)
        {
            close(sockfd);
            return -1;
        }
        else if(len < 0)
        {
            ret = SSL_get_error(ssl, len);
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
		if(nRecv == buf_len)
			return nRecv;	
	}
	return nRecv;
}

int __inline__ Write(int fd, const char * buf, unsigned int buf_len)
{
	if(buf_len == 0)
		return 0;
	unsigned int nWrite = 0;
	while(1)
	{
		int len = write(fd, buf + nWrite, buf_len - nWrite);
		if(len <= 0)
			return -1;
		nWrite = nWrite + len;
		if(nWrite == buf_len)
			break;
	}
	return 0;
}

int __inline__ SSLWrite(int sockfd, SSL* ssl, const char * buf, unsigned int buf_len, unsigned int idle_timeout)
{
	if(buf_len == 0)
		return 0;
	unsigned int nSend = 0;
    int len, ret;
	while(1)
	{
		len = SSL_write(ssl, buf + nSend, buf_len - nSend);
		if(len == 0)
        {
            close(sockfd);
            return -1;
        }
        else if(len < 0)
        {
            ret = SSL_get_error(ssl, len);
            if(ret == SSL_ERROR_WANT_WRITE || ret == SSL_ERROR_WANT_READ)
            {
                continue;
            }
            else
            {
                close(sockfd);
                return -1;
            }

        }
		nSend = nSend + len;
		if(nSend == buf_len)
			break;
	}
	return 0;
}

void __inline__ Replace(string& src, const char* dst, const char* rpl)
{
	string::size_type pos = 0 - strlen(rpl);
	while( (pos = src.find(dst, pos + strlen(rpl)) ) != string::npos )
	{
		src.replace(pos, strlen(dst), rpl);
	}
	
}

void __inline__ GlobalReplace(string& src, const char* dst, const char* rpl)
{
	string::size_type pos = 0 - strlen(rpl);
	while( (pos = src.find(dst, 0) ) != string::npos )
	{
		src.replace(pos, strlen(dst), rpl);
	}
	
}

void __inline__ _strdelete_(string& src, const char* str)
{
	int slen = strlen(str);
	char* tmpstr = (char*)malloc(src.length() + 1);
	memcpy(tmpstr, src.c_str(), src.length());
	tmpstr[src.length()] = '\0';
	
	while(1)
	{
		char* p = strstr(tmpstr, str);
		
		if(p != NULL)
		{
			memmove(p, p + slen, (tmpstr + strlen(tmpstr)) - (p + slen) + 1);
		}
		else
		{
			break;
		}
	}

	src = tmpstr;
	free(tmpstr);
}

void __inline__ strtrim(string &src)
{
    if (src.empty() || src[0] == '\0') 
        return;
    std::size_t p1 = src.find_first_not_of(" \r\n\t");
    if((src[0] == ' ' || src[0] == '\r' || src[0] == '\n' || src[0] == '\t') && (p1 == string::npos))
    {
        src.erase();
        return;
    }
    else if(p1 != string::npos)
        src.erase((std::size_t)0, (std::size_t)p1);
    
    std::size_t p2 = src.find_last_not_of(" \r\n\t");
    if(p2 != string::npos)
    {
        src.erase((std::size_t)p2 + 1, string::npos);
    }
}

void __inline__ strtrim(string &src, const char* aim)
{
	string::size_type ops1;
	ops1 = src.find_first_not_of(aim);
	if((ops1 != string::npos)&&(ops1 != 0))
	{
		src.erase(0, ops1);
	}
	else if(ops1 == string::npos)
	{
		src.erase(0, src.length());
	}
	
	ops1 = src.find_last_not_of(aim);
	if((ops1 != string::npos)&&(ops1 != src.length() - 1 ))
	{
		src.erase(ops1 + 1, src.length() - ops1 - 1);
	}
}

void __inline__ strtolower(char* str)
{
	int slen = strlen(str);
	for(int x = 0; x < slen; x++)
		str[x] = tolower(str[x]);
}

void __inline__ strtoupper(char* str)
{
	int slen = strlen(str);
	for(int x = 0; x < slen; x++)
		str[x] = toupper(str[x]);
}

BOOL __inline__ strmatch(const char* aPattern, const char* aSource)
{
	char* StringPtr, *PatternPtr;
	char* StringRes, *PatternRes;
	BOOL Result = FALSE;
	BOOL isTransfed = FALSE;
	StringPtr = (char*)aSource;
	PatternPtr = (char*)aPattern;
	StringRes = NULL;
	PatternRes = NULL;
	while(1)
	{
		while(1)
		{
			if(*PatternPtr == '\0')
			{
				Result = (*StringPtr == '\0' ? TRUE : FALSE);
				if (Result || (StringRes == NULL) || (PatternRes == NULL))
				{
					return Result;
				}
				StringPtr = StringRes;
				PatternPtr = PatternRes;
				break;
			}
			else if(*PatternPtr == '*')
			{
				PatternPtr++;
				PatternRes = PatternPtr;
				break;
			}
			else if(*PatternPtr == '?')
			{
				if (*StringPtr == '0')
					return Result;
				StringPtr++;
				PatternPtr++;
			}
			else
			{
				if(*StringPtr == '\0')
					return Result;
				if (*StringPtr != *PatternPtr)
				{
					if ((StringRes == NULL) || (PatternRes == NULL))
						return Result;
					StringPtr = StringRes;
					PatternPtr = PatternRes;
					break;
				}
				else
				{
					StringPtr++;
					PatternPtr++;
				}
			}
		}
		while(1)
		{
			if( *PatternPtr == '\0')
			{
				Result = TRUE;
				return Result;
			}
			else if(*PatternPtr == '*')
			{
				PatternPtr++;
				PatternRes = PatternPtr;
			}
			else if(*PatternPtr == '?')
			{
				if (*StringPtr == '\0')
					return Result;
				StringPtr++;
				PatternPtr++;
			}
			else
			{
				while(1)
				{
					if (*StringPtr == '\0')
						return Result;
					if (*StringPtr == *PatternPtr)
						break;
					StringPtr++;
				}
				StringPtr++;
				StringRes = StringPtr;
				PatternPtr++;
				break;
			}
		}
	}
}

//match any character of separator
void __inline__ vSplitString( string strSrc ,vector<string>& vecDest ,const char* szSeparator, BOOL emptyignore = FALSE, unsigned int count = 0x7FFFFFFF )
{
    if( strSrc.empty() )
        return ;
      vecDest.clear();
      string::size_type size_pos = 0;
      string::size_type size_prev_pos = 0;
        
      while((size_pos = strSrc.find_first_of( szSeparator ,size_pos)) != string::npos)
      {
      	   if(count <= 0)
		   		break;
           string strTemp=  strSrc.substr(size_prev_pos , size_pos-size_prev_pos );
           if(strTemp == "" && emptyignore)
		   {
               //do nothing
		   }
		   else
		   {
           	vecDest.push_back(strTemp);
		   }
           size_prev_pos = ++size_pos;
		   count--;
      }

	  if(strcmp(&strSrc[size_prev_pos], "") == 0 && emptyignore)
	  {
          //do nothing
	  }
	  else
	  {
          vecDest.push_back(&strSrc[size_prev_pos]);
	  }
};

//match whole separator
void __inline__ vSplitStringEx( string strSrc ,vector<string>& vecDest ,const char* szSeparator, BOOL emptyignore = FALSE, unsigned int count = 0x7FFFFFFF )
{
    if( strSrc.empty() )
        return ;
      vecDest.clear();
      string::size_type size_pos = 0;
      string::size_type size_prev_pos = 0;
      
      while( (size_pos = strSrc.find( szSeparator ,size_pos, strlen(szSeparator))) != string::npos)
      {
      	   if(count <=0)
		   		break;
           string strTemp=  strSrc.substr(size_prev_pos , size_pos - size_prev_pos );
           if((strTemp == "")&&(emptyignore))
		   {

		   }
		   else
		   {
           	vecDest.push_back(strTemp);
           	//cout << "string = ["<< strTemp <<"]"<< endl;
		   }
		   size_pos += strlen(szSeparator);
           size_prev_pos = size_pos;
		   count--;
      }

	  if((strcmp(&strSrc[size_prev_pos], "") == 0)&&(emptyignore))
	  {

	  }
	  else
	  {
      	vecDest.push_back(&strSrc[size_prev_pos]);
      	//cout << "end string = ["<< &strSrc[size_prev_pos] <<"]"<< endl;
	  }
};

void __inline__ Toupper( string & str)
{
    for(int i=0;i<str.size();i++)
        str.at(i)=toupper(str.at(i));
}

void __inline__ OutTimeString(unsigned int nTime, string & strTime)
{
	char buf[128];
	time_t clock = nTime;
	struct tm *tm;
	tm = localtime(&clock);

	strftime(buf, sizeof(buf), "%a, %d %h %Y %H:%M:%S %z (%Z)", tm);

	strTime = buf;

}

void __inline__ OutHTTPGMTDateString(unsigned int nTime, string & strTime)
{
	char buf[128];
	time_t clock = nTime;
	struct tm *tm;
	tm = gmtime(&clock);

	strftime(buf, sizeof(buf), "%a, %d %h %Y %H:%M:%S GMT", tm);

	strTime = buf;

}

void __inline__ OutHTTPUTCDateString(unsigned int nTime, string & strTime)
{
	char buf[128];
	time_t clock = nTime;
	struct tm *tm;
	tm = gmtime(&clock);

	strftime(buf, sizeof(buf), "%a, %d %h %Y %H:%M:%S UTC", tm);

	strTime = buf;

}

void __inline__ OutSimpleTimeString(unsigned int nTime, string & strTime)
{
	char buf[128];
	time_t clock = nTime;
	struct tm *tm;
	tm = localtime(&clock);

	strftime(buf, sizeof(buf), "%y-%m-%d %H:%M", tm);

	strTime = buf;
	
}


int __inline__ getmonthnumber(const char* mstr)
{
	//"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"

	if(strcasecmp(mstr, "Jan") == 0)
	{
		return 1;
	}
	else if(strcasecmp(mstr, "Feb") == 0)
	{
		return 2;
	}
	else if(strcasecmp(mstr, "Mar") == 0)
	{
		return 3;
	}
	else if(strcasecmp(mstr, "Apr") == 0)
	{
		return 4;
	}
	else if(strcasecmp(mstr, "May") == 0)
	{
		return 5;
	}
	else if(strcasecmp(mstr, "Jun") == 0)
	{
		return 6;
	}
	else if(strcasecmp(mstr, "Jul") == 0)
	{
		return 7;
	}
	else if(strcasecmp(mstr, "Aug") == 0)
	{
		return 8;
	}
	else if(strcasecmp(mstr, "Sep") == 0)
	{
		return 9;
	}
	else if(strcasecmp(mstr, "Oct") == 0)
	{
		return 10;
	}
	else if(strcasecmp(mstr, "Nov") == 0)
	{
		return 11;
	}
	else if(strcasecmp(mstr, "Dec") == 0)
	{
		return 12;
	}
	else
	{
		return atoi(mstr);
	}
}

int __inline__ datecmp(unsigned int y1, unsigned int m1, unsigned int d1,unsigned int y2, unsigned int m2, unsigned int d2)
{
	if(y1 > y2)
		return 1;
	else if (y1 == y2)
	{
		if(m1 > m2)
			return 1;
		else if(m1 == m2)
		{
			if(d1 > d2)
				return 1;
			else if(d1 == d2)
			{
				return 0;
			}
			else
				return -1;
		}
		else
			return -1;
	}
	else
		return -1;
}

unsigned int __inline__ getcharnum(const char* src, char ch)
{
	unsigned int num = 0;
	for(int x = 0; x < strlen(src); x++)
	{
		if(src[x] == ch)
		{
			num++;
		}
	}
	return num;
}

void __inline__ lowercase(const char* szSrc, string &strDst)
{
	strDst.clear();
	int len = strlen(szSrc);
	for(int x = 0; x < len; x++)
	{
		strDst.push_back(LOCH(szSrc[x]));
	}
	strDst.push_back('\0');
}

void __inline__ get_extend_name(const char* filename, string & extname)
{
	vector<string> vTmp;
	vSplitString(filename, vTmp, ".");
	if(vTmp.size() > 1)
	{
		extname = vTmp[vTmp.size() - 1];
	}
	else
	{
		extname = "";
	}
}

int __inline__ code_convert(const char *from_charset, const char *to_charset, const char *inbuf, size_t inlen, char *outbuf, size_t outlen)   
{
	iconv_t cd;   
	int rc;   
	char **pin = (char**)&inbuf;   
	char **pout = &outbuf;
	cd = iconv_open(to_charset, from_charset);
	if(cd == (iconv_t)-1)
		return -1;
	memset(outbuf, 0, outlen);

#ifdef _SOLARIS_OS_
	if(iconv(cd, (const char**)pin, &inlen, pout, &outlen) == -1)
	
#else
	if(iconv(cd, pin, &inlen, pout, &outlen) == -1)
#endif /* SOLARIS */
	{
		return -1;
	}
	iconv_close(cd);
	
	return 0;   
}

int __inline__ code_convert_ex(const char *from_charset, const char *to_charset, const char *inbuf, string& strout)   
{
	if(strcasecmp(from_charset, to_charset) == 0)
	{
		strout = inbuf;
		return 0;
	}

	size_t outlen = strlen(inbuf)*4 + 1;
	char *outbuf = new char[outlen];
	if(!outbuf)
		return -1;
	int ret = code_convert(from_charset, to_charset, inbuf, strlen(inbuf), outbuf, outlen);
	if( ret == 0 )
	{
		strout = outbuf;
	}
	delete[] outbuf;
	return ret;
}

int __inline__ utf8_to_gb2312_ex(const char *inbuf, string& strout)   
{
	size_t outlen = strlen(inbuf)*4 + 1;
	char *outbuf = new char[outlen];
	int ret = code_convert("UTF-8", "GB2312", inbuf, strlen(inbuf), outbuf, outlen);
	if( ret == 0 )
	{
		strout = outbuf;
	}
	delete[] outbuf;
	return ret;
}

int __inline__ gb2312_to_utf8_ex(const char *inbuf, string& strout)   
{
	size_t outlen = strlen(inbuf)*4 + 1;
	char *outbuf = new char[outlen];
	int ret = code_convert("GB2312", "UTF-8", inbuf, strlen(inbuf), outbuf, outlen);
	if( ret == 0 )
	{
		strout = outbuf;
	}
	delete[] outbuf;
	return ret;
}


int __inline__ gb2312_to_ucs2_ex(const char *inbuf, string& strout)   
{
	size_t outlen = strlen(inbuf)*4 + 1;
	char *outbuf = new char[outlen];
	int ret = code_convert("GB2312", "UCS2", inbuf, strlen(inbuf), outbuf, outlen);
	if( ret == 0 )
	{
		strout = outbuf;
	}
	delete[] outbuf;
	return ret;
}

int __inline__ UCS2_to_gb2312_ex(const char *inbuf, string& strout)   
{
	size_t outlen = strlen(inbuf)*4 + 1;
	char *outbuf = new char[outlen];
	int ret = code_convert("UCS2", "GB2312", inbuf, strlen(inbuf), outbuf, outlen);
	if( ret == 0 )
	{
		strout = outbuf;
	}
	delete[] outbuf;
	return ret;
}

int __inline__ utf8_to_ucs2_ex(const char *inbuf, string& strout)   
{
	size_t outlen = strlen(inbuf)*4 + 1;
	char *outbuf = new char[outlen];
	int ret = code_convert("GB2312", "UCS2", inbuf, strlen(inbuf), outbuf, outlen);
	if( ret == 0 )
	{
		strout = outbuf;
	}
	delete[] outbuf;
	return ret;
}

int __inline__ UCS2_to_utf8_ex(const char *inbuf, string& strout)   
{
	size_t outlen = strlen(inbuf)*4 + 1;
	char *outbuf = new char[outlen];
	int ret = code_convert("UCS2", "GB2312", inbuf, strlen(inbuf), outbuf, outlen);
	if( ret == 0 )
	{
		strout = outbuf;
	}
	delete[] outbuf;
	return ret;
}

int __inline__ utf8_to_utf7_ex(const char *inbuf, string& strout)   
{
	size_t outlen = strlen(inbuf)*4 + 1;
	char *outbuf = new char[outlen];
	
	int ret = code_convert("UTF-8", "UTF-7", inbuf, strlen(inbuf), outbuf, outlen);
	if( ret == 0 )
	{
		strout = outbuf;	
	}
	delete[] outbuf;
	return ret;
}

int __inline__ utf7_to_utf8_ex(const char *inbuf, string& strout)   
{
	size_t outlen = strlen(inbuf)*4 + 1;
	char *outbuf = new char[outlen];
	int ret = code_convert("UTF-7", "UTF-8", inbuf, strlen(inbuf), outbuf, outlen);
	if( ret == 0 )
	{
		strout = outbuf;
	}
	delete[] outbuf;
	return ret;
}

int __inline__ utf7_to_gb2312_ex(const char *inbuf, string& strout)   
{
	size_t outlen = strlen(inbuf)*4 + 1;
	char *outbuf = new char[outlen];
	int ret = code_convert("UTF-7", "GB2312", inbuf, strlen(inbuf), outbuf, outlen);
	if( ret == 0 )
	{
		strout = outbuf;	
	}
	delete[] outbuf;
	return ret;
}

int __inline__ gb2312_to_utf7_ex(const char *inbuf, string& strout)   
{
	size_t outlen = strlen(inbuf)*4 + 1;
	char *outbuf = new char[outlen];
	int ret = code_convert("GB2312", "UTF-7", inbuf, strlen(inbuf), outbuf, outlen);
	if( ret == 0 )
	{
		strout = outbuf;
	}
	delete[] outbuf;
	return ret;
}

//convent the modified UTF-7 to standard UTF-7
int __inline__ utf7_modified_to_standard(const char* inbuf, int inlen, char* outbuf, int outlen)
{
	int x = 0;
	int y = 0;

	BOOL isUTF7 = FALSE;
	while(1)
	{
		if(inbuf[x] == '\0')
		{
			break;
		}
		if(y >= outlen)
		{
			return -1;
		}
		else if(inbuf[x] == '&')
		{
			if(inbuf[x + 1] != '\0')
			{
				if(inbuf[x + 1] == '-')
				{
					outbuf[y] = '&';
					x += 2;
					y++;
				}
				else
				{
					outbuf[y] = '+';
					x++;
					y++;

					isUTF7 = TRUE;
				}
			}
			else /* the last character */
			{
				outbuf[y] = '&';
				x++;
				y++;
			}
		}
		else if(inbuf[x] == ',')
		{
			if(isUTF7)
			{
				outbuf[y] = '/';
				x++;
				y++;
			}
			else
			{
				outbuf[y] = ',';
				x++;
				y++;
			}
		}
		else if(inbuf[x] == '+')
		{
			outbuf[y] = '+';
			outbuf[y + 1] = '-';
			x++;
			y += 2;
		}
		else if(inbuf[x] == '-')
		{
			outbuf[y] = '-';
			x++;
			y++;
			
			if(isUTF7)
			{
				isUTF7 = FALSE;
			}
		}
		else
		{
			outbuf[y] = inbuf[x];
			x++;
			y++;
		}
	}

	outbuf[y] = '\0';

	return 0;
}

int __inline__ utf7_modified_to_standard_ex(const char* inbuf, string& outlen)
{
	int tmplen = strlen(inbuf) * 4 + 1;
	char* tmpbuf = new char[tmplen + 1];
	int ret = utf7_modified_to_standard(inbuf, strlen(inbuf), tmpbuf, tmplen);

	if(ret == 0)
		outlen = tmpbuf;

	delete [] tmpbuf;
	return ret;
}

//convent the standard UTF-7 to modified UTF-7
int __inline__ utf7_standard_to_modified(const char* inbuf, int inlen, char* outbuf, int outlen)
{
	int x = 0;
	int y = 0;

	BOOL isUTF7 = FALSE;
	while(1)
	{
		if(inbuf[x] == '\0')
		{
			break;
		}
		if(y >= outlen)
		{
			return -1;
		}
		else if(inbuf[x] == '+')
		{
			if(inbuf[x + 1] != '\0')
			{
				if(inbuf[x + 1] == '-')
				{
					outbuf[y] = '+';
					x += 2;
					y++;
				}
				else
				{
					outbuf[y] = '&';
					x++;
					y++;

					isUTF7 = TRUE;
				}
			}
			else /* the last character */
			{
				outbuf[y] = '+';
				x++;
				y++;
			}
		}
		else if(inbuf[x] == '/')
		{
			if(isUTF7)
			{
				outbuf[y] = ',';
				x++;
				y++;
			}
			else
			{
				outbuf[y] = '/';
				x++;
				y++;
			}
		}
		else if(inbuf[x] == '&')
		{
			outbuf[y] = '&';
			outbuf[y + 1] = '-';
			x++;
			y += 2;
		}
		else if(inbuf[x] == '-')
		{
			outbuf[y] = '-';
			x++;
			y++;
			
			if(isUTF7)
			{
				isUTF7 = FALSE;
			}
		}
		else
		{
			outbuf[y] = inbuf[x];
			x++;
			y++;
		}
	}

	outbuf[y] = '\0';

	return 0;
}

int __inline__ utf7_standard_to_modified_ex(const char* inbuf, string& outlen)
{
	int tmplen = strlen(inbuf) * 4 + 1;
	char* tmpbuf = new char[tmplen + 1];
	int ret = utf7_standard_to_modified(inbuf, strlen(inbuf), tmpbuf, tmplen);

	if(ret == 0)
		outlen = tmpbuf;

	delete [] tmpbuf;
	return ret;
}


unsigned long long __inline__ atollu(const char* str)
{
	unsigned long long num;
	sscanf(str, "%llu", &num);
	return num;
}

/* Wed, 09 Jun 2021 10:18:14 GMT */
time_t __inline__ ParseGMTorUTCTimeString(const char* szDateTime)
{
    if(strlen(szDateTime) < 29)
        return 0;
    struct tm tm1;
    unsigned year , day, hour, min, sec, zone;
	char sz_mon[32];
	char sz_zone[32];
	sscanf(szDateTime, "%*[^,], %d %[^ ] %d %d:%d:%d %[^ ]", &day, sz_mon, &year, &hour, &min, &sec, sz_zone);
	
	if(strcmp(sz_zone, "GMT") != 0 && strcmp(sz_zone, "UTC") != 0)
	    return 0;
	
	tm1.tm_year = year - 1900;
	tm1.tm_mon = getmonthnumber(sz_mon) - 1;
	tm1.tm_mday = day;
	tm1.tm_hour = hour;
	tm1.tm_min = min;
	tm1.tm_sec = sec;
	tm1.tm_isdst = -1;
	
	return timegm(&tm1);
}
#endif /* _HEAPHTTPD_GENERAL_H_ */


