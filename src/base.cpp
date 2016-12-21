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
BOOL CHttpBase::m_close_stderr = TRUE;
string CHttpBase::m_encoding = "UTF-8";

string CHttpBase::m_private_path = "/tmp/niuhttpd/private";
string CHttpBase::m_work_path = "/var/niuhttpd/";
string CHttpBase::m_ext_list_file = "/etc/niuhttpd/extension.xml";
vector<stExtension> CHttpBase::m_ext_list;

string CHttpBase::m_localhostname = "localhost";
string CHttpBase::m_hostip = "";

BOOL	CHttpBase::m_enablehttp = TRUE;
unsigned short	CHttpBase::m_httpport = 5080;

BOOL	CHttpBase::m_enablehttps = TRUE;
unsigned short	CHttpBase::m_httpsport = 5081;

string   CHttpBase::m_https_cipher = "ALL";

BOOL   CHttpBase::m_enablehttp2 = FALSE;

/* OpenSSL TLSv1.2 default cipher list
 *
    ECDHE-RSA-AES256-GCM-SHA384
    ECDHE-ECDSA-AES256-GCM-SHA384
    ECDHE-RSA-AES256-SHA384
    ECDHE-ECDSA-AES256-SHA384
    DH-DSS-AES256-GCM-SHA384
    DHE-DSS-AES256-GCM-SHA384
    DH-RSA-AES256-GCM-SHA384
    DHE-RSA-AES256-GCM-SHA384
    DHE-RSA-AES256-SHA256
    DHE-DSS-AES256-SHA256
    DH-RSA-AES256-SHA256
    DH-DSS-AES256-SHA256
    ECDH-RSA-AES256-GCM-SHA384
    ECDH-ECDSA-AES256-GCM-SHA384
    ECDH-RSA-AES256-SHA384
    ECDH-ECDSA-AES256-SHA384
    AES256-GCM-SHA384
    AES256-SHA256
    ECDHE-RSA-AES128-GCM-SHA256
    ECDHE-ECDSA-AES128-GCM-SHA256
    ECDHE-RSA-AES128-SHA256
    ECDHE-ECDSA-AES128-SHA256
    DH-DSS-AES128-GCM-SHA256
    DHE-DSS-AES128-GCM-SHA256
    DH-RSA-AES128-GCM-SHA256
    DHE-RSA-AES128-GCM-SHA256
    DHE-RSA-AES128-SHA256
    DHE-DSS-AES128-SHA256
    DH-RSA-AES128-SHA256
    DH-DSS-AES128-SHA256
    ECDH-RSA-AES128-GCM-SHA256
    ECDH-ECDSA-AES128-GCM-SHA256
    ECDH-RSA-AES128-SHA256
    ECDH-ECDSA-AES128-SHA256
    AES128-GCM-SHA256
    AES128-SHA256
*/
string   CHttpBase::m_http2_tls_cipher = "ECDHE-RSA-AES128-GCM-SHA256:ALL";

string CHttpBase::m_www_authenticate = "";
BOOL   CHttpBase::m_client_cer_check = FALSE;
string CHttpBase::m_ca_crt_root   = "/var/niuhttpd/cert/ca.crt";
string CHttpBase::m_ca_crt_server = "/var/niuhttpd/cert/server.crt";
string CHttpBase::m_ca_key_server = "/var/niuhttpd/cert/server.key";
string CHttpBase::m_ca_crt_client = "/var/niuhttpd/cert/client.crt";
string CHttpBase::m_ca_key_client = "/var/niuhttpd/cert/client.key";
string CHttpBase::m_ca_password = "";

string	CHttpBase::m_php_mode = "fpm";
cgi_socket_t  CHttpBase::m_fpm_socktype = unix_socket;
string	CHttpBase::m_fpm_addr = "127.0.0.1";
unsigned short CHttpBase::m_fpm_port = 9000;
string CHttpBase::m_fpm_sockfile = "/var/run/php5-fpm.sock";
string CHttpBase::m_phpcgi_path = "/usr/bin/php-cgi";

map<string, cgi_cfg_t> CHttpBase::m_cgi_list;

unsigned int CHttpBase::m_max_instance_num = 10;
unsigned int CHttpBase::m_max_instance_thread_num = 1024;
BOOL CHttpBase::m_instance_prestart = FALSE;
string CHttpBase::m_instance_balance_scheme = "R";

unsigned int CHttpBase::m_runtime = 0;
string	CHttpBase::m_config_file = CONFIG_FILE_PATH;
string	CHttpBase::m_permit_list_file = PERMIT_FILE_PATH;
string	CHttpBase::m_reject_list_file = REJECT_FILE_PATH;

vector<stReject> CHttpBase::m_reject_list;
vector<string> CHttpBase::m_permit_list;

#ifdef _WITH_MEMCACHED_
    map<string, int> CHttpBase::m_memcached_list;
