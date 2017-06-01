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
        perror("write_lock");
		if((errno == EACCES) || (errno == EAGAIN))
		{   
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

