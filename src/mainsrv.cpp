/*
	Copyright (c) openheap, uplusware
	uplusware@gmail.com
*/
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <mqueue.h>
#include <time.h>
#include <string>
#include <iostream>
#include <syslog.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <pwd.h>
#include <libgen.h>
#include <errno.h>
#include <sys/param.h> 
#include <sys/stat.h> 
#include <iostream>
#include <fstream>
#include <sstream>
#include <iterator>
#include <streambuf>
#include <semaphore.h>
#include "service.h"
#include "base.h"
#include "fstring.h"
#include "util/md5.h"
#include "util/qp.h"
#include "util/trace.h"

using namespace std;

static void usage()
{
	printf("Usage:niuhttpd start | stop | status | reload | access | extension | version [CONFIG FILE]\n");
}

//set to daemon mode
static void daemon_init()
{
	setsid();
	chdir("/");
	umask(0);
	close(STDIN_FILENO);
	close(STDOUT_FILENO);
	close(STDERR_FILENO);
}

char PIDFILE[256] = "/tmp/niuhttpd/niuhttpd.pid";

static int Run()
{
	CUplusTrace uTrace(LOGNAME, LCKNAME);
	uTrace.Write(Trace_Msg, "%s", "Service starts");

	int retVal = 0;
	int http_pid = -1, https_pid = -1;

	do
	{
		int pfd[2];
		pipe(pfd);
		
		if(CHttpBase::m_enablehttp)
		{
			pipe(pfd);
			http_pid = fork();
			if(http_pid == 0)
			{
				char szFlag[128];
				sprintf(szFlag, "/tmp/niuhttpd/%s.pid", SVR_NAME_TBL[stHTTP]);
				if(lock_pid_file(szFlag) == false)  
				{   
					printf("%s is aready runing.\n", SVR_DESP_TBL[stHTTP]);   
					exit(-1);  
				}
				
				close(pfd[0]);
				daemon_init();
				Service http_svr(stHTTP);
				http_svr.Run(pfd[1], CHttpBase::m_hostip.c_str(), (unsigned short)CHttpBase::m_httpport);
				exit(0);
			}
			else if(http_pid > 0)
			{
				unsigned int result;
				close(pfd[1]);
				read(pfd[0], &result, sizeof(unsigned int));
				if(result == 0)
					printf("Start HTTP Service OK \t\t\t[%u]\n", http_pid);
				else
				{
					uTrace.Write(Trace_Error, "%s", "Start HTTP Service Failed.");
					printf("Start HTTP Service Failed. \t\t\t[Error]\n");
				}
				close(pfd[0]);
			}
			else
			{
				close(pfd[0]);
				close(pfd[1]);
				retVal = -1;
				break;
			}
		}
		
		if(CHttpBase::m_enablehttps)
		{
			pipe(pfd);
			https_pid = fork();
			if(https_pid == 0)
			{
				char szFlag[128];
				sprintf(szFlag, "/tmp/niuhttpd/%s.pid", SVR_NAME_TBL[stHTTPS]);
				if(lock_pid_file(szFlag) == false)    
				{   
					printf("%s is aready runing.\n", SVR_DESP_TBL[stHTTPS]);   
					exit(-1);  
				}
				
				close(pfd[0]);
				
				daemon_init();
				Service https_svr(stHTTPS);
				https_svr.Run(pfd[1], CHttpBase::m_hostip.c_str(), (unsigned short)CHttpBase::m_httpsport);
				exit(0);
			}
			else if(https_pid > 0)
			{
				unsigned int result;
				close(pfd[1]);
				read(pfd[0], &result, sizeof(unsigned int));
				if(result == 0)
					printf("Start HTTPS Service OK \t\t[%u]\n", https_pid);
				else
				{
					uTrace.Write(Trace_Error, "%s", "Start HTTPS Service Failed.");
					printf("Start HTTPS Service Failed. \t\t[Error]\n");
				}
				close(pfd[0]);
			}
			else
			{
				close(pfd[0]);
				close(pfd[1]);

				retVal = -1;
				break;
			}
		}
	}while(0);
	
	CHttpBase::UnLoadConfig();
	
	return retVal;
}

