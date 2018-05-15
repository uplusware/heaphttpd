/*
	Copyright (c) openheap, uplusware
	uplusware@gmail.com
*/

#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <errno.h>
#include <semaphore.h>
#include <mqueue.h>
#include <pthread.h>
#include <queue>
#include <sys/syscall.h>
#define gettid() syscall(__NR_gettid)
#define gettid2() syscall(SYS_gettid)
#include "service.h"
#include "session.h"
#include "cache.h"
#include "pool.h"
#include "util/trace.h"

#define HEAPHTTPD_DYNAMIC_WORKDERS 0

int SEND_FD(int sfd, int fd_file, CLIENT_PARAM* param, int wait_timeout = 0) 
{
    int res;
	fd_set mask; 
	struct timeval timeout; 
    FD_ZERO(&mask);
    FD_SET(sfd, &mask);
    
    timeout.tv_sec = wait_timeout; 
    timeout.tv_usec = 0;
    
    res = select(sfd + 1, NULL, &mask, NULL, wait_timeout > 0 ? &timeout : NULL);// 0 means recieve block mode
	
    if(res == 1) 
	{ 
        struct msghdr msg;  
        struct iovec iov[1];  
        union{  
            struct cmsghdr cm;  
            char control[CMSG_SPACE(sizeof(int))];  
        }control_un;  
        struct cmsghdr *cmptr;     
        msg.msg_control = control_un.control;   
        msg.msg_controllen = sizeof(control_un.control);  
        cmptr = CMSG_FIRSTHDR(&msg);  
        cmptr->cmsg_len = CMSG_LEN(sizeof(int));
        cmptr->cmsg_level = SOL_SOCKET;   
        cmptr->cmsg_type = SCM_RIGHTS;
        *((int*)CMSG_DATA(cmptr)) = fd_file;
        msg.msg_name = NULL;  
        msg.msg_namelen = 0;  
        iov[0].iov_base = param;  
        iov[0].iov_len = sizeof(CLIENT_PARAM);  
        msg.msg_iov = iov;  
        msg.msg_iovlen = 1;

        return sendmsg(sfd, &msg, 0); 
    }
    else if(res == 0)
    {
        return -2; //timeout
    }
    else
    {
        return -1;
    }

}

int RECV_FD(int sfd, int* fd_file, CLIENT_PARAM* param, int wait_timeout = 0) 
{
    int res;
	fd_set mask; 
	struct timeval timeout; 
    FD_ZERO(&mask);
    FD_SET(sfd, &mask);
    
    timeout.tv_sec = wait_timeout; 
    timeout.tv_usec = 0;
    
    res = select(sfd + 1, &mask, NULL, NULL, wait_timeout > 0 ? &timeout : NULL);// 0 means recieve block mode
	
    if(res == 1) 
	{    
        struct msghdr msg;  
        struct iovec iov[1];  
        int nrecv;  
        union{
            struct cmsghdr cm;  
            char control[CMSG_SPACE(sizeof(int))];  
        }control_un;  
        struct cmsghdr *cmptr;  
        msg.msg_control = control_un.control;  
        msg.msg_controllen = sizeof(control_un.control);
        msg.msg_name = NULL;  
        msg.msg_namelen = 0;  

        iov[0].iov_base = param;  
        iov[0].iov_len = sizeof(CLIENT_PARAM);  
        msg.msg_iov = iov;  
        msg.msg_iovlen = 1;
        
        if((nrecv = recvmsg(sfd, &msg, 0)) < 0)  
        {  
            return nrecv;  
        }

        cmptr = CMSG_FIRSTHDR(&msg);  
        if((cmptr != NULL) && (cmptr->cmsg_len == CMSG_LEN(sizeof(int))))  
        {  
            if(cmptr->cmsg_level != SOL_SOCKET)  
            {  
                fprintf(stderr, "control level != SOL_SOCKET/n");  
                return -1;
            }  
            if(cmptr->cmsg_type != SCM_RIGHTS)  
            {  
                fprintf(stderr, "control type != SCM_RIGHTS/n");  
                return -1; 
            }
            *fd_file = *((int*)CMSG_DATA(cmptr));  
        }  
        else  
        {  
            if(cmptr == NULL)
            {
                perror("null cmptr, fd not passed");
            }
            else
            {
                fprintf(stderr, "message len[%d] if incorrect: %s\n", cmptr->cmsg_len, strerror(errno));  
            }
            *fd_file = -1; // descriptor was not passed  
        }   
        return *fd_file;
    }
    else if(res == 0)
    {
        return -2; //timeout
    }
    else
    {
        return -1;
    }
}

/*
OpenSSL example:
	unsigned char vector[] = {
	 6, 's', 'p', 'd', 'y', '/', '1',
	 8, 'h', 't', 't', 'p', '/', '1', '.', '1'
	};
	unsigned int length = sizeof(vector);
*/

typedef struct{
    char* ptr;
    int len;
}tls_alpn;

static int alpn_cb(SSL *ssl,
				const unsigned char **out,
				unsigned char *outlen,
				const unsigned char *in,
				unsigned int inlen,
				void *arg)
{
    int ret = SSL_TLSEXT_ERR_NOACK;
    *out = NULL;
	*outlen = 0;
    
	unsigned char* p = (unsigned char*)in;
	while(inlen > 0 && in && (p - in) < inlen)
	{
		int len = p[0];
		p++;
        
		/*
        for(int x = 0; x < len; x++)
		{
			printf("%c", p[x]);
		}
		printf("\n");
        */
        
		if(len == 2 && memcmp(p, "h2", 2) == 0)
		{
            BOOL* pIsHttp2 = (BOOL*)arg;
            *pIsHttp2 = TRUE;
            *out = p;
            *outlen = len;
            ret = SSL_TLSEXT_ERR_OK;
            break;
		}
        else if(len == 8 && memcmp(p, "http/1.1", 8) == 0)
        {
            BOOL* pIsHttp2 = (BOOL*)arg;
            *pIsHttp2 = FALSE;
            *out = p;
            *outlen = len;
            ret = SSL_TLSEXT_ERR_OK;
            break;
        }
		p = p + len;
	}
    
	return ret;
}

std::queue<SESSION_PARAM*> Worker::m_STATIC_THREAD_POOL_ARG_QUEUE;
volatile char Worker::m_STATIC_THREAD_POOL_EXIT = TRUE;
pthread_mutex_t Worker::m_STATIC_THREAD_POOL_MUTEX;
sem_t Worker::m_STATIC_THREAD_POOL_SEM;
volatile unsigned int Worker::m_STATIC_THREAD_POOL_SIZE = 0;
pthread_mutex_t Worker::m_STATIC_THREAD_POOL_SIZE_MUTEX;

pthread_rwlock_t Worker::m_STATIC_THREAD_IDLE_NUM_LOCK;
volatile unsigned int Worker::m_STATIC_THREAD_IDLE_NUM = 0;
    