#endif /* _WITH_MEMCACHED_ */

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
    
    string current_cgi_name = "";
    
	while(getline(configfilein, strline))
	{
		strtrim(strline);
		
		if(strline == "")
			continue;
			
		if(strncasecmp(strline.c_str(), "#", strlen("#")) != 0)
		{	
			if(strncasecmp(strline.c_str(), "CloseStderr", strlen("CloseStderr")) == 0)
			{
				string close_stderr;
				strcut(strline.c_str(), "=", NULL, close_stderr );
				strtrim(close_stderr);
				m_close_stderr = (strcasecmp(close_stderr.c_str(), "yes")) == 0 ? TRUE : FALSE;
			}
            else if(strncasecmp(strline.c_str(), "PrivatePath", strlen("PrivatePath")) == 0)
			{
				strcut(strline.c_str(), "=", NULL, m_private_path);
				strtrim(m_private_path);
			}
			else if(strncasecmp(strline.c_str(), "WorkPath", strlen("WorkPath")) == 0)
			{
				strcut(strline.c_str(), "=", NULL, m_work_path);
				strtrim(m_work_path);
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
			}
			else if(strncasecmp(strline.c_str(), "HostIP", strlen("HostIP")) == 0)
			{
				strcut(strline.c_str(), "=", NULL, m_hostip );
				strtrim(m_hostip);
			}
			else if(strncasecmp(strline.c_str(), "InstanceNum", strlen("InstanceNum")) == 0)
			{
				string maxconn;
				strcut(strline.c_str(), "=", NULL, maxconn );
				strtrim(maxconn);
				m_max_instance_num = atoi(maxconn.c_str());
			}
			else if(strncasecmp(strline.c_str(), "InstanceThreadNum", strlen("InstanceThreadNum")) == 0)
			{
				string maxconn;
				strcut(strline.c_str(), "=", NULL, maxconn );
				strtrim(maxconn);
				m_max_instance_thread_num = atoi(maxconn.c_str());
			}
            else if(strncasecmp(strline.c_str(), "InstancePrestart", strlen("InstancePrestart")) == 0)
			{
				string instance_prestart;
				strcut(strline.c_str(), "=", NULL, instance_prestart );
				strtrim(instance_prestart);
				m_instance_prestart = (strcasecmp(instance_prestart.c_str(), "yes")) == 0 ? TRUE : FALSE;
			}
            else if(strncasecmp(strline.c_str(), "InstanceBalanceScheme", strlen("InstanceBalanceScheme")) == 0)
			{
				strcut(strline.c_str(), "=", NULL, m_instance_balance_scheme );
				strtrim(m_instance_balance_scheme);
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
			}
            else if(strncasecmp(strline.c_str(), "HTTPSCipher", strlen("HTTPSCipher")) == 0)
			{
				strcut(strline.c_str(), "=", NULL, m_https_cipher);
				strtrim(m_https_cipher);
			}
			else if(strncasecmp(strline.c_str(), "HTTP2Enable", strlen("HTTP2Enable")) == 0)
			{
				string HTTP2Enable;
				strcut(strline.c_str(), "=", NULL, HTTP2Enable );
				strtrim(HTTP2Enable);
				m_enablehttp2= (strcasecmp(HTTP2Enable.c_str(), "yes")) == 0 ? TRUE : FALSE;
			}
            else if(strncasecmp(strline.c_str(), "HTTP2TLSCipher", strlen("HTTP2TLSCipher")) == 0)
			{
				strcut(strline.c_str(), "=", NULL, m_http2_tls_cipher);
				strtrim(m_http2_tls_cipher);
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
			}
			else if(strncasecmp(strline.c_str(), "CAServerCrt", strlen("CAServerCrt")) == 0)
			{
				strcut(strline.c_str(), "=", NULL, m_ca_crt_server);
				strtrim(m_ca_crt_server);
			}
			else if(strncasecmp(strline.c_str(), "CAServerKey", strlen("CAServerKey")) == 0)
			{
				strcut(strline.c_str(), "=", NULL, m_ca_key_server);
				strtrim(m_ca_key_server);
			}
			else if(strncasecmp(strline.c_str(), "CAClientCrt", strlen("CAClientCrt")) == 0)
			{
				strcut(strline.c_str(), "=", NULL, m_ca_crt_client);
				strtrim(m_ca_crt_client);
			}
			else if(strncasecmp(strline.c_str(), "CAClientKey", strlen("CAClientKey")) == 0)
			{
				strcut(strline.c_str(), "=", NULL, m_ca_key_client);
				strtrim(m_ca_key_client);
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
                string fpm_socktype;
				strcut(strline.c_str(), "=", NULL, fpm_socktype);
				strtrim(fpm_socktype);
                if(strcasecmp(fpm_socktype.c_str(), "UNIX") == 0)
                    m_fpm_socktype = unix_socket;
                else if(strcasecmp(fpm_socktype.c_str(), "INET") == 0)
                    m_fpm_socktype = inet_socket;
                else
                    m_fpm_socktype = unknow_socket;
                
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
			/* Fast-CGI/S-CGI/uWSGI */
            else if(strncasecmp(strline.c_str(), "CGIName", strlen("CGIName")) == 0)
			{
				strcut(strline.c_str(), "=", NULL, current_cgi_name);
				strtrim(current_cgi_name);
                if(current_cgi_name != "")
                    m_cgi_list[current_cgi_name].cgi_name = current_cgi_name;
			}
            else if(strncasecmp(strline.c_str(), "CGIType", strlen("CGIType")) == 0)
			{
                string cgi_type;
				strcut(strline.c_str(), "=", NULL, cgi_type);
				strtrim(cgi_type);
                
                if(current_cgi_name != "")
                {
                    if(cgi_type[0] == 'F')
                        m_cgi_list[current_cgi_name].cgi_type = fastcgi_e;
                    else if(cgi_type[0] == 'S')
                        m_cgi_list[current_cgi_name].cgi_type = scgi_e;
                    else
                        m_cgi_list[current_cgi_name].cgi_type = none_e;
                }
			}
			else if(strncasecmp(strline.c_str(), "CGIPgm", strlen("CGIPgm")) == 0)
			{
                string cgi_pgm;
				strcut(strline.c_str(), "=", NULL, cgi_pgm);
				strtrim(cgi_pgm);
                if(current_cgi_name != "")
                {
                    m_cgi_list[current_cgi_name].cgi_pgm = cgi_pgm;
                }
			}
			else if(strncasecmp(strline.c_str(), "CGISockType", strlen("CGISockType")) == 0)
			{
                string cgi_socktype;
				strcut(strline.c_str(), "=", NULL, cgi_socktype);
				strtrim(cgi_socktype);
                if(current_cgi_name != "")
                {
                    if(strcasecmp(cgi_socktype.c_str(), "UNIX") == 0)
                        m_cgi_list[current_cgi_name].cgi_socktype = unix_socket;
                    else if(strcasecmp(cgi_socktype.c_str(), "INET") == 0)
                        m_cgi_list[current_cgi_name].cgi_socktype = inet_socket;
                    else
                        m_cgi_list[current_cgi_name].cgi_socktype = unknow_socket;
                }
			}
			else if(strncasecmp(strline.c_str(), "Fast", strlen("CGIIPAddr")) == 0)
			{
                string cgi_addr;
				strcut(strline.c_str(), "=", NULL, cgi_addr);
				strtrim(cgi_addr);
                if(current_cgi_name != "")
                {
                    m_cgi_list[current_cgi_name].cgi_addr = cgi_addr;
                }
			}
			else if(strncasecmp(strline.c_str(), "CGIPort", strlen("CGIPort")) == 0)
			{
				string fastcgi_port;
				strcut(strline.c_str(), "=", NULL, fastcgi_port);
				strtrim(fastcgi_port);
                if(current_cgi_name != "")
                {
                    m_cgi_list[current_cgi_name].cgi_port = atoi(fastcgi_port.c_str());
                }
			}
            else if(strncasecmp(strline.c_str(), "CGISockFile", strlen("CGISockFile")) == 0)
			{
                string cgi_sockfile;
				strcut(strline.c_str(), "=", NULL, cgi_sockfile);
				strtrim(cgi_sockfile);
                if(current_cgi_name != "")
                {
                    m_cgi_list[current_cgi_name].cgi_sockfile = cgi_sockfile;
                }
			}
#ifdef _WITH_MEMCACHED_
            else if(strncasecmp(strline.c_str(), "MEMCACHEDList", strlen("MEMCACHEDList")) == 0)
			{
				string memc_list;
				strcut(strline.c_str(), "=", NULL, memc_list );
				strtrim(memc_list);
				
				string memc_addr, memc_port_str;
				int memc_port;
				
				strcut(memc_list.c_str(), NULL, ":", memc_addr );
				strcut(memc_list.c_str(), ":", NULL, memc_port_str );
				
				strtrim(memc_addr);
				strtrim(memc_port_str);

				memc_port = atoi(memc_port_str.c_str());
				
				m_memcached_list.insert(make_pair<string, int>(memc_addr.c_str(), memc_port));
			}
#endif /* _WITH_MEMCACHED_ */			
			/*else
			{
				printf("%s\n", strline.c_str());
			}*/
			strline = "";
		}
		
	}
	configfilein.close();

	_load_permit_();
	_load_reject_();
	_load_ext_();

	m_runtime = time(NULL);

	return TRUE;
}

BOOL CHttpBase::LoadAccessList()
{
	string strline;
	sem_t* plock = NULL;
	///////////////////////////////////////////////////////////////////////////////
	// GLOBAL_REJECT_LIST
	plock = sem_open("/.NIUHTTPD_GLOBAL_REJECT_LIST.sem", O_CREAT | O_RDWR, 0644, 1);
	if(plock != SEM_FAILED)
	{
		sem_wait(plock);

		_load_permit_();

		sem_post(plock);
		sem_close(plock);
	}
	/////////////////////////////////////////////////////////////////////////////////
	// GLOBAL_PERMIT_LIST
	plock = sem_open("/.NIUHTTPD_GLOBAL_PERMIT_LIST.sem", O_CREAT | O_RDWR, 0644, 1);
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
			sr.expire = 0xFFFFFFFFU;
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