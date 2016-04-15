/*
	Copyright (c) openheap, uplusware
	uplusware@gmail.com
*/

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <pthread.h>
#include "util/md5.h"
#include "util/sha1.h"
#include "util/uuid.h"
#include "util/sysdep.h"
#include "websocket.h"
#include "fastcgi.h"
#include "wwwauth.h"
#include "htdoc.h"
#include "httpcomm.h"
#include "webdoc.h"

void Htdoc::Response()
{
	if((m_session->GetWWWAuthScheme() == asBasic || m_session->GetWWWAuthScheme() == asDigest)
        && !m_session->IsPassedWWWAuth())
	{
		CHttpResponseHdr header;
		header.SetStatusCode(SC401);
		string strRealm = "User or Administrator";
		
		unsigned char md5key[17];
		sprintf((char*)md5key, "%016lx", pthread_self());
		
		unsigned char szMD5Realm[16];
		char szHexMD5Realm[33];
		HMAC_MD5((unsigned char*)strRealm.c_str(), strRealm.length(), md5key, 16, szMD5Realm);
		sprintf(szHexMD5Realm, "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x", 
			szMD5Realm[0], szMD5Realm[1], szMD5Realm[2], szMD5Realm[3],
			szMD5Realm[4], szMD5Realm[5], szMD5Realm[6], szMD5Realm[7],
			szMD5Realm[8], szMD5Realm[9], szMD5Realm[10], szMD5Realm[11],
			szMD5Realm[12], szMD5Realm[13], szMD5Realm[14], szMD5Realm[15]);
				
		string strVal;
		if(m_session->GetWWWAuthScheme() == asBasic)
		{
			strVal = "Basic realm=\"";
			strVal += strRealm;
			strVal += "\"";
		}
		else if(m_session->GetWWWAuthScheme() == asDigest)
		{
			
			struct timeval tval;
			struct timezone tzone;
		    gettimeofday(&tval, &tzone);
			char szNonce[35];
			srand(time(NULL));
            unsigned long long thisp64 = (unsigned long long)this;
            thisp64 <<= 32;
            thisp64 >>= 32;
            unsigned long thisp32 = (unsigned long)thisp64;
            
			sprintf(szNonce, "%08x%016lx%08x%02x", tval.tv_sec, tval.tv_usec + 0x01B21DD213814000ULL, thisp32, rand()%255);
			
			strVal = "Digest realm=\"";
			strVal += strRealm;
			strVal += "\", qop=\"auth,auth-int\", nonce=\"";
			strVal += szNonce;
			strVal += "\", opaque=\"";
			strVal += szHexMD5Realm;
			strVal += "\"";
			
		}
		//printf("%s\n", strVal.c_str());
		header.SetField("WWW-Authenticate", strVal.c_str());
		
        header.SetField("Content-Type", "text/html");
		header.SetField("Content-Length", header.GetDefaultHTMLLength());
		
		m_session->HttpSend(header.Text(), header.Length());
		m_session->HttpSend(header.GetDefaultHTML(), header.GetDefaultHTMLLength());
		return;
	}

    if(m_session->GetWebSocketHandShake())
    {
        string strwskey = m_session->GetRequestField("Sec-WebSocket-Key");
        strtrim(strwskey);
        if(strwskey != "")
        {
            string strwsaccept = strwskey;
            strwsaccept += "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"; //a fixed UUID
            
            unsigned char Message_Digest[20];
			SHA1Context sha;
            SHA1Reset(&sha);
            SHA1Input(&sha, (unsigned char*)strwsaccept.c_str(), strwsaccept.length());
            SHA1Result(&sha, Message_Digest);
            
            int outlen = BASE64_ENCODE_OUTPUT_MAX_LEN(sizeof(Message_Digest));
            char * szwsaccept_b64 = (char*)malloc(outlen);
            CBase64::Encode((char*)Message_Digest, sizeof(Message_Digest), szwsaccept_b64, &outlen);
            szwsaccept_b64[outlen] = '\0';
            string strwsaccept_b64 = szwsaccept_b64;
            free(szwsaccept_b64);
            
            CHttpResponseHdr header;
            header.SetStatusCode(SC101);
            header.SetField("Upgrade", "websocket");
            header.SetField("Connection", "Upgrade");
            header.SetField("Sec-WebSocket-Accept", strwsaccept_b64.c_str());
            m_session->HttpSend(header.Text(), header.Length());
			
			m_session->SetWebSocketHandShake(Websocket_Ack);
			
            /* - WebSocket handshake finishes, begin to communication -*/
            
            string strResource = m_session->GetResource();
	
            if(strResource != "")
            {
                string strWSFunctionName = "ws_";
                strWSFunctionName += strResource;
                strWSFunctionName += "_main";
            
                string strLibName = m_work_path.c_str();
				strLibName += "/ws/lib";
                strLibName += strResource.c_str();
                strLibName += ".so";
                
                void *lhandle = dlopen(strLibName.c_str(), RTLD_LOCAL | RTLD_LAZY);
                if(!lhandle)
                {
                    printf("dlopen %s\n", dlerror());
                }
                else
                {
                    void* (*ws_main)(int, SSL*);
                    ws_main = (void*(*)(int, SSL*))dlsym(lhandle, strWSFunctionName.c_str());
                    const char* errmsg;
                    if((errmsg = dlerror()) == NULL)
                    {
                        ws_main(m_session->GetSocket(), m_session->GetSSL());
                        dlclose(lhandle);
                    }
                    else
                    {
                        printf("dlsym %s\n", errmsg);
                    }
                }
            }          
            return;
        
        }
    }
	//printf("%d\n", m_session->GetMethod());
	if(m_session->GetMethod() != hmPost && m_session->GetMethod() != hmGet && m_session->GetMethod() != hmHead)
	{
        CHttpResponseHdr header;
		header.SetStatusCode(SC501);
		
        header.SetField("Content-Type", "text/html");
		header.SetField("Content-Length", header.GetDefaultHTMLLength());
        m_session->HttpSend(header.Text(), header.Length());
		m_session->HttpSend(header.GetDefaultHTML(), header.GetDefaultHTMLLength());
		return;
	}
	string strResource = m_session->GetResource();
	string strResp;
	strResp.clear();
	vector <string> vPath;
	
	if(strResource == "")
	{
		strResource = "index.html";
		
	}
	else
	{
		vSplitString(strResource, vPath, "/", TRUE, 0x7FFFFFFF);
		
		int nCDUP = 0;
		int nCDIN = 0;
		for(int x = 0; x < vPath.size(); x++)
		{
			if(vPath[x] == "..")
			{
				nCDUP++;
			}
			else if(vPath[x] == ".")
			{
				//do nothing
			}
			else
			{
				nCDIN++;
			}
		}
		if(nCDUP >= nCDIN)
		{
			strResource = "index.html";
		}
	}
	
    //printf("%s\n", strResource.c_str());
	m_session->SetResource(strResource.c_str());
	
	if(strncmp(strResource.c_str(), "api/", 4) == 0)
	{
        string strApiName = strResource.c_str() + 4;
		string strLibName = m_work_path.c_str();
		strLibName += "/api/lib";
		strLibName += strApiName;
		strLibName += ".so";
		
		//printf("%s\n", strLibName.c_str());
		void *lhandle = dlopen(strLibName.c_str(), RTLD_LOCAL | RTLD_LAZY);
		if(!lhandle)
		{
            CHttpResponseHdr header;
			header.SetStatusCode(SC400);
			
            header.SetField("Content-Type", "text/html");
			header.SetField("Content-Length", header.GetDefaultHTMLLength());
			
			m_session->HttpSend(header.Text(), header.Length());
			m_session->HttpSend(header.GetDefaultHTML(), header.GetDefaultHTMLLength());
			printf("dlopen %s\n", dlerror());
		}
		else
		{
            string strApiFunctionName = "api_";
            strApiFunctionName += strApiName;
            strApiFunctionName += "_response";
			void* (*api_response)(CHttp*, const char*);
			api_response = (void*(*)(CHttp*, const char*))dlsym(lhandle, strApiFunctionName.c_str());
			const char* errmsg;
			if((errmsg = dlerror()) == NULL)
			{
				api_response(m_session, m_work_path.c_str());
				dlclose(lhandle);
			}
			else
			{
				CHttpResponseHdr header;
				header.SetStatusCode(SC500);
                
                header.SetField("Content-Type", "text/html");
				header.SetField("Content-Length", header.GetDefaultHTMLLength());
		
				m_session->HttpSend(header.Text(), header.Length());
				m_session->HttpSend(header.GetDefaultHTML(), header.GetDefaultHTMLLength());
				printf("dlsym %s\n", errmsg);
			}
		}
	
	}
	else if(strncmp(strResource.c_str(), "cgi-bin/", 7) == 0)
	{
		string strcgipath =  m_work_path.c_str();
		strcgipath += "/cgi-bin/";
		strcgipath += m_session->GetCGIPgmName();
        
		if(access(strcgipath.c_str(), 01) != -1)
		{
            int fds[2];
            socketpair(AF_UNIX, SOCK_STREAM, 0, fds);
    
			pid_t cgipid = fork();
			if(cgipid > 0)
			{
                close(fds[1]);
                m_session->WriteDataToCGI(fds[0]);
				shutdown(fds[0], SHUT_WR);
				
                string strResponse, strHeader, strBody;
                m_session->ReadDataFromCGI(fds[0], strResponse);
				if(strResponse != "")
				{
					strcut(strResponse.c_str(), NULL, "\r\n\r\n", strHeader);
					strcut(strResponse.c_str(), "\r\n\r\n", NULL, strBody);
					
					if(strHeader != "")
					{
						strHeader += "\r\n";
					}
					CHttpResponseHdr header;
					header.SetStatusCode(SC200);
					header.SetFields(strHeader.c_str());
					header.SetField("Content-Length", strBody.length());
					m_session->HttpSend(header.Text(), header.Length());
					
					if(strBody != "" && m_session->GetMethod() != hmHead)
						m_session->HttpSend(strBody.c_str(), strBody.length());
                }
				else
				{
					CHttpResponseHdr header;
					header.SetStatusCode(SC500);
					
                    header.SetField("Content-Type", "text/html");
					header.SetField("Content-Length", header.GetDefaultHTMLLength());
					m_session->HttpSend(header.GetDefaultHTML(), header.GetDefaultHTMLLength());
					m_session->HttpSend(header.Text(), header.Length());
				}
                close(fds[0]);
				waitpid(cgipid, NULL, 0);
			}
			else if(cgipid == 0)
			{
                
                close(fds[0]);               
				m_session->SetMetaVarsToEnv();
				dup2(fds[1], STDIN_FILENO);
				dup2(fds[1], STDOUT_FILENO);
				dup2(fds[1], STDERR_FILENO);
                close(fds[1]);
                delete m_session;
                execl(strcgipath.c_str(), NULL);
                exit(-1);
			}
            else
            {
                    close(fds[0]);
                    close(fds[1]);
                
            }
		}
		else
		{
            CHttpResponseHdr header;
            header.SetStatusCode(SC403);
            
            header.SetField("Content-Type", "text/html");
			header.SetField("Content-Length", header.GetDefaultHTMLLength());
			
			m_session->HttpSend(header.Text(), header.Length());
			m_session->HttpSend(header.GetDefaultHTML(), header.GetDefaultHTMLLength());
		}
	}
	else
	{
        string strResource, strExtName;
        lowercase(m_session->GetResource(), strResource);
        get_extend_name(strResource.c_str(), strExtName);
        
        if(strExtName == "php")
        {
			if(strcasecmp(m_php_mode.c_str(), "cgi") == 0)
			{
				int fds[2];
				socketpair(AF_UNIX, SOCK_STREAM, 0, fds);
		
				pid_t cgipid = fork();
				if(cgipid > 0)
				{
					close(fds[1]);
					m_session->WriteDataToCGI(fds[0]);
					shutdown(fds[0], SHUT_WR);
					
					string strResponse, strHeader, strBody;
					m_session->ReadDataFromCGI(fds[0], strResponse);
					if(strResponse != "")
					{
						strcut(strResponse.c_str(), NULL, "\r\n\r\n", strHeader);
						strcut(strResponse.c_str(), "\r\n\r\n", NULL, strBody);
						
						if(strHeader != "")
						{
							strHeader += "\r\n";
						}
						CHttpResponseHdr header;
						header.SetStatusCode(SC200);
						header.SetFields(strHeader.c_str());
						
						string strWanning, strParseError;
						
						strcut(strHeader.c_str(), "PHP Warning:", "\n", strWanning);
						strcut(strHeader.c_str(), "PHP Parse error:", "\n", strParseError);
						
						strtrim(strWanning);
						strtrim(strParseError);
						
						//Replace the body with parse error
						if(strParseError != "")
						{
							strBody = strParseError;
						}
						
						//Append the warnning
						if(strWanning != "")
						{
							strBody = strWanning + strBody;
						}
						header.SetField("Content-Length", strBody.length());
						//printf("%s", header.Text());
						m_session->HttpSend(header.Text(), header.Length());
						if(strBody != "" && m_session->GetMethod() != hmHead)
							m_session->HttpSend(strBody.c_str(), strBody.length());
					}
					else
					{
						CHttpResponseHdr header;
						header.SetStatusCode(SC500);
						
						header.SetField("Content-Type", "text/html");
						header.SetField("Content-Length", header.GetDefaultHTMLLength());
						
						m_session->HttpSend(header.Text(), header.Length());
						m_session->HttpSend(header.GetDefaultHTML(), header.GetDefaultHTMLLength());
					}
					close(fds[0]);
					waitpid(cgipid, NULL, 0);
				}
				else if(cgipid == 0)
				{
					string phpfile = m_work_path.c_str();
					phpfile += "/html/";
					phpfile += m_session->GetResource();
					
					close(fds[0]);
					m_session->SetMetaVar("SCRIPT_FILENAME", phpfile.c_str());
					m_session->SetMetaVar("REDIRECT_STATUS", "200");
					m_session->SetMetaVarsToEnv();
					dup2(fds[1], STDIN_FILENO);
					dup2(fds[1], STDOUT_FILENO);
					dup2(fds[1], STDERR_FILENO);
					close(fds[1]);
					delete m_session;
					execl(m_phpcgi_path.c_str(), "php-cgi", NULL);
					exit(-1);
				}
				else
				{
						close(fds[0]);
						close(fds[1]);
				}
			}
			else if(strcasecmp(m_php_mode.c_str(), "fpm") == 0)
			{
				string phpfile =  m_work_path.c_str();
				phpfile += "/html/";
				phpfile += m_session->GetResource();
				m_session->SetMetaVar("SCRIPT_FILENAME", phpfile.c_str());
				m_session->SetMetaVar("REDIRECT_STATUS", "200");
				
                FastCGI* fcgi = NULL;
                if(strcasecmp(m_fpm_socktype.c_str(), "UNIX") == 0)
                    fcgi = new FastCGI(m_fpm_sockfile.c_str());
                else
    				fcgi = new FastCGI(m_fpm_addr.c_str(), m_fpm_port);
				if(fcgi && fcgi->Connect() == 0 && fcgi->BeginRequest(1) == 0)
				{	
					//printf("BeginRequest end\n");
					if(m_session->m_cgi.m_meta_var.size() > 0)
						fcgi->SendParams(m_session->m_cgi.m_meta_var);
					fcgi->SendEmptyParams();
					if(m_session->m_cgi.GetDataLen() > 0)
					{
						fcgi->Send_STDIN(m_session->m_cgi.GetData(), m_session->m_cgi.GetDataLen());
					}
					fcgi->SendEmpty_STDIN();
					
					string strResponse, strHeader, strBody;
					string strerr;
					unsigned int appstatus;
					unsigned char protocolstatus;
					
					int t = fcgi->RecvAppData(strResponse, strerr, appstatus, protocolstatus);
					//printf("%d [%s]\n", t, strResponse.c_str());
					if(strResponse != "")
					{
						strcut(strResponse.c_str(), NULL, "\r\n\r\n", strHeader);
						strcut(strResponse.c_str(), "\r\n\r\n", NULL, strBody);
						
						if(strHeader != "")
						{
							strHeader += "\r\n";
						}
						CHttpResponseHdr header;
						header.SetStatusCode(SC200);
						header.SetFields(strHeader.c_str());
						
						string strWanning, strParseError;
						
						strcut(strHeader.c_str(), "PHP Warning:", "\n", strWanning);
						strcut(strHeader.c_str(), "PHP Parse error:", "\n", strParseError);
						
						strtrim(strWanning);
						strtrim(strParseError);
						
						//Replace the body with parse error
						if(strParseError != "")
						{
							strBody = strParseError;
						}
						
						//Append the warnning
						if(strWanning != "")
						{
							strBody = strWanning + strBody;
						}
						header.SetField("Content-Length", strBody.length());
						//printf("%s", header.Text());
						m_session->HttpSend(header.Text(), header.Length());
						if(strBody != "" && m_session->GetMethod() != hmHead)
							m_session->HttpSend(strBody.c_str(), strBody.length());
					}
					else
					{
						CHttpResponseHdr header;
						header.SetStatusCode(SC500);
						
						header.SetField("Content-Type", "text/html");
						header.SetField("Content-Length", header.GetDefaultHTMLLength());
						
						m_session->HttpSend(header.Text(), header.Length());
						m_session->HttpSend(header.GetDefaultHTML(), header.GetDefaultHTMLLength());
					}
				}
				else
				{
					CHttpResponseHdr header;
					header.SetStatusCode(SC500);
					
					header.SetField("Content-Type", "text/html");
					header.SetField("Content-Length", header.GetDefaultHTMLLength());
					
					m_session->HttpSend(header.Text(), header.Length());
					m_session->HttpSend(header.GetDefaultHTML(), header.GetDefaultHTMLLength());
				}
                if(fcgi)
                    delete fcgi;
			
			}
			else
			{
				CHttpResponseHdr header;
				header.SetStatusCode(SC501);
				
				header.SetField("Content-Type", "text/html");
				header.SetField("Content-Length", header.GetDefaultHTMLLength());
				
				m_session->HttpSend(header.Text(), header.Length());
				m_session->HttpSend(header.GetDefaultHTML(), header.GetDefaultHTMLLength());
			}
        }
        else
        {
            doc *pDoc = new doc(m_session, m_work_path.c_str());
            pDoc->Response();
            delete pDoc;
        }
	}
}

