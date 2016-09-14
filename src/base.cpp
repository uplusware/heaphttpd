/*
	Copyright (c) openheap, uplusware
	uplusware@gmail.com
*/
#include <dlfcn.h>
#include "base.h"
#include "util/security.h"
#include "tinyxml/tinyxml.h"

//////////////////////////////////////////////////////////////////////////
//CHttpBase
//
//Software Version
string CHttpBase::m_sw_version = "0.3";

//Global
string CHttpBase::m_encoding = "UTF-8";

string CHttpBase::m_private_path = "/tmp/niuhttpd/private";
string CHttpBase::m_work_path = "/var/niuhttpd/";
string CHttpBase::m_ext_list_file = "/etc/niuhttpd/extension.xml";
vector<stExtension> CHttpBase::m_ext_list;

string CHttpBase::m_localhostname = "uplusware.com";
string CHttpBase::m_hostip = "";

BOOL	CHttpBase::m_enablehttp = TRUE;
unsigned short	CHttpBase::m_httpport = 5080;

BOOL	CHttpBase::m_enablehttps = TRUE;
unsigned short	CHttpBase::m_httpsport = 5081;

BOOL   CHttpBase::m_enablehttp2 = FALSE;

string CHttpBase::m_www_authenticate = "";
BOOL   CHttpBase::m_client_cer_check = FALSE;
string CHttpBase::m_ca_crt_root   = "/var/niuhttpd/cert/ca.crt";
string CHttpBase::m_ca_crt_server = "/var/niuhttpd/cert/server.crt";
string CHttpBase::m_ca_key_server = "/var/niuhttpd/cert/server.key";
string CHttpBase::m_ca_crt_client = "/var/niuhttpd/cert/client.crt";
string CHttpBase::m_ca_key_client = "/var/niuhttpd/cert/client.key";
string CHttpBase::m_ca_password = "";

string	CHttpBase::m_php_mode = "fpm";
string  CHttpBase::m_fpm_socktype = "UNIX";
string	CHttpBase::m_fpm_addr = "127.0.0.1";
unsigned short CHttpBase::m_fpm_port = 9000;
string CHttpBase::m_fpm_sockfile = "/var/run/php5-fpm.sock";
string CHttpBase::m_phpcgi_path = "/usr/bin/php-cgi";

string  CHttpBase::m_fastcgi_name = "webpy";
string  CHttpBase::m_fastcgi_pgm = "/var/niuhttpd/webpy/main.py";
string  CHttpBase::m_fastcgi_socktype = "INET";
string	CHttpBase::m_fastcgi_addr = "127.0.0.1";
unsigned short CHttpBase::m_fastcgi_port = 9001;
string CHttpBase::m_fastcgi_sockfile = "/var/run/webpy-fcgi.sock";

volatile unsigned int CHttpBase::m_global_uid = 0;

unsigned int CHttpBase::m_max_instance_num = 10;
unsigned int CHttpBase::m_max_instance_thread_num = 1024;

unsigned int CHttpBase::m_runtime = 0;
string	CHttpBase::m_config_file = CONFIG_FILE_PATH;
string	CHttpBase::m_permit_list_file = PERMIT_FILE_PATH;
string	CHttpBase::m_reject_list_file = REJECT_FILE_PATH;

vector<stReject> CHttpBase::m_reject_list;
vector<string> CHttpBase::m_permit_list;

/*unsigned char CHttpBase::m_rsa_pub_key[128];
unsigned char CHttpBase::m_rsa_pri_key[128];
*/
CHttpBase::CHttpBase()
{

}

CHttpBase::~CHttpBase()
{
	UnLoadConfig();
}

void CHttpBase::SetConfigFile(const char* config_file, const char* permit_list_file, const char* reject_list_file)
{
	m_config_file = config_file;
	m_permit_list_file = permit_list_file;
	m_reject_list_file = reject_list_file;
}