static int Stop()
{
	printf("Stop niuhttpd service ...\n");
	
	Service http_svr(stHTTP);
	http_svr.Stop();	

	Service https_svr(stHTTPS);
	https_svr.Stop();
}

static void Version()
{
	printf("v%s\n", CHttpBase::m_sw_version.c_str());
}

static int Reload()
{
	printf("Reload niuhttpd configuration ...\n");

	Service http_svr(stHTTP);
	http_svr.ReloadConfig();

	Service https_svr(stHTTPS);
	https_svr.ReloadConfig();
}

static int ReloadAccess()
{
	printf("Reload niuhttpd access list ...\n");

	Service http_svr(stHTTP);
	http_svr.ReloadAccess();

	Service https_svr(stHTTPS);
	https_svr.ReloadAccess();
}

static int ReloadExtension()
{
	printf("Reload niuhttpd extension ...\n");

	Service http_svr(stHTTP);
	http_svr.ReloadExtension();

	Service https_svr(stHTTPS);
	https_svr.ReloadExtension();
}
static int processcmd(const char* cmd, const char* conf, const char* permit, const char* reject)
{
	CHttpBase::SetConfigFile(conf, permit, reject);
	if(!CHttpBase::LoadConfig())
	{
		printf("Load Configure File Failed.\n");
		return -1;
	}
	
	if(strcasecmp(cmd, "stop") == 0)
	{
		Stop();
	}
	else if(strcasecmp(cmd, "start") == 0)
	{
		Run();
	}
	else if(strcasecmp(cmd, "reload") == 0)
	{
		Reload();
	}
	else if(strcasecmp(cmd, "access") == 0)
	{
		ReloadAccess();
	}
	else if(strcasecmp(cmd, "extension") == 0)
	{
		ReloadExtension();
	}
	else if(strcasecmp(cmd, "status") == 0)
	{
		char szFlag[128];
		sprintf(szFlag, "/tmp/niuhttpd/%s.pid", SVR_NAME_TBL[stHTTP]);
		if(check_pid_file(szFlag) == false)    
		{   
			printf("%s is runing.\n", SVR_DESP_TBL[stHTTP]);   
		}
		else
		{
			printf("%s stopped.\n", SVR_DESP_TBL[stHTTP]);   
		}

		sprintf(szFlag, "/tmp/niuhttpd/%s.pid", SVR_NAME_TBL[stHTTPS]);
		if(check_pid_file(szFlag) == false)    
		{   
			printf("%s is runing.\n", SVR_DESP_TBL[stHTTPS]);   
		}
		else
		{
			printf("%s stopped.\n", SVR_DESP_TBL[stHTTPS]);   
		}
		
	}
	else if(strcasecmp(cmd, "version") == 0)
	{
		Version();
	}
	else
	{
		usage();
	}
	CHttpBase::UnLoadConfig();
	return 0;	

}

static void handle_signal(int sid) 
{ 
	signal(SIGPIPE, handle_signal);
}

int main(int argc, char* argv[])
{	
    mkdir("/tmp/niuhttpd", 0777);
    chmod("/tmp/niuhttpd", 0777);


    mkdir("/var/log/niuhttpd/", 0744);
    
    // Set up the signal handler
    signal(SIGPIPE, SIG_IGN);
    sigset_t signals;
    sigemptyset(&signals);
    sigaddset(&signals, SIGPIPE);
    sigprocmask(SIG_BLOCK, &signals, NULL);
    
    if(argc == 2)
    {
        processcmd(argv[1], CONFIG_FILE_PATH, PERMIT_FILE_PATH, REJECT_FILE_PATH);
    }
    else
    {
        usage();
        return -1;
    }
    return 0;
}

