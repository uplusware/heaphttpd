/*
	Copyright (c) openheap, uplusware
	uplusware@gmail.com
*/

#include "service.h"
#include "session.h"
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <semaphore.h>
#include <mqueue.h>
#include <pthread.h>
#include "cache.h"
#include "pool.h"
#include <queue>
#include "util/trace.h"

typedef struct
{
    
	int sockfd;
	string client_ip;
	Service_Type svr_type;
	memory_cache* cache;
	ServiceObjMap* srvobjmap;
} SESSION_PARAM;

enum CLIENT_PARAM_CTRL{
	SessionParamData = 0,
	SessionParamQuit
};

typedef struct {
	CLIENT_PARAM_CTRL ctrl;
	char client_ip[128];
	Service_Type svr_type;
} CLIENT_PARAM;

int SEND_FD(int sfd, int fd_file, CLIENT_PARAM* param) 
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

int RECV_FD(int sfd, int* fd_file, CLIENT_PARAM* param) 
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
    if((nrecv = recvmsg(sfd, &msg, 0)) <= 0)  
    {  
        return nrecv;  
    }
    cmptr = CMSG_FIRSTHDR(&msg);  
    if((cmptr != NULL) && (cmptr->cmsg_len == CMSG_LEN(sizeof(int))))  
    {  
        if(cmptr->cmsg_level != SOL_SOCKET)  
        {  
            printf("control level != SOL_SOCKET/n");  
            exit(-1);  
        }  
        if(cmptr->cmsg_type != SCM_RIGHTS)  
        {  
            printf("control type != SCM_RIGHTS/n");  
            exit(-1);  
        } 
        *fd_file = *((int*)CMSG_DATA(cmptr));  
    }  
    else  
    {  
        if(cmptr == NULL)
			printf("null cmptr, fd not passed.\n");  
        else
			printf("message len[%d] if incorrect.\n", cmptr->cmsg_len);  
        *fd_file = -1; // descriptor was not passed  
    }   
    return *fd_file;  
}

static std::queue<SESSION_PARAM*> STATIC_THREAD_POOL_ARG_QUEUE;

static volatile BOOL STATIC_THREAD_POOL_EXIT = TRUE;
static pthread_mutex_t STATIC_THREAD_POOL_MUTEX;
static sem_t STATIC_THREAD_POOL_SEM;
static volatile unsigned int STATIC_THREAD_POOL_SIZE = 0;

static void SESSION_HANDLING(SESSION_PARAM* session_param)
{
	Session* pSession = NULL;
	pSession = new Session(session_param->srvobjmap, session_param->sockfd, session_param->client_ip.c_str(), session_param->svr_type, session_param->cache);
	if(pSession != NULL)
	{
		pSession->Process();
		delete pSession;
	}
	close(session_param->sockfd);
}

static void INIT_THREAD_POOL_HANDLER()
{
	STATIC_THREAD_POOL_EXIT = TRUE;
	STATIC_THREAD_POOL_SIZE = 0;
	while(!STATIC_THREAD_POOL_ARG_QUEUE.empty())
	{
		STATIC_THREAD_POOL_ARG_QUEUE.pop();
	}
	pthread_mutex_init(&STATIC_THREAD_POOL_MUTEX, NULL);
	sem_init(&STATIC_THREAD_POOL_SEM, 0, 0);
}

static void* START_THREAD_POOL_HANDLER(void* arg)
{
	STATIC_THREAD_POOL_SIZE++;
	struct timespec ts;
	while(STATIC_THREAD_POOL_EXIT)
	{
		clock_gettime(CLOCK_REALTIME, &ts);
		ts.tv_sec += 1;
		if(sem_timedwait(&STATIC_THREAD_POOL_SEM, &ts) == 0)
		{
			SESSION_PARAM* session_param = NULL;
		
			pthread_mutex_lock(&STATIC_THREAD_POOL_MUTEX);
			if(!STATIC_THREAD_POOL_ARG_QUEUE.empty())
			{
				session_param = STATIC_THREAD_POOL_ARG_QUEUE.front();
				STATIC_THREAD_POOL_ARG_QUEUE.pop();			
			}
			pthread_mutex_unlock(&STATIC_THREAD_POOL_MUTEX);
			if(session_param)
			{
				SESSION_HANDLING(session_param);
				delete session_param;
			}
		}
	}
	STATIC_THREAD_POOL_SIZE--;
	if(arg != NULL)
		delete arg;
	
	pthread_exit(0);
}