BOOL CHttpBase::LoadConfig()
{	

	m_permit_list.clear();
	m_reject_list.clear();
	
	ifstream configfilein(m_config_file.c_str(), ios_base::binary);
	string strline = "";
	if(!configfilein.is_open())
	{
		printf("%s is not exist.", m_config_file.c_str());
		return FALSE;
	}
	while(getline(configfilein, strline))
	{
		strtrim(strline);
		
		if(strline == "")
			continue;
			
		if(strncasecmp(strline.c_str(), "#", strlen("#")) != 0)
		{	
			if(strncasecmp(strline.c_str(), "PrivatePath", strlen("PrivatePath")) == 0)
			{
				strcut(strline.c_str(), "=", NULL, m_private_path);
				strtrim(m_private_path);
				//printf("%s\n", m_private_path.c_str());
			}
			else if(strncasecmp(strline.c_str(), "WorkPath", strlen("WorkPath")) == 0)
			{
				strcut(strline.c_str(), "=", NULL, m_work_path);
				strtrim(m_work_path);
				//printf("%s\n", m_work_path.c_str());
			}
			else if(strncasecmp(strline.c_str(), "ExtensionListFile", strlen("ExtensionListFile")) == 0)
			{
				strcut(strline.c_str(), "=", NULL, m_ext_list_file);
				strtrim(m_ext_list_file);
			}
			else if(strncasecmp(strline.c_str(), "LocalHostName", strlen("LocalHostName")) == 0)
			{
				strcut(strline.c_str(), "=", NULL, m_localhostname );
				strtrim(m_localhostname);
				//printf("%s\n", m_localhostname.c_str());
			}
			else if(strncasecmp(strline.c_str(), "HostIP", strlen("HostIP")) == 0)
			{
				strcut(strline.c_str(), "=", NULL, m_hostip );
				strtrim(m_hostip);
				/* printf("[%s]\n", m_hostip.c_str()); */
			}
			else if(strncasecmp(strline.c_str(), "InstanceNum", strlen("InstanceNum")) == 0)
			{
				string maxconn;
				strcut(strline.c_str(), "=", NULL, maxconn );
				strtrim(maxconn);
				m_max_instance_num = atoi(maxconn.c_str());
				/* printf("%d\n", m_max_instance_num); */
			}
			else if(strncasecmp(strline.c_str(), "InstanceThreadNum", strlen("InstanceThreadNum")) == 0)
			{
				string maxconn;
				strcut(strline.c_str(), "=", NULL, maxconn );
				strtrim(maxconn);
				m_max_instance_thread_num = atoi(maxconn.c_str());
				//printf("%d\n", m_max_conn);
			}
			else if(strncasecmp(strline.c_str(), "HTTPEnable", strlen("HTTPEnable")) == 0)
			{
				string HTTPEnable;
				strcut(strline.c_str(), "=", NULL, HTTPEnable );
				strtrim(HTTPEnable);
				m_enablehttps= (strcasecmp(HTTPEnable.c_str(), "yes")) == 0 ? TRUE : FALSE;
			}
			else if(strncasecmp(strline.c_str(), "HTTPPort", strlen("HTTPPort")) == 0)
			{
				string httpport;
				strcut(strline.c_str(), "=", NULL, httpport );
				strtrim(httpport);
				m_httpport = atoi(httpport.c_str());
				//printf("%d\n", m_httpport);
			}
			else if(strncasecmp(strline.c_str(), "HTTPSEnable", strlen("HTTPSEnable")) == 0)
			{
				string HTTPSEnable;
				strcut(strline.c_str(), "=", NULL, HTTPSEnable );
				strtrim(HTTPSEnable);
				m_enablehttps= (strcasecmp(HTTPSEnable.c_str(), "yes")) == 0 ? TRUE : FALSE;
			}
			else if(strncasecmp(strline.c_str(), "HTTPSPort", strlen("HTTPSPort")) == 0)
			{
				string httpsport;
				strcut(strline.c_str(), "=", NULL, httpsport );
				strtrim(httpsport);
				m_httpsport = atoi(httpsport.c_str());
				//printf("%d\n", m_httpsport);
			}
			else if(strncasecmp(strline.c_str(), "HTTP2Enable", strlen("HTTP2Enable")) == 0)
			{
				string HTTP2Enable;
				strcut(strline.c_str(), "=", NULL, HTTP2Enable );
				strtrim(HTTP2Enable);
				m_enablehttp2= (strcasecmp(HTTP2Enable.c_str(), "yes")) == 0 ? TRUE : FALSE;
			}
			else if(strncasecmp(strline.c_str(), "WWWAuthenticate", strlen("WWWAuthenticate")) == 0)
			{
				string www_authenticate;
				strcut(strline.c_str(), "=", NULL, m_www_authenticate );
				strtrim(m_www_authenticate);
			}
			else if(strncasecmp(strline.c_str(), "CheckClientCA", strlen("CheckClientCA")) == 0)
			{
				string CheckClientCA;
				strcut(strline.c_str(), "=", NULL, CheckClientCA );
				strtrim(CheckClientCA);
				m_client_cer_check = (strcasecmp(CheckClientCA.c_str(), "yes")) == 0 ? TRUE : FALSE;
			}
			else if(strncasecmp(strline.c_str(), "CARootCrt", strlen("CARootCrt")) == 0)
			{
				strcut(strline.c_str(), "=", NULL, m_ca_crt_root);
				strtrim(m_ca_crt_root);
				//printf("%s\n", m_ca_crt_root.c_str());
			}
			else if(strncasecmp(strline.c_str(), "CAServerCrt", strlen("CAServerCrt")) == 0)
			{
				strcut(strline.c_str(), "=", NULL, m_ca_crt_server);
				strtrim(m_ca_crt_server);
				//printf("%s\n", m_ca_crt_server.c_str());
			}
			else if(strncasecmp(strline.c_str(), "CAServerKey", strlen("CAServerKey")) == 0)
			{
				strcut(strline.c_str(), "=", NULL, m_ca_key_server);
				strtrim(m_ca_key_server);
				//printf("%s\n", m_ca_crt_server.c_str());
			}
			else if(strncasecmp(strline.c_str(), "CAClientCrt", strlen("CAClientCrt")) == 0)
			{
				strcut(strline.c_str(), "=", NULL, m_ca_crt_client);
				strtrim(m_ca_crt_client);
				//printf("%s\n", m_ca_crt_client.c_str());
			}
			else if(strncasecmp(strline.c_str(), "CAClientKey", strlen("CAClientKey")) == 0)
			{
				strcut(strline.c_str(), "=", NULL, m_ca_key_client);
				strtrim(m_ca_key_client);
				//printf("%s\n", m_ca_crt_client.c_str());
			}
			else if(strncasecmp(strline.c_str(), "CAPassword", strlen("CAPassword")) == 0)
			{
				m_ca_password = "";
				
				string strEncoded;
				strcut(strline.c_str(), "=", NULL, strEncoded);
				strtrim(strEncoded);

				Security::Decrypt(strEncoded.c_str(), strEncoded.length(), m_ca_password);
			}
			else if(strncasecmp(strline.c_str(), "PHPMode", strlen("PHPMode")) == 0)
			{
				strcut(strline.c_str(), "=", NULL, m_php_mode);
				strtrim(m_php_mode);
			}
            else if(strncasecmp(strline.c_str(), "FPMSockType", strlen("FPMSockType")) == 0)
			{
				strcut(strline.c_str(), "=", NULL, m_fpm_socktype);
				strtrim(m_fpm_socktype);
			}
			else if(strncasecmp(strline.c_str(), "FPMIPAddr", strlen("FPMIPAddr")) == 0)
			{
				strcut(strline.c_str(), "=", NULL, m_fpm_addr);
				strtrim(m_fpm_addr);
			}
			else if(strncasecmp(strline.c_str(), "FPMPort", strlen("FPMPort")) == 0)
			{
				string fpm_port;
				strcut(strline.c_str(), "=", NULL, fpm_port);
				strtrim(fpm_port);
				m_fpm_port = atoi(fpm_port.c_str());
			}
            else if(strncasecmp(strline.c_str(), "FPMSockFile", strlen("FPMSockFile")) == 0)
			{
				strcut(strline.c_str(), "=", NULL, m_fpm_sockfile);
				strtrim(m_fpm_sockfile);
			}
            else if(strncasecmp(strline.c_str(), "PHPCGIPath", strlen("PHPCGIPath")) == 0)
			{
				strcut(strline.c_str(), "=", NULL, m_phpcgi_path);
				strtrim(m_phpcgi_path);
			}
			/* Fast-CGI */
			else if(strncasecmp(strline.c_str(), "FastCGIName", strlen("FastCGIName")) == 0)
			{
				strcut(strline.c_str(), "=", NULL, m_fastcgi_name);
				strtrim(m_fastcgi_name);
			}
			else if(strncasecmp(strline.c_str(), "FastCGIPgm", strlen("FastCGIPgm")) == 0)
			{
				strcut(strline.c_str(), "=", NULL, m_fastcgi_pgm);
				strtrim(m_fastcgi_pgm);
			}
			else if(strncasecmp(strline.c_str(), "FastCGISockType", strlen("FastCGISockType")) == 0)
			{
				strcut(strline.c_str(), "=", NULL, m_fastcgi_socktype);
				strtrim(m_fastcgi_socktype);
			}
			else if(strncasecmp(strline.c_str(), "FastCGIIPAddr", strlen("FastCGIIPAddr")) == 0)
			{
				strcut(strline.c_str(), "=", NULL, m_fastcgi_addr);
				strtrim(m_fastcgi_addr);
			}
			else if(strncasecmp(strline.c_str(), "FastCGIPort", strlen("FastCGIPort")) == 0)
			{
				string fastcgi_port;
				strcut(strline.c_str(), "=", NULL, fastcgi_port);
				strtrim(fastcgi_port);
				m_fastcgi_port = atoi(fastcgi_port.c_str());
			}
            else if(strncasecmp(strline.c_str(), "FastCGISockFile", strlen("FastCGISockFile")) == 0)
			{
				strcut(strline.c_str(), "=", NULL, m_fastcgi_sockfile);
				strtrim(m_fastcgi_sockfile);
			}
			
			else
			{
				printf("%s\n", strline.c_str());
			}
			strline = "";
		}
		
	}
	configfilein.close();

	_load_permit_();
	_load_reject_();
	_load_ext_();

	m_runtime = time(NULL);

	//generate the RSA keys
/*	RSA* rsa = RSA_generate_key(1024, 0x10001, NULL, NULL);
	if(rsa)
	{
		BN_bn2bin( rsa->n, m_rsa_pub_key);
		BN_bn2bin( rsa->d, m_rsa_pri_key );
		
		RSA_free(rsa);
	}
*/	
	return TRUE;
}

