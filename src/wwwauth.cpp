/*
	Copyright (c) openheap, uplusware
	uplusware@gmail.com
*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "wwwauth.h"
#include "util/digcalc.h"
#include "util/base64.h"
#include "util/general.h"
#include "heapauth.h"

void __inline__ _strtrim_dquote_(string &src) /* icnluding double quote mark*/
{
	string::size_type ops1, ops2, ops3, ops4, ops5;
	bool bExit = false;
	while(!bExit)
	{
		bExit = true;
		ops1 = src.find_first_not_of(" ");
		if((ops1 != string::npos)&&(ops1 != 0))
		{
			src.erase(0, ops1);
			bExit = false;
		}
		else if(ops1 == string::npos)
		{
			src.erase(0, src.length());
		}
		ops2 = src.find_first_not_of("\t");
		if((ops2 != string::npos)&&(ops2 != 0))
		{
			src.erase(0, ops2);
		}
		else if(ops2 == string::npos)
		{
			src.erase(0, src.length());
		}
		ops3 = src.find_first_not_of("\r");
		if((ops3 != string::npos)&&(ops3 != 0))
		{
			src.erase(0, ops3);
		}
		else if(ops3 == string::npos)
		{
			src.erase(0, src.length());
		}
			
		ops4 = src.find_first_not_of("\n");
		if((ops4 != string::npos)&&(ops4 != 0))
		{
			src.erase(0, ops4);
		}
		else if(ops4 == string::npos)
		{
			src.erase(0, src.length());
		}
		
		ops5 = src.find_first_not_of("\"");
		if((ops5 != string::npos)&&(ops5 != 0))
		{
			src.erase(0, ops5);
		}
		else if(ops5 == string::npos)
		{
			src.erase(0, src.length());
		}
	}
	bExit = false;
	while(!bExit)
	{
		bExit = true;
		ops1 = src.find_last_not_of(" ");
		if((ops1 != string::npos)&&(ops1 != src.length() - 1 ))
		{
			src.erase(ops1 + 1, src.length() - ops1 - 1);
			bExit = false;
		}
		
		ops2 = src.find_last_not_of("\t");
		if((ops2 != string::npos)&&(ops2 != src.length() - 1 ))
		{
			src.erase(ops2 + 1, src.length() - ops2 - 1);
			bExit = false;
		}
		ops3 = src.find_last_not_of("\r");
		if((ops3 != string::npos)&&(ops3 != src.length() - 1 ))
		{
			src.erase(ops3 + 1, src.length() - ops3 - 1);
			bExit = false;
		}
			
		ops4 = src.find_last_not_of("\n");
		if((ops4 != string::npos)&&(ops4 != src.length() - 1 ))
		{
			src.erase(ops4 + 1, src.length() - ops4 - 1);
			bExit = false;
		}
		ops5 = src.find_last_not_of("\"");
		if((ops5 != string::npos)&&(ops5 != src.length() - 1 ))
		{
			src.erase(ops5 + 1, src.length() - ops5 - 1);
			bExit = false;
		}
	}
}

bool WWW_Auth(AUTH_SCHEME scheme, const char* authinfo, string& username, string &keywords, const char* method)
{
	string password, real_password;
	if(scheme == asBasic)
	{
		string strauth = authinfo;
		int outlen = BASE64_DECODE_OUTPUT_MAX_LEN(strauth.length());
		char * user_pwd_out = (char*)malloc(outlen);
		memset(user_pwd_out, 0, outlen);
		
		CBase64::Decode((char*)strauth.c_str(), strauth.length(), user_pwd_out, &outlen);
		strauth = user_pwd_out;
		free(user_pwd_out);

		strcut(strauth.c_str(), NULL, ":", username);
		strcut(strauth.c_str(), ":", NULL, password);
        
        keywords = password;
        
		if(heaphttpd_usrdef_get_password(username.c_str(), real_password) && password == real_password)
        {
            keywords = real_password;
			return true;
        }
		else
			return false;
	}
	else if(scheme == asDigest)
	{
        keywords = authinfo;
        
		map<string, string> DigestMap;
		char where = 'K'; /* K is for key, V is for value*/
		DigestMap.clear();
		string key, value;
		int x = 0;
		while(1)
		{
			if(authinfo[x] == ',' || authinfo[x] == '\0')
			{
				 _strtrim_dquote_(key);
				if(key != "")
				{
					_strtrim_dquote_(value);
					DigestMap[key] = value;
					key = "";
					value = "";
				}
				where = 'K';
				
			}
			else if(where == 'K' && authinfo[x] == '=')
			{
				where = 'V';
			}
			else
			{
				if( where == 'K' )
					key += (authinfo[x]);
				else if( where == 'V' )
					value += (authinfo[x]);
			}
			
			if(authinfo[x] == '\0')
				break;
			x++;
		}
		
		if(!heaphttpd_usrdef_get_password(DigestMap["username"].c_str(), real_password))
			return false;
       
		char * pszNonce = (char*)DigestMap["nonce"].c_str();
		char * pszCNonce = (char*)DigestMap["cnonce"].c_str();
		char * pszUser = (char*)DigestMap["username"].c_str();
		char * pszRealm = (char*)DigestMap["realm"].c_str();
		char * pszPass = (char*)real_password.c_str();
		char * pszAlg = "md5";
		char * szNonceCount = (char*)DigestMap["nc"].c_str();
		char * pszMethod = (char*)method;
		char * pszQop = (char*)DigestMap["qop"].c_str();
		char * pszURI = (char*)DigestMap["uri"].c_str();        
		HASHHEX HA1;
		HASHHEX HA2 = "";
		HASHHEX Response;
        
		DigestCalcHA1(pszAlg, pszUser, pszRealm, pszPass, pszNonce, pszCNonce, HA1);
		DigestCalcResponse(HA1, pszNonce, szNonceCount, pszCNonce, pszQop, pszMethod, pszURI, HA2, Response);
		if(strncmp(Response, DigestMap["response"].c_str(), 32) == 0)
			return true;
		else
			return false;
	}
	else
		return false;
}