static void LEAVE_THREAD_POOL_HANDLER()
{
	printf("LEAVE_THREAD_POOL_HANDLER\n");
	STATIC_THREAD_POOL_EXIT = FALSE;

	pthread_mutex_destroy(&STATIC_THREAD_POOL_MUTEX);
	sem_close(&STATIC_THREAD_POOL_SEM);

	unsigned long timeout = 200;
	while(STATIC_THREAD_POOL_SIZE > 0 && timeout > 0)
	{
		usleep(1000*10);
		timeout--;
	}	
}

static void CLEAR_QUEUE(mqd_t qid)
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
Worker::Worker(int thread_num, int sockfd)
{
	m_sockfd = sockfd;
	m_thread_num = thread_num;
	m_cache = new memory_cache();
	string strHtml = CHttpBase::m_work_path;
	strHtml += "/html";
	m_cache->load(strHtml.c_str());
}

Worker::~Worker()
{
	if(m_cache)
		delete m_cache;
	m_srvobjmap.ReleaseAll();
}

void Worker::Working()
{
	ThreadPool WorkerPool(m_thread_num, INIT_THREAD_POOL_HANDLER, START_THREAD_POOL_HANDLER, NULL, LEAVE_THREAD_POOL_HANDLER);
	
	bool bQuit = false;
	while(!bQuit)
	{
		int clt_sockfd;
		CLIENT_PARAM client_param;
		if(RECV_FD(m_sockfd, &clt_sockfd, &client_param)  < 0)
		{
			printf("RECV_FD < 0\n");
			continue;
		}
		if(clt_sockfd < 0)
		{
			perror("");
			bQuit = true;
		}
		if(client_param.ctrl == SessionParamQuit)
		{
			printf("QUIT\n");
			bQuit = true;
		}
		else
		{
			SESSION_PARAM* session_param = new SESSION_PARAM;
			session_param->srvobjmap = &m_srvobjmap;
			session_param->cache = m_cache;
			session_param->sockfd = clt_sockfd;
			session_param->client_ip = client_param.client_ip;
			session_param->svr_type = client_param.svr_type;
						
			pthread_mutex_lock(&STATIC_THREAD_POOL_MUTEX);
			STATIC_THREAD_POOL_ARG_QUEUE.push(session_param);
			pthread_mutex_unlock(&STATIC_THREAD_POOL_MUTEX);

			sem_post(&STATIC_THREAD_POOL_SEM);
		}
	}
}
//////////////////////////////////////////////////////////////////////////////////
//Service
Service::Service(Service_Type st)
{
	m_sockfd = -1;
	m_st = st;
	m_service_name = SVR_NAME_TBL[m_st];
#ifdef CYGWIN
	m_cache = NULL;
	
	if((m_st == stHTTP) || (m_st == stHTTPS))
	{
		m_cache = new memory_cache();
		string strHtml = CHttpBase::m_work_path;
		strHtml += "/html";
		m_cache->load(strHtml.c_str());
	}
#endif /* CYGWIN */
}

Service::~Service()
{
#ifdef CYGWIN
	if(m_cache)
		delete m_cache;
#endif /* CYGWIN */
}

