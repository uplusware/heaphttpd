/*
	Copyright (c) openheap, uplusware
	uplusware@gmail.com
*/

#ifndef _SERVICE_H_
#define _SERVICE_H_

#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <strings.h>
#include <stdio.h>
#include <sys/wait.h>
#include <sys/select.h>

#include "util/general.h"
#include "util/trace.h"
#include "base.h"
#include "session.h"
#include <mqueue.h>
#include <semaphore.h>
#include "cache.h"
#include "serviceobjmap.h"
#include "posixname.h"

#define DEFAULT_WORK_PROCESS_NUM 1

static const char* SVR_NAME_TBL[] = {NULL, "HTTP", NULL};
static const char* SVR_DESP_TBL[] = {NULL, "HTTP", NULL};

#define write_lock(fd, offset, whence, len) lock_reg(fd, F_SETLK, F_WRLCK, offset, whence, len)   

static int lock_reg(int fd, int cmd, int type, off_t offset, int whence, off_t len)   
{   
	struct   flock   lock;   
	lock.l_type = type;   
	lock.l_start = offset;   
	lock.l_whence = whence;   
	lock.l_len = len;
	return (fcntl(fd,cmd, &lock));   
}   

static bool lock_pid_file(const char* pflag)   
{
	int fd, val;   
	char buf[12];   
	
    if((fd = open(pflag, O_WRONLY|O_CREAT, 0644)) < 0)
	{
        perror("open");
		return false;  
	}
	/* try and set a write lock on the entire file   */   
	if(write_lock(fd, 0, SEEK_SET, 0) < 0)
	{   
        if((errno == EACCES) || (errno == EAGAIN))
		{   
            close(fd);
		    return false;   
		}
		else
		{   
		    close(fd);   
			return false;
		}   
	}   
	
	/* truncate to zero length, now that we have the lock   */   
	if(ftruncate(fd, 0) < 0)
	{   
        perror("ftruncate");
	    close(fd);               
		return false;
	}   
	
	/*   write   our   process   id   */   
	sprintf(buf, "%d\n", ::getpid());   
	if(write(fd, buf, strlen(buf)) != strlen(buf))
	{   
        perror("write");
	    close(fd);               
		return false;
	}   
	
	/*   set close-on-exec flag for descriptor   */   
	if((val = fcntl(fd, F_GETFD, 0) < 0 ))
	{   
        perror("fcntl, F_GETFD");
	    close(fd);   
		return false;
	}   
	val |= FD_CLOEXEC;   
	if(fcntl(fd, F_SETFD, val) <0 )
	{   
        perror("fcntl, F_SETFD");
	    close(fd);   
		return false;
	}
	/* leave file open until we terminate: lock will be held   */  
	return true;   
} 

static bool check_pid_file(const char* pflag)   
{
	int fd, val;   
	char buf[12];   
	
    if((fd = open(pflag, O_WRONLY|O_CREAT, 0644)) < 0)
	{
	    return false;  
	}
	/* try and set a write lock on the entire file   */   
	if(write_lock(fd, 0, SEEK_SET, 0) < 0)
	{   
		if((errno == EACCES) || (errno == EAGAIN))
		{
            close(fd);
		    return false;   
		}
		else
		{   
		    close(fd);   
			return false;
		}   
	}   
	
	/* truncate to zero length, now that we have the lock   */   
	if(ftruncate(fd, 0) < 0)
	{   
	    close(fd);               
		return false;
	}   
	
	/*   write   our   process   id   */   
	sprintf(buf, "%d\n", ::getpid());   
	if(write(fd, buf, strlen(buf)) != strlen(buf))
	{   
	    close(fd);               
		return false;
	}   
	
	/*   set close-on-exec flag for descriptor   */   
	if((val = fcntl(fd, F_GETFD, 0) < 0 ))
	{   
	    close(fd);   
		return false;
	}   
	val |= FD_CLOEXEC;   
	if(fcntl(fd, F_SETFD, val) <0 )
	{   
	    close(fd);   
		return false;
	}
	
	close(fd);
	return true;   
} 

typedef struct
{
    
	int sockfd;
	string client_ip;
	BOOL https;
	BOOL http2;
    string ca_crt_root;
    string ca_crt_server;
    string ca_password;
    string ca_key_server;
    BOOL client_cer_check;
	memory_cache* cache;
	ServiceObjMap* srvobjmap;
} SESSION_PARAM;

enum CLIENT_PARAM_CTRL{
	SessionParamData = 0,
	SessionParamExt,
    SessionParamReverseExt,
    SessionParamUsers,
	SessionParamQuit
};

typedef struct {
	CLIENT_PARAM_CTRL ctrl;
	char client_ip[128];
    BOOL https;
	BOOL http2;
    char ca_crt_root[256];
    char ca_crt_server[256];
    char ca_password[256];
    char ca_key_server[256];
    BOOL client_cer_check;
} CLIENT_PARAM;

class Worker
{
public:
	Worker(const char* service_name, int process_seq, int thread_num, int sockfd);
	virtual ~Worker();

	void Working(CUplusTrace& uTrace);
private:
	memory_cache* m_cache;
	ServiceObjMap m_srvobjmap;
	
	int m_sockfd;
	int m_thread_num;
	int m_process_seq;
	string m_service_name;
//static methods
    static void SESSION_HANDLING(SESSION_PARAM* session_param);
    static void INIT_THREAD_POOL_HANDLER();
    static void* START_THREAD_POOL_HANDLER(void* arg);
    static void LEAVE_THREAD_POOL_HANDLER();
    
    //static variables
    static std::queue<SESSION_PARAM*> m_STATIC_THREAD_POOL_ARG_QUEUE;
    static volatile char m_STATIC_THREAD_POOL_EXIT;
    static pthread_mutex_t m_STATIC_THREAD_POOL_MUTEX;
    static sem_t m_STATIC_THREAD_POOL_SEM;
    static volatile unsigned int m_STATIC_THREAD_POOL_SIZE;
    static pthread_mutex_t m_STATIC_THREAD_POOL_SIZE_MUTEX;
    
    static pthread_rwlock_t m_STATIC_THREAD_IDLE_NUM_LOCK;
    static volatile unsigned int m_STATIC_THREAD_IDLE_NUM;
};

typedef struct {
	int pid;
	int sockfds[2];
} WORK_PROCESS_INFO;

class Service
{
public:
	Service(Service_Type st);
	virtual ~Service();
	int Run(int fd, const char* hostip, unsigned short http_port, unsigned short https_port);
	void Stop();
	void ReloadConfig();
	void ReloadAccess();
	void ReloadExtension();
    void ReloadReverseExtension();
    void ReloadUsers();
	void AppendReject(const char* data);

protected:
    int Accept(CUplusTrace& uTrace, int& clt_sockfd, BOOL https, struct sockaddr_storage& clt_addr, socklen_t clt_size);

	mqd_t m_service_qid;
	sem_t* m_service_sid;
	string m_service_name;
	int m_sockfd;
    int m_sockfd_ssl;
	int m_ctrl_fd;
	Service_Type m_st;
	list<pid_t> m_child_list;
	vector<WORK_PROCESS_INFO> m_work_processes;
	unsigned int m_next_process;
};

#endif /* _SERVICE_H_ */

