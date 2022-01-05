/*
	Copyright (c) openheap, uplusware
	uplusware@gmail.com
*/

#include "httpsessionvar.h"

session_var::session_var(const char* uid, const char* name, const char* value)
{
    memcpy(m_uid, uid, HTTP_SESSION_UID_LEN);
    m_uid[HTTP_SESSION_UID_LEN] = '\0';
    
    m_access = m_create = time(NULL); 
    m_name = name;
    m_value = value;
}

session_var::session_var(const char* szline)
{
    vector<string> vVars;
    vSplitString(szline, vVars, ";", TRUE);
    vector<string>::iterator iter_c;
    SessionVarParsePhase phase = SESSION_VAR_UID;
    for(iter_c = vVars.begin(); iter_c != vVars.end(); iter_c++)
    {
        string strField(*iter_c);
        strtrim(strField);
        
        if(phase == SESSION_VAR_UID)
        {
            memcpy(m_uid, strField.c_str(), HTTP_SESSION_UID_LEN);
            m_uid[HTTP_SESSION_UID_LEN] = '\0';
            phase = SESSION_VAR_CREATE;
        }
        else if(phase == SESSION_VAR_CREATE)
        {
            m_create = atoi(strField.c_str());
            phase = SESSION_VAR_ACCESS;
        }
        else if(phase == SESSION_VAR_ACCESS)
        {
            m_access = atoi(strField.c_str());
            phase = SESSION_VAR_KEYVAL;
        }
        else if(phase == SESSION_VAR_KEYVAL)
        {
            strcut(strField.c_str(), NULL, "=", m_name);
            strcut(strField.c_str(), "=", NULL, m_value);
            strtrim(m_name);
            strtrim(m_value);
            phase = SESSION_VAR_OPTION;
        }
    }
}

session_var::~session_var()
{

}

time_t session_var::getCreateTime()
{
    return m_create;
}

time_t session_var::getAccessTime()
{
    return m_access;
}

void   session_var::setAccessTime(time_t t)
{
    m_access = t;
}

const char* session_var::getName()
{
    return m_name.c_str();
}

const char* session_var::getValue()
{
    return m_value.c_str();
}

const char* session_var::getUID()
{
    return m_uid;
}