BOOL CHttpBase::LoadAccessList()
{
	string strline;
	sem_t* plock = NULL;
	///////////////////////////////////////////////////////////////////////////////
	// GLOBAL_REJECT_LIST
	plock = sem_open("/.GLOBAL_REJECT_LIST.sem", O_CREAT | O_RDWR, 0644, 1);
	if(plock != SEM_FAILED)
	{
		sem_wait(plock);

		_load_permit_();

		sem_post(plock);
		sem_close(plock);
	}
	/////////////////////////////////////////////////////////////////////////////////
	// GLOBAL_PERMIT_LIST
	plock = sem_open("/.GLOBAL_PERMIT_LIST.sem", O_CREAT | O_RDWR, 0644, 1);
	if(plock != SEM_FAILED)
	{
		sem_wait(plock);

		_load_reject_();

		sem_post(plock);
		sem_close(plock);
	}
}

BOOL CHttpBase::LoadExtensionList()
{
	_load_ext_();
}

void CHttpBase::_load_permit_()
{
	string strline;
	m_permit_list.clear();
	ifstream permitfilein(m_permit_list_file.c_str(), ios_base::binary);
	if(!permitfilein.is_open())
	{
		printf("%s is not exist. please creat it", m_permit_list_file.c_str());
		return;
	}
	while(getline(permitfilein, strline))
	{
		strtrim(strline);
		if((strline != "")&&(strncmp(strline.c_str(), "#", 1) != 0))
			m_permit_list.push_back(strline);
	}
	permitfilein.close();
}