void Service::Stop()
{
	string strqueue = "/.";
	strqueue += m_service_name;
	strqueue += "_queue";

	string strsem = "/.";
	strsem += m_service_name;
	strsem += "_lock";
	
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
	string strqueue = "/.";
	strqueue += m_service_name;
	strqueue += "_queue";

	string strsem = "/.";
	strsem += m_service_name;
	strsem += "_lock";
	
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

void Service::ReloadList()
{
	string strqueue = "/.";
	strqueue += m_service_name;
	strqueue += "_queue";

	string strsem = "/.";
	strsem += m_service_name;
	strsem += "_lock";
	
	m_service_qid = mq_open(strqueue.c_str(), O_RDWR);
	m_service_sid = sem_open(strsem.c_str(), O_RDWR);

	if(m_service_qid == (mqd_t)-1 || m_service_sid == SEM_FAILED)
		return;

	stQueueMsg qMsg;
	qMsg.cmd = MSG_LIST_RELOAD;
	sem_wait(m_service_sid);
	mq_send(m_service_qid, (const char*)&qMsg, sizeof(stQueueMsg), 0);
	sem_post(m_service_sid);
	
	if(m_service_qid != (mqd_t)-1)
		mq_close(m_service_qid);
	if(m_service_sid != SEM_FAILED)
		sem_close(m_service_sid);
}

int Service::Run(int fd, const char* hostip, unsigned short nPort)
{	
	CUplusTrace uTrace(LOGNAME, LCKNAME);
	CHttpBase::LoadConfig();

	m_child_list.clear();
	unsigned int result = 0;
	string strqueue = "/.";
	strqueue += m_service_name;
	strqueue += "_queue";

	string strsem = "/.";
	strsem += m_service_name;
	strsem += "_lock";
	
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
	int qBufLen = attr.mq_msgsize;
	char* qBufPtr = (char*)malloc(qBufLen);

#ifdef CYGWIN
	ThreadPool WorkerPool(CHttpBase::m_max_instance_thread_num, INIT_THREAD_POOL_HANDLER, START_THREAD_POOL_HANDLER, NULL, LEAVE_THREAD_POOL_HANDLER);
#else
	m_next_process = 0;
	for(int i = 0; i < CHttpBase::m_max_instance_num; i++)
	{
		char pid_file[1024];
		sprintf(pid_file, "/tmp/niuhttpd/%s_WORKER%d.pid", m_service_name.c_str(), i);
		WORK_PROCESS_INFO  wpinfo;
		if (socketpair(AF_UNIX, SOCK_DGRAM, 0, wpinfo.sockfds) < 0)
			perror("socketpair");
		int work_pid = fork();
		if(work_pid == 0)
		{
			if(check_single_on(pid_file))
				exit(-1);
			close(wpinfo.sockfds[0]);
			Worker worker(CHttpBase::m_max_instance_thread_num, wpinfo.sockfds[1]);
			worker.Working();
			close(wpinfo.sockfds[1]);
			exit(0);
		}
		else if(work_pid > 0)
		{
			close(wpinfo.sockfds[1]);
			wpinfo.pid = work_pid;
			m_work_processes.push_back(wpinfo);
		}
		else
		{
			perror("fork:");
		}
	}
#endif /* CYGWIN */

	while(!svr_exit)
	{		
		int nFlag;
		
		struct sockaddr_in svr_addr;
		
		bzero(&svr_addr, sizeof(struct sockaddr_in));
		
		m_sockfd = socket(AF_INET, SOCK_STREAM, 0);
		
		svr_addr.sin_family = AF_INET;
		svr_addr.sin_port = htons(nPort);
		svr_addr.sin_addr.s_addr = (checkip(hostip) == -1 ? INADDR_ANY : inet_addr(hostip));
		
		nFlag =1;
		setsockopt(m_sockfd, SOL_SOCKET, SO_REUSEADDR, (char*)&nFlag, sizeof(nFlag));
		
		if(bind(m_sockfd, (struct sockaddr*)&svr_addr, sizeof(struct sockaddr_in)) == -1)
		{
			uTrace.Write(Trace_Error, "Service BIND error, Port: %d.", nPort);
			result = 1;
			write(fd, &result, sizeof(unsigned int));
			close(fd);
			break;
		}
		
		nFlag = fcntl(m_sockfd, F_GETFL, 0);
		fcntl(m_sockfd, F_SETFL, nFlag|O_NONBLOCK);
		
		if(listen(m_sockfd, 128) == -1)
		{
			uTrace.Write(Trace_Error, "Service LISTEN error.");
			result = 1;
			write(fd, &result, sizeof(unsigned int));
			close(fd);
			break;
		}

		result = 0;
		write(fd, &result, sizeof(unsigned int));
		close(fd);
		fd_set accept_mask;
		struct timeval accept_timeout;
		struct timespec ts;
		stQueueMsg* pQMsg;
		int rc;
		while(1)
		{	
			waitpid(-1, NULL, WNOHANG);

			clock_gettime(CLOCK_REALTIME, &ts);
			rc = mq_timedreceive(m_service_qid, qBufPtr, qBufLen, 0, &ts);

			if( rc != -1)
			{
				pQMsg = (stQueueMsg*)qBufPtr;
				if(pQMsg->cmd == MSG_EXIT)
				{
					for(int j = 0; j < m_work_processes.size(); j++)
					{
						CLIENT_PARAM client_param;
						client_param.ctrl = SessionParamQuit;
						
						SEND_FD(m_work_processes[j].sockfds[0], 0, &client_param);
					}
					svr_exit = TRUE;
					break;
				}
				else if(pQMsg->cmd == MSG_GLOBAL_RELOAD)
				{
					CHttpBase::UnLoadConfig();
					CHttpBase::LoadConfig();
				}
				else if(pQMsg->cmd == MSG_LIST_RELOAD)
				{
					CHttpBase::UnLoadList();
					CHttpBase::LoadList();
				}
				else if(pQMsg->cmd == MSG_REJECT)
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
				if((errno != ETIMEDOUT)&&(errno != EINTR)&&(errno != EMSGSIZE))
				{
					perror("");
					svr_exit = TRUE;
					break;
				}
				
			}

			FD_ZERO(&accept_mask);
			FD_SET(m_sockfd, &accept_mask);
			
			accept_timeout.tv_sec = 1;
			accept_timeout.tv_usec = 0;
			rc = select(m_sockfd + 1, &accept_mask, NULL, NULL, &accept_timeout);
			if(rc == 0)
			{	
				continue;
			}
			else if(rc == 1)
			{
				struct sockaddr_in clt_addr;
				socklen_t clt_size = sizeof(struct sockaddr_in);
				int clt_sockfd;
				if(FD_ISSET(m_sockfd, &accept_mask))
				{
					
					CHttpBase::m_global_uid++;
					clt_sockfd = accept(m_sockfd, (sockaddr*)&clt_addr, &clt_size);

					if(clt_sockfd < 0)
					{
						continue;
					}
					string client_ip = inet_ntoa(clt_addr.sin_addr);
					int access_result;
					if(CHttpBase::m_permit_list.size() > 0)
					{
						access_result = FALSE;
						for(int x = 0; x < CHttpBase::m_permit_list.size(); x++)
						{
							if(strlike(CHttpBase::m_permit_list[x].c_str(), client_ip.c_str()) == TRUE)
							{
								access_result = TRUE;
								break;
							}
						}
						
						for(int x = 0; x < CHttpBase::m_reject_list.size(); x++)
						{
							if( (strlike(CHttpBase::m_reject_list[x].ip.c_str(), (char*)client_ip.c_str()) == TRUE) && (time(NULL) < CHttpBase::m_reject_list[x].expire) )
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
							if( (strlike(CHttpBase::m_reject_list[x].ip.c_str(), (char*)client_ip.c_str()) == TRUE) && (time(NULL) < CHttpBase::m_reject_list[x].expire) )
							{
								access_result = FALSE;
								break;
							}
						}
					}
					
					if(access_result == FALSE)
					{
						close(clt_sockfd);
					}
					else
					{
#ifdef CYGWIN						
						SESSION_PARAM* session_param = new SESSION_PARAM;
                				session_param->srvobjmap = &m_srvobjmap;
						session_param->sockfd = clt_sockfd;
						
						session_param->client_ip = client_ip;
						session_param->svr_type = m_st;
						session_param->cache = m_cache;

						pthread_mutex_lock(&STATIC_THREAD_POOL_MUTEX);
						STATIC_THREAD_POOL_ARG_QUEUE.push(session_param);
						pthread_mutex_unlock(&STATIC_THREAD_POOL_MUTEX);

						sem_post(&STATIC_THREAD_POOL_SEM);
#else					
						int next_process_index = m_next_process % m_work_processes.size();
						char pid_file[1024];
						sprintf(pid_file, "/tmp/niuhttpd/%s_WORKER%d.pid", m_service_name.c_str(), next_process_index);
						
						if(!check_single_on(pid_file))
						{
							WORK_PROCESS_INFO  wpinfo;
							if (socketpair(AF_UNIX, SOCK_DGRAM, 0, wpinfo.sockfds) < 0)
								perror("socketpair");
							
							int work_pid = fork();
							if(work_pid == 0)
							{
								if(check_single_on(pid_file))
									exit(-1);
								close(wpinfo.sockfds[0]);
								Worker worker(CHttpBase::m_max_instance_thread_num, wpinfo.sockfds[1]);
								worker.Working();
								close(wpinfo.sockfds[1]);
								exit(0);
							}
							else if(work_pid > 0)
							{
								close(wpinfo.sockfds[1]);
								wpinfo.pid = work_pid;
								m_work_processes[next_process_index] = wpinfo;
							}
							else
							{
								continue;
							}
						}
						CLIENT_PARAM client_param;
						strncpy(client_param.client_ip, client_ip.c_str(), 127);
						client_param.client_ip[127] = '\0';
						client_param.svr_type = m_st;
						client_param.ctrl = SessionParamData;
						SEND_FD(m_work_processes[next_process_index].sockfds[0], clt_sockfd, &client_param);
						m_next_process++;
#endif /* CYGWIN */
		
					}
				}
				else
				{
					continue;
				}
				
			}
			else
			{
				break;
			}
		}
		if(m_sockfd)
		{
			m_sockfd = -1;
			close(m_sockfd);
		}
	}
	free(qBufPtr);
	if(m_service_qid != (mqd_t)-1)
		mq_close(m_service_qid);
	if(m_service_sid != SEM_FAILED)
		sem_close(m_service_sid);

	mq_unlink(strqueue.c_str());
	sem_unlink(strsem.c_str());
#ifdef CYGWIN
	m_srvobjmap.ReleaseAll();
#endif /* CYGWIN */
	CHttpBase::UnLoadConfig();
	
	return 0;
}