void Worker::SESSION_HANDLING(SESSION_PARAM* session_param)
{   
    Worker * p_worker = session_param->worker;
	BOOL isHttp2 = FALSE;
	Session* pSession = NULL;
    SSL* ssl = NULL;
    int ssl_rc = -1;
    BOOL bSSLAccepted;
	SSL_CTX* ssl_ctx = NULL;
	X509* client_cert = NULL;
    
    int flags = fcntl(session_param->sockfd, F_GETFL, 0); 
	fcntl(session_param->sockfd, F_SETFL, flags | O_NONBLOCK);
        
    if(session_param->https == TRUE)
	{
		SSL_METHOD* meth;
#if OPENSSL_VERSION_NUMBER >= 0x010100000L
        meth = (SSL_METHOD*)TLS_server_method();
#else
        meth = (SSL_METHOD*)SSLv23_server_method();
#endif /* OPENSSL_VERSION_NUMBER */
		ssl_ctx = SSL_CTX_new(meth);
		if(!ssl_ctx)
		{
			fprintf(stderr, "SSL_CTX_use_certificate_file: %s", ERR_error_string(ERR_get_error(),NULL));
			goto FAIL_CLEAN_SSL_3;
		}
		
		if(session_param->client_cer_check)
		{
    		SSL_CTX_set_verify(ssl_ctx, SSL_VERIFY_PEER|SSL_VERIFY_FAIL_IF_NO_PEER_CERT, NULL);
    		SSL_CTX_set_verify_depth(ssl_ctx, 4);
		}
		SSL_CTX_load_verify_locations(ssl_ctx, session_param->ca_crt_root.c_str(), NULL);
		if(SSL_CTX_use_certificate_file(ssl_ctx, session_param->ca_crt_server.c_str(), SSL_FILETYPE_PEM) <= 0)
		{
			fprintf(stderr, "SSL_CTX_use_certificate_file: %s", ERR_error_string(ERR_get_error(),NULL));
			goto FAIL_CLEAN_SSL_3;
		}
        
		SSL_CTX_set_default_passwd_cb_userdata(ssl_ctx, (char*)session_param->ca_password.c_str());
		if(SSL_CTX_use_PrivateKey_file(ssl_ctx, session_param->ca_key_server.c_str(), SSL_FILETYPE_PEM) <= 0)
		{
			fprintf(stderr, "SSL_CTX_use_PrivateKey_file: %s", ERR_error_string(ERR_get_error(),NULL));
			goto FAIL_CLEAN_SSL_3;

		}
		if(!SSL_CTX_check_private_key(ssl_ctx))
		{
			fprintf(stderr, "SSL_CTX_check_private_key: %s", ERR_error_string(ERR_get_error(),NULL));
			goto FAIL_CLEAN_SSL_3;
		}
		if(session_param->http2)
            ssl_rc = SSL_CTX_set_cipher_list(ssl_ctx, CHttpBase::m_http2_tls_cipher.c_str());
        else
            ssl_rc = SSL_CTX_set_cipher_list(ssl_ctx, CHttpBase::m_https_cipher.c_str());
        if(ssl_rc == 0)
        {
            fprintf(stderr, "SSL_CTX_set_cipher_list: %s", ERR_error_string(ERR_get_error(),NULL));
            goto FAIL_CLEAN_SSL_3;
        }
		SSL_CTX_set_mode(ssl_ctx, SSL_MODE_AUTO_RETRY);
#if OPENSSL_VERSION_NUMBER >= 0x010002000L
		if(session_param->http2)
			SSL_CTX_set_alpn_select_cb(ssl_ctx, alpn_cb, &isHttp2);
#endif /* OPENSSL_VERSION_NUMBER */

		ssl = SSL_new(ssl_ctx);
		if(!ssl)
		{
			fprintf(stderr, "SSL_new: %s", ERR_error_string(ERR_get_error(),NULL));
			goto FAIL_CLEAN_SSL_2;
		}
		ssl_rc = SSL_set_fd(ssl, session_param->sockfd);
        if(ssl_rc == 0)
        {
            fprintf(stderr, "SSL_set_fd: %s", ERR_error_string(ERR_get_error(),NULL));
            goto FAIL_CLEAN_SSL_2;
        }
        if(session_param->http2)
            ssl_rc = SSL_set_cipher_list(ssl, CHttpBase::m_http2_tls_cipher.c_str());
        else
            ssl_rc = SSL_set_cipher_list(ssl, CHttpBase::m_https_cipher.c_str());
        if(ssl_rc == 0)
        {
            fprintf(stderr, "SSL_set_cipher_list: %s", ERR_error_string(ERR_get_error(),NULL));
            goto FAIL_CLEAN_SSL_2;
        }

        do {
            ssl_rc = SSL_accept(ssl);
            if(ssl_rc < 0)
            {
                int ret = SSL_get_error(ssl, ssl_rc);
                    
                if(ret == SSL_ERROR_WANT_READ || ret == SSL_ERROR_WANT_WRITE)
                {
                    fd_set mask;
                    struct timeval timeout;
            
                    timeout.tv_sec = CHttpBase::m_connection_idle_timeout;
                    timeout.tv_usec = 0;

                    FD_ZERO(&mask);
                    FD_SET(session_param->sockfd, &mask);
                    
                    int res = select(session_param->sockfd + 1, ret == SSL_ERROR_WANT_READ ? &mask : NULL, ret == SSL_ERROR_WANT_WRITE ? &mask : NULL, NULL, &timeout);

                    if( res == 1)
                    {
                        continue;
                    }
                    else /* timeout or error */
                    {
                        
                        fprintf(stderr, "SSL_accept: %s %s", ERR_error_string(ERR_get_error(),NULL), strerror(errno));
                        goto FAIL_CLEAN_SSL_2;
                    }
                }
                else
                {
                    
                    fprintf(stderr, "SSL_accept: %s", ERR_error_string(ERR_get_error(),NULL));
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
        
        bSSLAccepted = TRUE;
        if(session_param->client_cer_check)
        {
            ssl_rc = SSL_get_verify_result(ssl);
            if(ssl_rc != X509_V_OK)
            {
                
                fprintf(stderr, "SSL_get_verify_result: %s", ERR_error_string(ERR_get_error(),NULL));
                goto FAIL_CLEAN_SSL_1;
            }
        }
		if(session_param->client_cer_check)
		{
			client_cert = SSL_get_peer_certificate(ssl);
			if (client_cert == NULL)
			{
				
                fprintf(stderr, "SSL_get_peer_certificate: %s", ERR_error_string(ERR_get_error(),NULL));
				goto FAIL_CLEAN_SSL_1;
			}
		}
	}
#ifdef _WITH_ASYNC_
    pSession = new Session(p_worker->GetSessionGroup()->Get_epoll_fd(), session_param->srvobjmap, session_param->sockfd, ssl,
        session_param->client_ip.c_str(), client_cert, session_param->https, isHttp2, session_param->cache);
	if(pSession != NULL)
	{
		p_worker->GetSessionGroup()->Append(session_param->sockfd, pSession);
	}

#else
	pSession = new Session(-1, session_param->srvobjmap, session_param->sockfd, ssl,
        session_param->client_ip.c_str(), client_cert, session_param->https, isHttp2, session_param->cache);
	if(pSession != NULL)
	{
		pSession->Processing();
		delete pSession;
        pSession = NULL;
	}
#endif /* _WITH_ASYNC_ */
FAIL_CLEAN_SSL_1:
    if(client_cert)
        X509_free (client_cert);
    client_cert = NULL;
	if(ssl && bSSLAccepted)
    {
		SSL_shutdown(ssl);
        bSSLAccepted = FALSE;
    }
FAIL_CLEAN_SSL_2:
	if(ssl)
    {
		SSL_free(ssl);
        ssl = NULL;
    }
FAIL_CLEAN_SSL_3:
	if(ssl_ctx)
    {
		SSL_CTX_free(ssl_ctx);
        ssl_ctx = NULL;
    }
	close(session_param->sockfd);
}

void Worker::INIT_THREAD_POOL_HANDLER()
{
	m_STATIC_THREAD_POOL_EXIT = TRUE;
	m_STATIC_THREAD_POOL_SIZE = 0;
    m_STATIC_THREAD_IDLE_NUM = 0;
	while(!m_STATIC_THREAD_POOL_ARG_QUEUE.empty())
	{
		m_STATIC_THREAD_POOL_ARG_QUEUE.pop();
	}
	pthread_mutex_init(&m_STATIC_THREAD_POOL_MUTEX, NULL);
    pthread_mutex_init(&m_STATIC_THREAD_POOL_SIZE_MUTEX, NULL);
    pthread_rwlock_init(&m_STATIC_THREAD_IDLE_NUM_LOCK, NULL);
    
	sem_init(&m_STATIC_THREAD_POOL_SEM, 0, 0);
}

void* Worker::START_THREAD_POOL_HANDLER(void* arg)
{
    Worker* p_worker = (Worker*)arg;
	m_STATIC_THREAD_POOL_EXIT = TRUE;

#ifdef HEAPHTTPD_DYNAMIC_WORKDERS	
    pthread_mutex_lock(&m_STATIC_THREAD_POOL_SIZE_MUTEX);
	m_STATIC_THREAD_POOL_SIZE++;
    pthread_mutex_unlock(&m_STATIC_THREAD_POOL_SIZE_MUTEX);
    
    pthread_rwlock_wrlock(&m_STATIC_THREAD_IDLE_NUM_LOCK);
    m_STATIC_THREAD_IDLE_NUM++;
    pthread_rwlock_unlock(&m_STATIC_THREAD_IDLE_NUM_LOCK);
                
#endif /* HEAPHTTPD_DYNAMIC_WORKDERS */
	struct timespec ts;
	while(m_STATIC_THREAD_POOL_EXIT)
	{
		clock_gettime(CLOCK_REALTIME, &ts);
#ifndef _WITH_ASYNC_
		ts.tv_sec += CHttpBase::m_service_idle_timeout;
#endif /* _WITH_ASYNC_ */
		if(sem_timedwait(&m_STATIC_THREAD_POOL_SEM, &ts) == 0)
		{
			SESSION_PARAM* session_param = NULL;
		
			pthread_mutex_lock(&m_STATIC_THREAD_POOL_MUTEX);
			if(!m_STATIC_THREAD_POOL_ARG_QUEUE.empty())
			{
				session_param = m_STATIC_THREAD_POOL_ARG_QUEUE.front();
				m_STATIC_THREAD_POOL_ARG_QUEUE.pop();			
			}
			pthread_mutex_unlock(&m_STATIC_THREAD_POOL_MUTEX);
			if(session_param)
			{
#ifdef HEAPHTTPD_DYNAMIC_WORKDERS
                pthread_rwlock_wrlock(&m_STATIC_THREAD_IDLE_NUM_LOCK);
                m_STATIC_THREAD_IDLE_NUM--;
                pthread_rwlock_unlock(&m_STATIC_THREAD_IDLE_NUM_LOCK);
#endif /* HEAPHTTPD_DYNAMIC_WORKDERS */
				SESSION_HANDLING(session_param);
				delete session_param;

#ifdef HEAPHTTPD_DYNAMIC_WORKDERS
                pthread_rwlock_wrlock(&m_STATIC_THREAD_IDLE_NUM_LOCK);
                m_STATIC_THREAD_IDLE_NUM++;
                pthread_rwlock_unlock(&m_STATIC_THREAD_IDLE_NUM_LOCK);
#endif /* HEAPHTTPD_DYNAMIC_WORKDERS */                
			}
		}
#ifdef HEAPHTTPD_DYNAMIC_WORKDERS
		else
		{
			
            if(errno == EINTR || errno == ETIMEDOUT)
            {
                p_worker->GetSessionGroup()->Processing();
            }
            else
            {
                break; //exit from the current thread
            }
		}
#endif /* HEAPHTTPD_DYNAMIC_WORKDERS */
	}

#ifdef HEAPHTTPD_DYNAMIC_WORKDERS
    pthread_mutex_lock(&m_STATIC_THREAD_POOL_SIZE_MUTEX);
	m_STATIC_THREAD_POOL_SIZE--;
    pthread_mutex_unlock(&m_STATIC_THREAD_POOL_SIZE_MUTEX);
    
    pthread_rwlock_wrlock(&m_STATIC_THREAD_IDLE_NUM_LOCK);
    m_STATIC_THREAD_IDLE_NUM--;
    pthread_rwlock_unlock(&m_STATIC_THREAD_IDLE_NUM_LOCK);
    
#endif /* HEAPHTTPD_DYNAMIC_WORKDERS */

	if(arg != NULL)
		delete arg;
	
	pthread_exit(0);
}

void Worker::LEAVE_THREAD_POOL_HANDLER()
{
	printf("LEAVE_THREAD_POOL_HANDLER 1\n");
    
	m_STATIC_THREAD_POOL_EXIT = FALSE;

#ifdef HEAPHTTPD_DYNAMIC_WORKDERS
    BOOL STILL_HAVE_THREADS = TRUE;
    
	while(STILL_HAVE_THREADS)
	{
        pthread_mutex_lock(&m_STATIC_THREAD_POOL_SIZE_MUTEX);
        if(m_STATIC_THREAD_POOL_SIZE <= 0)
        {
            STILL_HAVE_THREADS = FALSE;
        }
        pthread_mutex_unlock(&m_STATIC_THREAD_POOL_SIZE_MUTEX);
        
        if(STILL_HAVE_THREADS)
            usleep(1000*10);
	}
#endif /* HEAPHTTPD_DYNAMIC_WORKDERS */
	
    sem_close(&m_STATIC_THREAD_POOL_SEM);

    char local_sockfile[256];
    sprintf(local_sockfile, "/tmp/heaphttpd/fastcgi.sock.%05d.%05d", getpid(), gettid());

    unlink(local_sockfile);
    
    pthread_mutex_destroy(&m_STATIC_THREAD_POOL_MUTEX);
    pthread_mutex_destroy(&m_STATIC_THREAD_POOL_SIZE_MUTEX);    
    pthread_rwlock_destroy(&m_STATIC_THREAD_IDLE_NUM_LOCK);

#ifndef HEAPHTTPD_DYNAMIC_WORKDERS
	pthread_exit(0);
#endif /* HEAPHTTPD_DYNAMIC_WORKDERS */

    printf("LEAVE_THREAD_POOL_HANDLER 2\n");
}

void CLEAR_QUEUE(mqd_t qid)
{
	mq_attr attr;
	struct timespec ts;
	mq_getattr(qid, &attr);
	char* buf = (char*)malloc(attr.mq_msgsize);
	while(1)
	{
		clock_gettime(CLOCK_REALTIME, &ts);
		if(mq_timedreceive(qid, (char*)buf, attr.mq_msgsize, NULL, &ts) == -1)
		{
			break;
		}
	}
	free(buf);
}

//////////////////////////////////////////////////////////////////////////////////
//Worker
Worker::Worker(const char* service_name, int process_seq, int thread_num, int sockfd)
{

	m_sockfd = sockfd;
	m_thread_num = thread_num;
	m_process_seq = process_seq;
	m_service_name = service_name;
#ifdef _WITH_MEMCACHED_    
	m_cache = new memory_cache(m_service_name.c_str(), m_process_seq, CHttpBase::m_work_path.c_str(), CHttpBase::m_memcached_list);
#else
    m_cache = new memory_cache(m_service_name.c_str(), m_process_seq, CHttpBase::m_work_path.c_str());
#endif /* _WITH_MEMCACHED_ */    
	m_cache->load();
    
    int flags = fcntl(m_sockfd, F_GETFL, 0); 
	fcntl(m_sockfd, F_SETFL, flags | O_NONBLOCK);
    
    m_session_group = new Session_Group();
}

Worker::~Worker()
{
	if(m_cache)
		delete m_cache;
	m_srvobjmap.ReleaseAll();
    if(m_sockfd > 0)
        close(m_sockfd);
    
    if(m_session_group)
        delete m_session_group;
    m_session_group = NULL;
}

void Worker::Working(CUplusTrace& uTrace)
{
#ifdef HEAPHTTPD_DYNAMIC_WORKDERS
	ThreadPool WorkerPool(m_thread_num, INIT_THREAD_POOL_HANDLER, START_THREAD_POOL_HANDLER, this, 0, LEAVE_THREAD_POOL_HANDLER,
		CHttpBase::m_thread_increase_step);
#else
	ThreadPool WorkerPool(m_thread_num, INIT_THREAD_POOL_HANDLER, START_THREAD_POOL_HANDLER, this, 0, LEAVE_THREAD_POOL_HANDLER,
		CHttpBase::m_max_instance_thread_num);
#endif /* HEAPHTTPD_DYNAMIC_WORKDERS*/	
    std::queue<SESSION_PARAM*> local_session_queue;
    
	bool bQuit = false;
    
    time_t last_service_idle_time = time(NULL);
    
	while(!bQuit)
	{
		int clt_sockfd;
		CLIENT_PARAM client_param;
        int res = RECV_FD(m_sockfd, &clt_sockfd, &client_param, 1); // 1 second
        if( res < 0)
		{
            if(res == -2)
            {
                if(!local_session_queue.empty() && pthread_mutex_trylock(&m_STATIC_THREAD_POOL_MUTEX) == 0)
                {
                    last_service_idle_time = time(NULL);
                    
                    while(!local_session_queue.empty())
                    {
                        SESSION_PARAM* previous_session_param = local_session_queue.front();
                        local_session_queue.pop();
                        m_STATIC_THREAD_POOL_ARG_QUEUE.push(previous_session_param);
                        sem_post(&m_STATIC_THREAD_POOL_SEM);
                    }
                }
#ifdef HEAPHTTPD_DYNAMIC_WORKDERS
                if((time(NULL) - last_service_idle_time) < CHttpBase::m_service_idle_timeout)
                {
                    continue;
                }
                
                pthread_mutex_lock(&m_STATIC_THREAD_POOL_SIZE_MUTEX);
                int current_thread_pool_size = m_STATIC_THREAD_POOL_SIZE;
                pthread_mutex_unlock(&m_STATIC_THREAD_POOL_SIZE_MUTEX);
                
                pthread_rwlock_rdlock(&m_STATIC_THREAD_IDLE_NUM_LOCK);
                int current_idle_thread_num = m_STATIC_THREAD_IDLE_NUM;
                pthread_rwlock_unlock(&m_STATIC_THREAD_IDLE_NUM_LOCK);
                
                if(current_thread_pool_size !=  current_idle_thread_num)
                    continue;
#else
                continue;
#endif /* HEAPHTTPD_DYNAMIC_WORKDERS */
            }           
            
            close(m_sockfd);
            m_sockfd = -1;
            bQuit = true;
			break;
		}
        else
        {
            last_service_idle_time = time(NULL);
        }
        
		if(clt_sockfd < 0)
		{
			fprintf(stderr, "RECV_FD error, clt_sockfd = %d %s %d", clt_sockfd, __FILE__, __LINE__);
			bQuit = true;
		}

		if(client_param.ctrl == SessionParamQuit)
		{
			printf("QUIT\n");
			bQuit = true;
		}
		else if(client_param.ctrl == SessionParamExt)
		{
			printf("Reload extensions\n");
			CHttpBase::LoadExtensionList();
		}
        else if(client_param.ctrl == SessionParamReverseExt)
		{
			printf("Reload Reverse extensions\n");
			CHttpBase::LoadReverseExtensionList();
		}
        else if(client_param.ctrl == SessionParamUsers)
		{
			printf("Reload Users\n");
			m_cache->reload_users();
		}
		else
		{
			SESSION_PARAM* session_param = new SESSION_PARAM;
			session_param->srvobjmap = &m_srvobjmap;
			session_param->cache = m_cache;
			session_param->sockfd = clt_sockfd;
			session_param->client_ip = client_param.client_ip;
			session_param->https = client_param.https;
			session_param->http2 = client_param.http2;
		    session_param->ca_crt_root = client_param.ca_crt_root;
       		session_param->ca_crt_server = client_param.ca_crt_server;
		    session_param->ca_password = client_param.ca_password;
		    session_param->ca_key_server = client_param.ca_key_server;
		    session_param->client_cer_check = client_param.client_cer_check;
            session_param->worker = this;
			if(pthread_mutex_trylock(&m_STATIC_THREAD_POOL_MUTEX) == 0)
            {
                while(!local_session_queue.empty())
                {
                    SESSION_PARAM* previous_session_param = local_session_queue.front();
                    local_session_queue.pop();
                    m_STATIC_THREAD_POOL_ARG_QUEUE.push(previous_session_param);
                    sem_post(&m_STATIC_THREAD_POOL_SEM);
                }
            
                m_STATIC_THREAD_POOL_ARG_QUEUE.push(session_param);
                pthread_mutex_unlock(&m_STATIC_THREAD_POOL_MUTEX);
                sem_post(&m_STATIC_THREAD_POOL_SEM);
            }
            else
            {
                local_session_queue.push(session_param);
            }
			
#ifdef HEAPHTTPD_DYNAMIC_WORKDERS
			pthread_rwlock_rdlock(&m_STATIC_THREAD_IDLE_NUM_LOCK);
			int current_idle_thread_num = m_STATIC_THREAD_IDLE_NUM;
            pthread_rwlock_unlock(&m_STATIC_THREAD_IDLE_NUM_LOCK);

            pthread_mutex_lock(&m_STATIC_THREAD_POOL_SIZE_MUTEX);
            int current_thread_pool_size = m_STATIC_THREAD_POOL_SIZE;
            pthread_mutex_unlock(&m_STATIC_THREAD_POOL_SIZE_MUTEX);
    
            if(current_idle_thread_num < CHttpBase::m_thread_increase_step && current_thread_pool_size < CHttpBase::m_max_instance_thread_num)
            {
                WorkerPool.More((CHttpBase::m_max_instance_thread_num - current_thread_pool_size) < CHttpBase::m_thread_increase_step ? (CHttpBase::m_max_instance_thread_num - current_thread_pool_size) : CHttpBase::m_thread_increase_step);
            }
#endif /* HEAPHTTPD_DYNAMIC_WORKDERS */
		}
	}
}
//////////////////////////////////////////////////////////////////////////////////
//Service
Service::Service(Service_Type st)
{
	m_sockfd = -1;
    m_sockfd_ssl = -1;
	m_st = st;
	m_service_name = SVR_NAME_TBL[m_st];
}

Service::~Service()
{

}

void Service::Stop()
{
	string strqueue = HEAPHTTPD_POSIX_PREFIX;
	strqueue += m_service_name;
	strqueue += HEAPHTTPD_POSIX_QUEUE_SUFFIX;

	string strsem = HEAPHTTPD_POSIX_PREFIX;
	strsem += m_service_name;
	strsem += HEAPHTTPD_POSIX_SEMAPHORE_SUFFIX;
	
	m_service_qid = mq_open(strqueue.c_str(), O_RDWR);
	m_service_sid = sem_open(strsem.c_str(), O_RDWR);
	if(m_service_qid == (mqd_t)-1 || m_service_sid == SEM_FAILED)
	{
		return;
	}	
        
	stQueueMsg qMsg;
	qMsg.cmd = MSG_EXIT;
	sem_wait(m_service_sid);
	mq_send(m_service_qid, (const char*)&qMsg, sizeof(stQueueMsg), 0);
	sem_post(m_service_sid);
        if(m_service_qid)
		mq_close(m_service_qid);

	if(m_service_sid != SEM_FAILED)
		sem_close(m_service_sid);
        printf("Stop %s OK\n", SVR_DESP_TBL[m_st]);
}

void Service::ReloadConfig()
{
	string strqueue = HEAPHTTPD_POSIX_PREFIX;
	strqueue += m_service_name;
	strqueue += HEAPHTTPD_POSIX_QUEUE_SUFFIX;

	string strsem = HEAPHTTPD_POSIX_PREFIX;
	strsem += m_service_name;
	strsem += HEAPHTTPD_POSIX_SEMAPHORE_SUFFIX;
	
	m_service_qid = mq_open(strqueue.c_str(), O_RDWR);
	m_service_sid = sem_open(strsem.c_str(), O_RDWR);

	if(m_service_qid == (mqd_t)-1 || m_service_sid == SEM_FAILED)
		return;

	stQueueMsg qMsg;
	qMsg.cmd = MSG_GLOBAL_RELOAD;
	sem_wait(m_service_sid);
	mq_send(m_service_qid, (const char*)&qMsg, sizeof(stQueueMsg), 0);
	sem_post(m_service_sid);
	
	if(m_service_qid != (mqd_t)-1)
		mq_close(m_service_qid);
	if(m_service_sid != SEM_FAILED)
		sem_close(m_service_sid);

	printf("Reload %s OK\n", SVR_DESP_TBL[m_st]);
}

void Service::ReloadAccess()
{
	string strqueue = HEAPHTTPD_POSIX_PREFIX;
	strqueue += m_service_name;
	strqueue += HEAPHTTPD_POSIX_QUEUE_SUFFIX;

	string strsem = HEAPHTTPD_POSIX_PREFIX;
	strsem += m_service_name;
	strsem += HEAPHTTPD_POSIX_SEMAPHORE_SUFFIX;
	
	m_service_qid = mq_open(strqueue.c_str(), O_RDWR);
	m_service_sid = sem_open(strsem.c_str(), O_RDWR);

	if(m_service_qid == (mqd_t)-1 || m_service_sid == SEM_FAILED)
		return;

	stQueueMsg qMsg;
	qMsg.cmd = MSG_ACCESS_RELOAD;
	sem_wait(m_service_sid);
	mq_send(m_service_qid, (const char*)&qMsg, sizeof(stQueueMsg), 0);
	sem_post(m_service_sid);
	
	if(m_service_qid != (mqd_t)-1)
		mq_close(m_service_qid);
	if(m_service_sid != SEM_FAILED)
		sem_close(m_service_sid);
}

void Service::AppendReject(const char* data)
{
	string strqueue = HEAPHTTPD_POSIX_PREFIX;
	strqueue += m_service_name;
	strqueue += HEAPHTTPD_POSIX_QUEUE_SUFFIX;

	string strsem = HEAPHTTPD_POSIX_PREFIX;
	strsem += m_service_name;
	strsem += HEAPHTTPD_POSIX_SEMAPHORE_SUFFIX;
	
	m_service_qid = mq_open(strqueue.c_str(), O_RDWR);
	m_service_sid = sem_open(strsem.c_str(), O_RDWR);

	if(m_service_qid == (mqd_t)-1 || m_service_sid == SEM_FAILED)
		return;

	stQueueMsg qMsg;
	qMsg.cmd = MSG_REJECT_APPEND;
	strncpy(qMsg.data.reject_ip, data, 255);
	qMsg.data.reject_ip[255] = '\0';

	sem_wait(m_service_sid);
	mq_send(m_service_qid, (const char*)&qMsg, sizeof(stQueueMsg), 0);
	sem_post(m_service_sid);
	
	if(m_service_qid != (mqd_t)-1)
		mq_close(m_service_qid);
	if(m_service_sid != SEM_FAILED)
		sem_close(m_service_sid);
}

void Service::ReloadExtension()
{
	string strqueue = HEAPHTTPD_POSIX_PREFIX;
	strqueue += m_service_name;
	strqueue += HEAPHTTPD_POSIX_QUEUE_SUFFIX;

	string strsem = HEAPHTTPD_POSIX_PREFIX;
	strsem += m_service_name;
	strsem += HEAPHTTPD_POSIX_SEMAPHORE_SUFFIX;
	
	m_service_qid = mq_open(strqueue.c_str(), O_RDWR);
	m_service_sid = sem_open(strsem.c_str(), O_RDWR);

	if(m_service_qid == (mqd_t)-1 || m_service_sid == SEM_FAILED)
		return;

	stQueueMsg qMsg;
	qMsg.cmd = MSG_EXTENSION_RELOAD;
	sem_wait(m_service_sid);
	mq_send(m_service_qid, (const char*)&qMsg, sizeof(stQueueMsg), 0);
	sem_post(m_service_sid);
	
	if(m_service_qid != (mqd_t)-1)
		mq_close(m_service_qid);
	if(m_service_sid != SEM_FAILED)
		sem_close(m_service_sid);
}

void Service::ReloadReverseExtension()
{
	string strqueue = HEAPHTTPD_POSIX_PREFIX;
	strqueue += m_service_name;
	strqueue += HEAPHTTPD_POSIX_QUEUE_SUFFIX;

	string strsem = HEAPHTTPD_POSIX_PREFIX;
	strsem += m_service_name;
	strsem += HEAPHTTPD_POSIX_SEMAPHORE_SUFFIX;
	
	m_service_qid = mq_open(strqueue.c_str(), O_RDWR);
	m_service_sid = sem_open(strsem.c_str(), O_RDWR);

	if(m_service_qid == (mqd_t)-1 || m_service_sid == SEM_FAILED)
		return;

	stQueueMsg qMsg;
	qMsg.cmd = MSG_REVERSE_EXTENSION_RELOAD;
	sem_wait(m_service_sid);
	mq_send(m_service_qid, (const char*)&qMsg, sizeof(stQueueMsg), 0);
	sem_post(m_service_sid);
	
	if(m_service_qid != (mqd_t)-1)
		mq_close(m_service_qid);
	if(m_service_sid != SEM_FAILED)
		sem_close(m_service_sid);
}

void Service::ReloadUsers()
{
	string strqueue = HEAPHTTPD_POSIX_PREFIX;
	strqueue += m_service_name;
	strqueue += HEAPHTTPD_POSIX_QUEUE_SUFFIX;

	string strsem = HEAPHTTPD_POSIX_PREFIX;
	strsem += m_service_name;
	strsem += HEAPHTTPD_POSIX_SEMAPHORE_SUFFIX;
	
	m_service_qid = mq_open(strqueue.c_str(), O_RDWR);
	m_service_sid = sem_open(strsem.c_str(), O_RDWR);

	if(m_service_qid == (mqd_t)-1 || m_service_sid == SEM_FAILED)
		return;

	stQueueMsg qMsg;
	qMsg.cmd = MSG_USERS_RELOAD;
	sem_wait(m_service_sid);
	mq_send(m_service_qid, (const char*)&qMsg, sizeof(stQueueMsg), 0);
	sem_post(m_service_sid);
	
	if(m_service_qid != (mqd_t)-1)
		mq_close(m_service_qid);
	if(m_service_sid != SEM_FAILED)
		sem_close(m_service_sid);
}


int Service::Accept(CUplusTrace& uTrace, int& clt_sockfd, BOOL https, struct sockaddr_storage& clt_addr, socklen_t clt_size)
{
    unsigned int ip_lowbytes;
    
    struct sockaddr_in * v4_addr;
    struct sockaddr_in6 * v6_addr;
        
    char szclientip[INET6_ADDRSTRLEN];
    if (clt_addr.ss_family == AF_INET)
    {
        v4_addr = (struct sockaddr_in*)&clt_addr;
        if(inet_ntop(AF_INET, (void*)&v4_addr->sin_addr, szclientip, INET6_ADDRSTRLEN) == NULL)
        {    
            close(clt_sockfd);
            return 0;
        }
        ip_lowbytes = ntohl(v4_addr->sin_addr.s_addr);
    }
    else if(clt_addr.ss_family == AF_INET6)
    {
        v6_addr = (struct sockaddr_in6*)&clt_addr;
        if(inet_ntop(AF_INET6, (void*)&v6_addr->sin6_addr, szclientip, INET6_ADDRSTRLEN) == NULL)
        {    
            close(clt_sockfd);
            return 0;
        }
        ip_lowbytes = ntohl(v6_addr->sin6_addr.s6_addr32[3]); 
    }
        
    if(CHttpBase::m_instance_balance_scheme[0] == 'R')
    {
        m_next_process++;
    }
    else
    {
        m_next_process = ip_lowbytes;
    }
    
    m_next_process = m_next_process % m_work_processes.size();
    
    string client_ip = szclientip;
    int access_result;
    if(CHttpBase::m_permit_list.size() > 0)
    {
        access_result = FALSE;
        for(int x = 0; x < CHttpBase::m_permit_list.size(); x++)
        {
            if(strmatch(CHttpBase::m_permit_list[x].c_str(), client_ip.c_str()) == TRUE)
            {
                access_result = TRUE;
                break;
            }
        }
        
        for(int x = 0; x < CHttpBase::m_reject_list.size(); x++)
        {
            if( (strmatch(CHttpBase::m_reject_list[x].ip.c_str(), (char*)client_ip.c_str()) == TRUE)
                && (time(NULL) < CHttpBase::m_reject_list[x].expire) )
            {
                access_result = FALSE;
                break;
            }
        }
    }
    else
    {
        access_result = TRUE;
        for(int x = 0; x < CHttpBase::m_reject_list.size(); x++)
        {
            if( (strmatch(CHttpBase::m_reject_list[x].ip.c_str(), (char*)client_ip.c_str()) == TRUE)
                && (time(NULL) < CHttpBase::m_reject_list[x].expire) )
            {
                access_result = FALSE;
                break;
            }
        }
    }
    
    if(access_result == FALSE)
    {
        uTrace.Write(Trace_Msg, "Reject %s", client_ip.c_str());
        close(clt_sockfd);
        return 0;
    }
    else
    {					                    
        char pid_file[1024];
        sprintf(pid_file, "/tmp/heaphttpd/%s_WORKER%d.pid",
            m_service_name.c_str(), m_next_process);
        if(check_pid_file(pid_file) == true) /* The related process had crashed */
        {
            WORK_PROCESS_INFO  wpinfo;
            if (socketpair(AF_UNIX, SOCK_DGRAM, 0, wpinfo.sockfds) < 0)
                fprintf(stderr, "socketpair error, %s %d\n", __FILE__, __LINE__);
            
            int work_pid = fork();
            if(work_pid == 0)
            {
                if(m_sockfd > 0)
                    close(m_sockfd); //close service socket
                if(m_sockfd_ssl > 0)
                    close(m_sockfd_ssl); //close ssl service socket
                
                close(clt_sockfd);
                
                if(lock_pid_file(pid_file) == false)
                {
                    printf("lock pid file failed: %s\n", pid_file);
                    exit(-1);
                }
                close(wpinfo.sockfds[0]);
                wpinfo.sockfds[0] = -1;
                
                uTrace.Write(Trace_Msg, "Create worker process [%u]", m_next_process);
                Worker * pWorker = new Worker(m_service_name.c_str(), m_next_process,
                    CHttpBase::m_max_instance_thread_num, wpinfo.sockfds[1]);
                pWorker->Working(uTrace);
                delete pWorker;
                close(wpinfo.sockfds[1]);
                wpinfo.sockfds[1] = -1;
                
                uTrace.Write(Trace_Msg, "Quit from workder process [%d]\n", m_next_process);
                exit(0);
            }
            else if(work_pid > 0)
            {   
                close(wpinfo.sockfds[1]);
                wpinfo.sockfds[1] = -1;
                wpinfo.pid = work_pid;
                
                int flags = fcntl(wpinfo.sockfds[0], F_GETFL, 0); 
                fcntl(wpinfo.sockfds[0], F_SETFL, flags | O_NONBLOCK);
                
                if(m_work_processes[m_next_process].sockfds[0] > 0)
                    close(m_work_processes[m_next_process].sockfds[0]);
                
                if(m_work_processes[m_next_process].sockfds[1] > 0)
                    close(m_work_processes[m_next_process].sockfds[1]);
                            
                m_work_processes[m_next_process].sockfds[0] = wpinfo.sockfds[0];
                m_work_processes[m_next_process].sockfds[1] = wpinfo.sockfds[1];
                m_work_processes[m_next_process].pid = wpinfo.pid;
            }
            else
            {
                return 0;
            }
        }
        
        CLIENT_PARAM client_param;
        strncpy(client_param.client_ip, client_ip.c_str(), 127);
        client_param.client_ip[127] = '\0';
        client_param.https = https;
        client_param.http2 = CHttpBase::m_enablehttp2;
        strncpy(client_param.ca_crt_root, CHttpBase::m_ca_crt_root.c_str(), 255);
        client_param.ca_crt_root[255] = '\0';
        strncpy(client_param.ca_crt_server, CHttpBase::m_ca_crt_server.c_str(), 255);
        client_param.ca_crt_server[255] = '\0';
        strncpy(client_param.ca_password, CHttpBase::m_ca_password.c_str(), 255);
        client_param.ca_password[255] = '\0';
        strncpy(client_param.ca_key_server, CHttpBase::m_ca_key_server.c_str(), 255);
        client_param.ca_key_server[255] = '\0';
        client_param.client_cer_check = CHttpBase::m_client_cer_check;

        client_param.ctrl = SessionParamData;
            
        for(int t = 0; t < m_work_processes.size(); t++)
        {
            if(m_work_processes[m_next_process].sockfds[0] > 0)
            {
                if(SEND_FD(m_work_processes[m_next_process].sockfds[0], clt_sockfd, &client_param, CHttpBase::m_connection_keep_alive_timeout) < 0)
                {
                    printf("fail to sent fd\n");
                    usleep(100);
                    m_next_process++;
                    m_next_process = m_next_process % m_work_processes.size();
                }
                else
                {
                    break;
                }
            }
            else
            {
                m_next_process++;
                m_next_process = m_next_process % m_work_processes.size();
            }
        }
            
        close(clt_sockfd);
    }
    
    return 0;
}

int Service::Run(int fd, const char* hostip, unsigned short http_port, unsigned short https_port)
{	
    CUplusTrace uTrace(HEAPHTTPD_SERVICE_LOGNAME, HEAPHTTPD_SERVICE_LCKNAME);
    
	m_child_list.clear();
	unsigned int result = 0;
	string strqueue = HEAPHTTPD_POSIX_PREFIX;
	strqueue += m_service_name;
	strqueue += HEAPHTTPD_POSIX_QUEUE_SUFFIX;

	string strsem = HEAPHTTPD_POSIX_PREFIX;
	strsem += m_service_name;
	strsem += HEAPHTTPD_POSIX_SEMAPHORE_SUFFIX;
	
	mq_attr attr;
	attr.mq_maxmsg = 8;
	attr.mq_msgsize = 1448; 
	attr.mq_flags = 0;

	m_service_qid = (mqd_t)-1;
	m_service_sid = SEM_FAILED;
	
	m_service_qid = mq_open(strqueue.c_str(), O_CREAT|O_RDWR, 0644, &attr);
	m_service_sid = sem_open(strsem.c_str(), O_CREAT|O_RDWR, 0644, 1);
	if((m_service_qid == (mqd_t)-1) || (m_service_sid == SEM_FAILED))
	{		
		if(m_service_sid != SEM_FAILED)
			sem_close(m_service_sid);
	
		if(m_service_qid != (mqd_t)-1)
			mq_close(m_service_qid);

		sem_unlink(strsem.c_str());
		mq_unlink(strqueue.c_str());

		result = 1;
		write(fd, &result, sizeof(unsigned int));
		close(fd);
		return -1;
	}
	
	CLEAR_QUEUE(m_service_qid);
	
	BOOL svr_exit = FALSE;

	m_next_process = 0;
    
	for(int i = 0; i < CHttpBase::m_max_instance_num; i++)
	{
		char pid_file[1024];
		sprintf(pid_file, "/tmp/heaphttpd/%s_WORKER%d.pid", m_service_name.c_str(), i);
		unlink(pid_file);
		WORK_PROCESS_INFO  wpinfo;
        wpinfo.sockfds[0] = -1;
        wpinfo.sockfds[1] = -1;
        wpinfo.pid = 0;
        if(CHttpBase::m_instance_prestart == TRUE)
        {
            if (socketpair(AF_UNIX, SOCK_DGRAM, 0, wpinfo.sockfds) < 0)
                fprintf(stderr, "socketpair error, %s %d\n", __FILE__, __LINE__);
            int work_pid = fork();
            if(work_pid == 0)
            {
                if(m_sockfd > 0)
                    close(m_sockfd); //close service socket
                if(m_sockfd_ssl > 0)
                    close(m_sockfd_ssl); //close ssl service socket
                
                if(lock_pid_file(pid_file) == false)
                {
                    printf("lock pid file failed: %s\n", pid_file);
                    exit(-1);
                }
                
                close(wpinfo.sockfds[0]);
                wpinfo.sockfds[0] = -1;
                
                uTrace.Write(Trace_Msg, "Create worker process %u", i);
                Worker* pWorker = new Worker(m_service_name.c_str(), i, CHttpBase::m_max_instance_thread_num, wpinfo.sockfds[1]);
                if(pWorker)
                {
                    pWorker->Working(uTrace);
                    delete pWorker;
                }
                close(wpinfo.sockfds[1]);
                wpinfo.sockfds[1] = -1;
                exit(0);
            }
            else if(work_pid > 0)
            {
                close(wpinfo.sockfds[1]);
                wpinfo.sockfds[1] = -1;
                
                wpinfo.pid = work_pid;
                
                int flags = fcntl(wpinfo.sockfds[0], F_GETFL, 0); 
                fcntl(wpinfo.sockfds[0], F_SETFL, flags | O_NONBLOCK); 
                
            }
            else
            {
                fprintf(stderr, "fork error, work_pid = %d, %s %d\n", work_pid, __FILE__, __LINE__);
            }
        }        
        m_work_processes.push_back(wpinfo);
	}

	while(!svr_exit)
	{		
		int nFlag;
        if(http_port > 0)
        {
            struct addrinfo hints;
            struct addrinfo *server_addr, *rp;
            
            memset(&hints, 0, sizeof(struct addrinfo));
            hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
            hints.ai_socktype = SOCK_STREAM; /* Datagram socket */
            hints.ai_flags = AI_PASSIVE;    /* For wildcard IP address */
            hints.ai_protocol = 0;          /* Any protocol */
            hints.ai_canonname = NULL;
            hints.ai_addr = NULL;
            hints.ai_next = NULL;
            
            char szPort[32];
            sprintf(szPort, "%u", http_port);

            int s = getaddrinfo((hostip && hostip[0] != '\0') ? hostip : NULL, szPort, &hints, &server_addr);
            if (s != 0)
            {
               fprintf(stderr, "getaddrinfo: %s %s %d", gai_strerror(s), __FILE__, __LINE__);
               break;
            }
            
            for (rp = server_addr; rp != NULL; rp = rp->ai_next)
            {
               m_sockfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
               if (m_sockfd == -1)
                   continue;
               
               nFlag = 1;
               setsockopt(m_sockfd, SOL_SOCKET, SO_REUSEADDR, (char*)&nFlag, sizeof(nFlag));
            
               if (bind(m_sockfd, rp->ai_addr, rp->ai_addrlen) == 0)
                   break;                  /* Success */

               close(m_sockfd);
            }
            
            if (rp == NULL)
            {               /* No address succeeded */
                  fprintf(stderr, "Could not bind %s %d", __FILE__, __LINE__);
                  break;
            }

            freeaddrinfo(server_addr);           /* No longer needed */
            
            nFlag = fcntl(m_sockfd, F_GETFL, 0);
            fcntl(m_sockfd, F_SETFL, nFlag|O_NONBLOCK);
            
            if(listen(m_sockfd, 128) == -1)
            {
                fprintf(stderr, "Service LISTEN error, %s:%u", hostip ? hostip : "", http_port);
                result = 1;
                write(fd, &result, sizeof(unsigned int));
                close(fd);
                break;
            }
            uTrace.Write(Trace_Msg, "HTTP service works on %s:%u", hostip ? hostip : "", http_port);
        }
        //SSL
        if(https_port > 0)
        {
            struct addrinfo hints2;
            struct addrinfo *server_addr2, *rp2;
            
            memset(&hints2, 0, sizeof(struct addrinfo));
            hints2.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
            hints2.ai_socktype = SOCK_STREAM; /* Datagram socket */
            hints2.ai_flags = AI_PASSIVE;    /* For wildcard IP address */
            hints2.ai_protocol = 0;          /* Any protocol */
            hints2.ai_canonname = NULL;
            hints2.ai_addr = NULL;
            hints2.ai_next = NULL;
            
            char szPort2[32];
            sprintf(szPort2, "%u", https_port);

            int s = getaddrinfo((hostip && hostip[0] != '\0') ? hostip : NULL, szPort2, &hints2, &server_addr2);
            if (s != 0)
            {
               fprintf(stderr, "getaddrinfo: %s %s %d", gai_strerror(s), __FILE__, __LINE__);
               break;
            }
            
            for (rp2 = server_addr2; rp2 != NULL; rp2 = rp2->ai_next)
            {
               m_sockfd_ssl = socket(rp2->ai_family, rp2->ai_socktype, rp2->ai_protocol);
               if (m_sockfd_ssl == -1)
                   continue;
               
               nFlag = 1;
               setsockopt(m_sockfd_ssl, SOL_SOCKET, SO_REUSEADDR, (char*)&nFlag, sizeof(nFlag));
            
               if (bind(m_sockfd_ssl, rp2->ai_addr, rp2->ai_addrlen) == 0)
                   break;                  /* Success */

               close(m_sockfd_ssl);
            }
            
            if (rp2 == NULL)
            {     /* No address succeeded */
                  fprintf(stderr, "Could not bind %s %d", __FILE__, __LINE__);
                  break;
            }

            freeaddrinfo(server_addr2);           /* No longer needed */
            
            nFlag = fcntl(m_sockfd_ssl, F_GETFL, 0);
            fcntl(m_sockfd_ssl, F_SETFL, nFlag|O_NONBLOCK);
            
            if(listen(m_sockfd_ssl, 128) == -1)
            {
                uTrace.Write(Trace_Msg, "Service LISTEN error, %s:%u", hostip ? hostip : "", https_port);
                result = 1;
                write(fd, &result, sizeof(unsigned int));
                close(fd);
                break;
            }
            uTrace.Write(Trace_Msg, "HTTPS service works on %s:%u", hostip ? hostip : "", https_port);
        }
        
        if(m_sockfd == -1 && m_sockfd_ssl == -1)
        {
            result = 1;
            write(fd, &result, sizeof(unsigned int));
            close(fd);
            break;
        }
        BOOL accept_ssl_first = FALSE;
		result = 0;
		write(fd, &result, sizeof(unsigned int));
		close(fd);
		fd_set accept_mask;
    	FD_ZERO(&accept_mask);
		struct timeval accept_timeout;
		struct timespec ts;
        
        int q_buf_len = attr.mq_msgsize;
        char* q_buf_ptr = (char*)malloc(q_buf_len);
            
		stQueueMsg* pQMsg = NULL;
		int rc;
		while(1)
		{	
            while(1)
            {
                pid_t w = waitpid(-1, NULL, WNOHANG);
                if(w > 0)
                    printf("pid %u exits\n", w);
                else
                    break;
            }

			clock_gettime(CLOCK_REALTIME, &ts);

			rc = mq_timedreceive(m_service_qid, q_buf_ptr, q_buf_len, 0, &ts);

			if( rc != -1)
			{
				pQMsg = (stQueueMsg*)q_buf_ptr;
				if(pQMsg->cmd == MSG_EXIT)
				{
					for(int j = 0; j < m_work_processes.size(); j++)
					{
						CLIENT_PARAM client_param;
						client_param.ctrl = SessionParamQuit;
						if(m_work_processes[j].sockfds[0] > 0)
                            SEND_FD(m_work_processes[j].sockfds[0], 0, &client_param);
					}
					svr_exit = TRUE;
					break;
				}
				else if(pQMsg->cmd == MSG_EXTENSION_RELOAD)
				{
					CHttpBase::LoadExtensionList();
					for(int j = 0; j < m_work_processes.size(); j++)
					{
						CLIENT_PARAM client_param;
						client_param.ctrl = SessionParamExt;
						if(m_work_processes[j].sockfds[0] > 0)
                            SEND_FD(m_work_processes[j].sockfds[0], 0, &client_param);
					}
				}
                else if(pQMsg->cmd == MSG_REVERSE_EXTENSION_RELOAD)
				{
					CHttpBase::LoadReverseExtensionList();
					for(int j = 0; j < m_work_processes.size(); j++)
					{
						CLIENT_PARAM client_param;
						client_param.ctrl = SessionParamReverseExt;
						if(m_work_processes[j].sockfds[0] > 0)
                            SEND_FD(m_work_processes[j].sockfds[0], 0, &client_param);
					}
				}
                else if(pQMsg->cmd == MSG_USERS_RELOAD)
				{
					for(int j = 0; j < m_work_processes.size(); j++)
					{
						CLIENT_PARAM client_param;
						client_param.ctrl = SessionParamUsers;
						if(m_work_processes[j].sockfds[0] > 0)
                            SEND_FD(m_work_processes[j].sockfds[0], 0, &client_param);
					}
				}
				else if(pQMsg->cmd == MSG_GLOBAL_RELOAD)
				{
					CHttpBase::UnLoadConfig();
					CHttpBase::LoadConfig();
				}
				else if(pQMsg->cmd == MSG_ACCESS_RELOAD)
				{
					CHttpBase::LoadAccessList();
				}
				else if(pQMsg->cmd == MSG_REJECT_APPEND)
				{
					//firstly erase the expire record
					vector<stReject>::iterator x;
					for(x = CHttpBase::m_reject_list.begin(); x != CHttpBase::m_reject_list.end();)
					{
						if(x->expire < time(NULL))
							CHttpBase::m_reject_list.erase(x);
					}
	
					stReject sr;
					sr.ip = pQMsg->data.reject_ip;
					sr.expire = time(NULL) + 5;
					CHttpBase::m_reject_list.push_back(sr);
				}
			}
			else
			{
				if(errno != ETIMEDOUT && errno != EINTR && errno != EMSGSIZE)
				{
					fprintf(stderr, "mq_timedreceive error, errno = %d, %s, %s %d\n", errno, strerror(errno), __FILE__, __LINE__);
					svr_exit = TRUE;
					break;
				}
				
			}
            
                
            if(m_sockfd > 0)
                FD_SET(m_sockfd, &accept_mask);
            
            //SSL
            if(m_sockfd_ssl > 0)
                FD_SET(m_sockfd_ssl, &accept_mask);
            
			accept_timeout.tv_sec = 1;
			accept_timeout.tv_usec = 0;
			rc = select((m_sockfd > m_sockfd_ssl ? m_sockfd : m_sockfd_ssl) + 1, &accept_mask, NULL, NULL, &accept_timeout);
			if(rc == -1)
			{	
                sleep(5);
				break;
			}
			else if(rc == 0)
			{
                continue;
			}
			else
			{
				BOOL https= FALSE;
                
				struct sockaddr_storage clt_addr, clt_addr_ssl;
                
				socklen_t clt_size = sizeof(struct sockaddr_storage);
                socklen_t clt_size_ssl = sizeof(struct sockaddr_storage);
				int clt_sockfd = -1;
                int clt_sockfd_ssl = -1;
                
                if(FD_ISSET(m_sockfd, &accept_mask))
                {
                    https = FALSE;
                    clt_sockfd = accept(m_sockfd, (sockaddr*)&clt_addr, &clt_size);

                    if(clt_sockfd < 0)
                    {
                        continue;
                    }
                    if(Accept(uTrace, clt_sockfd, https, clt_addr, clt_size) < 0)
                        break;
                }
                
                if(FD_ISSET(m_sockfd_ssl, &accept_mask))
                {
                    https = TRUE;
                    clt_sockfd_ssl = accept(m_sockfd_ssl, (sockaddr*)&clt_addr_ssl, &clt_size_ssl);

                    if(clt_sockfd_ssl < 0)
                    {
                        continue;
                    }
                    if(Accept(uTrace, clt_sockfd_ssl, https, clt_addr_ssl, clt_size_ssl) < 0)
                        break;
                }
			}
		}
		
        free(q_buf_ptr);
        
        if(m_sockfd > 0)
		{
			
			close(m_sockfd);
            m_sockfd = -1;
		}
        
        if(m_sockfd_ssl > 0)
		{
			close(m_sockfd_ssl);
            m_sockfd_ssl = -1;
		}
	}
    
	if(m_service_qid != (mqd_t)-1)
		mq_close(m_service_qid);
	if(m_service_sid != SEM_FAILED)
		sem_close(m_service_sid);

	mq_unlink(strqueue.c_str());
	sem_unlink(strsem.c_str());

	CHttpBase::UnLoadConfig();
	
	return 0;
}

