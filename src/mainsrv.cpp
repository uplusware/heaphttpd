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
	printf("Usage:heaphttpd start | stop | status | reload | access | reject [ip] | extension | reverse | version\n");
}

//set to daemon mode
static void daemon_init()
{
	setsid();
	chdir("/");
	umask(0);
    
	close(STDIN_FILENO);
	close(STDOUT_FILENO);
    if(CHttpBase::m_close_stderr)
        close(STDERR_FILENO);
}

char PIDFILE[256] = "/tmp/heaphttpd/heaphttpd.pid";

static int Run()
{
	int retVal = 0;
	int http_pid = -1, https_pid = -1;

	do
	{
		int pfd[2];
		pipe(pfd);
		
		if(CHttpBase::m_enablehttp || CHttpBase::m_enablehttps)
		{
			pipe(pfd);
			http_pid = fork();
			if(http_pid == 0)
			{
				char szFlag[128];
				sprintf(szFlag, "/tmp/heaphttpd/%s.pid", SVR_NAME_TBL[stHTTP]);
				if(lock_pid_file(szFlag) == false)  
				{   
					printf("%s is aready runing.\n", SVR_DESP_TBL[stHTTP]);   
					exit(-1);  
				}
				
				close(pfd[0]);
				daemon_init();
				Service http_svr(stHTTP);
				http_svr.Run(pfd[1], CHttpBase::m_hostip.c_str(), 
                    CHttpBase::m_enablehttp ? (unsigned short)CHttpBase::m_httpport : 0,
                    CHttpBase::m_enablehttps ? (unsigned short)CHttpBase::m_httpsport : 0);
                close(pfd[1]);
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
		
	}while(0);
	
	CHttpBase::UnLoadConfig();
	
	return retVal;
}

static int Stop()
{
	printf("Stop heaphttpd service ...\n");
	
	Service http_svr(stHTTP);
	http_svr.Stop();	
}

static void Version()
{
	printf("v%s\n", CHttpBase::m_sw_version.c_str());
}

static int Reload()
{
	printf("Reload heaphttpd configuration ...\n");

	Service http_svr(stHTTP);
	http_svr.ReloadConfig();
}

static int ReloadAccess()
{
	printf("Reload heaphttpd access list ...\n");

	Service http_svr(stHTTP);
	http_svr.ReloadAccess();
}

static int AppendReject(const char* data)
{
	printf("Append heaphttpd reject list ...\n");

	Service http_svr(stHTTP);
	http_svr.AppendReject(data);
}

static int ReloadExtension()
{
	printf("Reload heaphttpd extension ...\n");

	Service http_svr(stHTTP);
	http_svr.ReloadExtension();
}

static int ReloadReverseExtension()
{
	printf("Reload heaphttpd reverse extension ...\n");

	Service http_svr(stHTTP);
	http_svr.ReloadReverseExtension();
}
static int ReloadUsers()
{
	printf("Reload heaphttpd users ...\n");

	Service http_svr(stHTTP);
	http_svr.ReloadUsers();
}

static int processcmd(const char* cmd, const char* conf, const char* permit, const char* reject, const char* data)
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
	else if(strcasecmp(cmd, "reject") == 0 && data != NULL)
	{
		AppendReject(data);
	}
	else if(strcasecmp(cmd, "extension") == 0)
	{
		ReloadExtension();
	}
    else if(strcasecmp(cmd, "reverse") == 0)
	{
		ReloadReverseExtension();
	}
    else if(strcasecmp(cmd, "users") == 0)
	{
		ReloadUsers();
	}
	else if(strcasecmp(cmd, "status") == 0)
	{
		char szFlag[128];
		sprintf(szFlag, "/tmp/heaphttpd/%s.pid", SVR_NAME_TBL[stHTTP]);
		if(check_pid_file(szFlag) == false)    
		{   
			printf("%s is runing.\n", SVR_DESP_TBL[stHTTP]);   
		}
		else
		{
			printf("%s stopped.\n", SVR_DESP_TBL[stHTTP]);   
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

#include "util/huffman.h"

int main(int argc, char* argv[])
{
    mkdir("/tmp/heaphttpd", 0777);
    chmod("/tmp/heaphttpd", 0777);

    mkdir("/var/log/heaphttpd/", 0744);
    
    // Set up the signal handler
    signal(SIGPIPE, SIG_IGN);
    sigset_t signals;
    sigemptyset(&signals);
    sigaddset(&signals, SIGPIPE);
    sigprocmask(SIG_BLOCK, &signals, NULL);
    
#if OPENSSL_VERSION_NUMBER >= 0x010100000L
    OPENSSL_init_ssl(OPENSSL_INIT_LOAD_SSL_STRINGS, NULL);
#else
    SSL_load_error_strings();
	OpenSSL_add_ssl_algorithms();
#endif /* OPENSSL_VERSION_NUMBER */

    if(argc == 2)
    {
        processcmd(argv[1], CONFIG_FILE_PATH, PERMIT_FILE_PATH, REJECT_FILE_PATH, NULL);
    }
    else if(argc == 3)
    {
        processcmd(argv[1], CONFIG_FILE_PATH, PERMIT_FILE_PATH, REJECT_FILE_PATH, argv[2]);
    }
    else
    {
        usage();
        return -1;
    }
    return 0;
}

