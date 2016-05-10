/*
	Copyright (c) openheap, uplusware
	uplusware@gmail.com
*/

#include "httpservervar.h"

server_var::server_var(const char* name, const char* value)
{
    m_access = m_create = time(NULL); 
    m_name = name;
    m_value = value;
}

server_var::server_var(const char* szline)
{
    vector<string> vVars;
    vSplitString(szline, vVars, ";", TRUE);
    vector<string>::iterator iter_c;
    
    ServerVarParsePhase phase = SERVER_VAR_CREATE;
    for(iter_c = vVars.begin(); iter_c != vVars.end(); iter_c++)
    {
        string strField(*iter_c);
        strtrim(strField);
        
        if(phase == SERVER_VAR_CREATE)
        {
            m_create = atoi(strField.c_str());
            phase == SERVER_VAR_ACCESS;
        }
        else if(phase == SERVER_VAR_ACCESS)
        {
            m_access = atoi(strField.c_str());
            phase == SERVER_VAR_KEYVAL;
        }
        else if(phase == SERVER_VAR_KEYVAL)
        {
            strcut(strField.c_str(), NULL, "=", m_name);
            strcut(strField.c_str(), "=", NULL, m_value);
            strtrim(m_name);
            strtrim(m_value);
            phase == SERVER_VAR_OPTION;
        }
    }
}

server_var::~server_var()
{

}

time_t server_var::getCreateTime()
{
    return m_create;
}

time_t server_var::getAccessTime()
{
    return m_access;
}

void   server_var::setAccessTime(time_t t)
{
    m_access = t;
}

const char* server_var::getName()
{
    return m_name.c_str();
}

const char* server_var::getValue()
{
    return m_value.c_str();
}