void CHttpBase::_load_reject_()
{
	string strline;
	m_reject_list.clear();
	ifstream rejectfilein(m_reject_list_file.c_str(), ios_base::binary);
	if(!rejectfilein.is_open())
	{
		printf("%s is not exist. please creat it", m_reject_list_file.c_str());
		return;
	}
	while(getline(rejectfilein, strline))
	{
		strtrim(strline);
		if((strline != "")&&(strncmp(strline.c_str(), "#", 1) != 0))
		{
			stReject sr;
			sr.ip = strline;
			sr.expire = 0xFFFFFFFF;
			m_reject_list.push_back(sr);
		}
	}
	rejectfilein.close();
}

void CHttpBase::_load_ext_()
{
	for(int x = 0; x < m_ext_list.size(); x++)
    {
    	dlclose(m_ext_list[x].handle);
    }
    m_ext_list.clear();

	TiXmlDocument xmlFileterDoc;
	xmlFileterDoc.LoadFile(m_ext_list_file.c_str());
	TiXmlElement * pRootElement = xmlFileterDoc.RootElement();
	if(pRootElement)
	{
		TiXmlNode* pChildNode = pRootElement->FirstChild("extension");
		while(pChildNode)
		{
			if(pChildNode && pChildNode->ToElement())
			{
				stExtension ext;
				
				ext.handle = dlopen(pChildNode->ToElement()->Attribute("libso"), RTLD_NOW);
				
				if(ext.handle)
				{
					ext.action = pChildNode->ToElement()->Attribute("action");
					m_ext_list.push_back(ext);

					/* printf("%s, action: %s\n", pChildNode->ToElement()->Attribute("libso"), pChildNode->ToElement()->Attribute("action")); */
				}
			}
			pChildNode = pChildNode->NextSibling("extension");
		}
	}
}


BOOL CHttpBase::UnLoadConfig()
{
	for(int x = 0; x < m_ext_list.size(); x++)
    {
    	dlclose(m_ext_list[x].handle);
    }
    m_ext_list.clear();
	return TRUE;
}