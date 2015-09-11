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
#include "base.h"
#include "session.h"
#include <mqueue.h>
#include <semaphore.h>
#include "cache.h"
#include "serviceobj.h"

#define DEFAULT_WORK_PROCESS_NUM 1

static char LOGNAME[256] = "/var/log/niuhttpd/service.log";
static char LCKNAME[256] = "/.niuhttpd_sys.lock";

static const char* SVR_NAME_TBL[] = {NULL, "HTTP", "HTTPS", NULL};
static const char* SVR_DESP_TBL[] = {NULL, "HTTP", "HTTPS", NULL};

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

static int check_single_on(const char* pflag)   
{
	int fd, val;   
	char buf[12];   
	
    if((fd = open(pflag, O_WRONLY|O_CREAT, 0644))   <0   )
	{
		return 1;  
	}   
	
	/* try and set a write lock on the entire file   */   
	if(write_lock(fd, 0, SEEK_SET, 0) < 0)
	{   
		if((errno == EACCES) || (errno == EAGAIN))
		{   
			return 1;   
		}
		else
		{   
			close(fd);   
			return 1;
		}   
	}   
	
	/* truncate to zero length, now that we have the lock   */   
	if(ftruncate(fd, 0) < 0)
	{   
		close(fd);               
		return 1;
	}   
	
	/*   write   our   process   id   */   
	sprintf(buf, "%d\n", ::getpid());   
	if(write(fd, buf, strlen(buf)) != strlen(buf))
	{   
		close(fd);               
		return 1;
	}   
	
	/*   set close-on-exec flag for descriptor   */   
	if((val = fcntl(fd, F_GETFD, 0) < 0 ))
	{   
		close(fd);   
		return 1;
	}   
	val |= FD_CLOEXEC;   
	if(fcntl(fd, F_SETFD, val) <0 )
	{   
		close(fd);   
		return 1;
	}
	/* leave file open until we terminate: lock will be held   */   
	return 0;   
} 

class Worker
{
public:
	Worker(int thread_num, int sockfd);
	virtual ~Worker();

	void Working();
private:
	memory_cache* m_cache;
	ServiceObjMap m_srvobjmap;
	
	int m_sockfd;
	int m_thread_num;
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
	int Run(int fd, const char* hostip, unsigned short nPort);
	void Stop();
	void ReloadConfig();
	void ReloadList();
#ifdef CYGWIN    
	memory_cache* m_cache;
#endif /* CYGWIN*/

protected:
	mqd_t m_service_qid;
#ifdef CYGWIN
	ServiceObjMap m_srvobjmap;
#endif /* CYGWIN*/
	sem_t* m_service_sid;
	string m_service_name;
	int m_sockfd;
	Service_Type m_st;
	list<pid_t> m_child_list;
	vector<WORK_PROCESS_INFO> m_work_processes;
	unsigned int m_next_process;
};

#endif /* _SERVICE_H_ */

