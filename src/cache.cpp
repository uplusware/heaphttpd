/*
        Copyright (c) openheap, uplusware
        uplusware@gmail.com
*/
#include "cache.h"
#include "tinyxml/tinyxml.h"
#include "util/general.h"
#include "util/md5.h"
#include "util/security.h"

#ifdef _WITH_MEMCACHED_
memory_cache::memory_cache(const char* service_name,
                           int process_seq,
                           const char* dirpath,
                           map<string, int>& memcached_list)
#else
memory_cache::memory_cache(const char* service_name,
                           int process_seq,
                           const char* dirpath)
#endif /* _WITH_MEMCACHED_ */
{
  m_process_seq = process_seq;
  m_service_name = service_name;
  m_dirpath = dirpath;

  char hostname[256];
  memset(hostname, 0, 256);
  int ret = gethostname(hostname, sizeof(hostname));
  if (ret != 0) {
    m_localhostname = "localhost";
  } else
    m_localhostname = hostname;

  pthread_rwlock_init(&m_cookie_rwlock, NULL);
  pthread_rwlock_init(&m_session_var_rwlock, NULL);
  pthread_rwlock_init(&m_server_var_rwlock, NULL);
  pthread_rwlock_init(&m_file_rwlock, NULL);
  pthread_rwlock_init(&m_tunneling_rwlock, NULL);
  pthread_rwlock_init(&m_users_rwlock, NULL);

  m_file_cache.clear();
  m_tunneling_cache.clear();
  m_cookies.clear();
  m_session_vars.clear();
  m_server_vars.clear();
  m_type_table.clear();
  m_file_cache_size = 0;
  m_tunneling_cache_size = 0;

#ifdef _WITH_MEMCACHED_
  m_memcached_servers = NULL;
  memcached_return rc;
  m_memcached = NULL;
  for (map<string, int>::iterator iter = memcached_list.begin();
       iter != memcached_list.end(); ++iter) {
    if (!m_memcached)
      m_memcached = memcached_create(NULL);
    m_memcached_servers = memcached_server_list_append(
        m_memcached_servers, (*iter).first.c_str(), (*iter).second, &rc);
    rc = memcached_server_push(m_memcached, m_memcached_servers);
  }
#endif /* _WITH_MEMCACHED_ */
}

memory_cache::~memory_cache() {
  unload();
  pthread_rwlock_destroy(&m_users_rwlock);
  pthread_rwlock_destroy(&m_tunneling_rwlock);
  pthread_rwlock_destroy(&m_file_rwlock);
  pthread_rwlock_destroy(&m_cookie_rwlock);
  pthread_rwlock_destroy(&m_session_var_rwlock);
  pthread_rwlock_destroy(&m_server_var_rwlock);

#ifdef _WITH_MEMCACHED_
  if (m_memcached)
    memcached_free(m_memcached);

  if (m_memcached_servers)
    free(m_memcached_servers);

  m_memcached_servers = NULL;
  m_memcached = NULL;
  map<pthread_t, memcached_st*>::iterator iter_st;

  for (iter_st = m_memcached_map.begin(); iter_st != m_memcached_map.end();
       iter_st++) {
    if (iter_st->second)
      memcached_free(iter_st->second);
  }
#endif /* _WITH_MEMCACHED_ */
}

// Cookie
void memory_cache::push_cookie(const char* name, Cookie* ck) {
  pthread_rwlock_wrlock(&m_cookie_rwlock);
  map<string, Cookie*>::iterator iter = m_cookies.find(name);
  if (iter != m_cookies.end()) {
    iter->second = ck;
  } else {
    m_cookies.insert(map<string, Cookie*>::value_type(name, ck));
  }
  // Save cookie for other process for synchronization
  _save_cookies_();
  pthread_rwlock_unlock(&m_cookie_rwlock);
}

void memory_cache::pop_cookie(const char* name) {
  pthread_rwlock_wrlock(&m_cookie_rwlock);
  map<string, Cookie*>::iterator iter = m_cookies.find(name);
  if (iter != m_cookies.end())
    m_cookies.erase(iter);
  pthread_rwlock_unlock(&m_cookie_rwlock);
}

int memory_cache::get_cookie(const char* name, Cookie* ck) {
  int ret = -1;
  pthread_rwlock_rdlock(&m_cookie_rwlock);
  map<string, Cookie*>::iterator iter = m_cookies.find(name);
  if (iter != m_cookies.end()) {
    ck = iter->second;
    ret = 0;
  }
  pthread_rwlock_unlock(&m_cookie_rwlock);
  return ret;
}

// Session variable
void memory_cache::push_session_var(const char* uid,
                                    const char* name,
                                    const char* value) {
  pthread_rwlock_wrlock(&m_session_var_rwlock);
  session_var* pVar = new session_var(uid, name, value);

  map<session_var_key, session_var*>::iterator iter =
      m_session_vars.find(session_var_key(uid, name));
  if (iter != m_session_vars.end()) {
    iter->second = pVar;
  } else
    m_session_vars.insert(map<session_var_key, session_var*>::value_type(
        session_var_key(uid, name), pVar));
    // Save session vars for other process for synchronization
    /* disable it since the server will balance the connect to the same process
     * via IP*/
#ifndef _WITH_MEMCACHED_
  _save_session_vars_();
#endif /* _WITH_MEMCACHED_ */
  pthread_rwlock_unlock(&m_session_var_rwlock);

#ifdef _WITH_MEMCACHED_
  if (m_memcached) {
    string keyname;
    keyname = uid;
    keyname += "#";
    keyname += name;
    char szMD5dst[41];
    MD5_CTX_OBJ context;
    context.MD5Update((unsigned char*)keyname.c_str(), keyname.length());
    unsigned char digest[16];
    context.MD5Final(digest);
    sprintf(szMD5dst,
            "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
            digest[0], digest[1], digest[2], digest[3], digest[4], digest[5],
            digest[6], digest[7], digest[8], digest[9], digest[10], digest[11],
            digest[12], digest[13], digest[14], digest[15]);
    szMD5dst[32] = '.';
    szMD5dst[33] = 's';
    szMD5dst[34] = 'e';
    szMD5dst[35] = 's';
    szMD5dst[36] = 's';
    szMD5dst[37] = 'v';
    szMD5dst[38] = 'a';
    szMD5dst[39] = 'r';
    szMD5dst[40] = '\0';

    if (m_memcached_map.find(pthread_self()) == m_memcached_map.end())
      m_memcached_map[pthread_self()] = memcached_clone(NULL, m_memcached);
    memcached_return memc_rc =
        memcached_set(m_memcached_map[pthread_self()], szMD5dst, 40, value,
                      strlen(value) + 1, (time_t)0, (uint32_t)0);
  }
#endif /* _WITH_MEMCACHED_ */
}

void memory_cache::save_session_vars() {
  pthread_rwlock_wrlock(&m_session_var_rwlock);
  _save_session_vars_();
  pthread_rwlock_unlock(&m_session_var_rwlock);
}

int memory_cache::get_session_var(const char* uid,
                                  const char* name,
                                  string& value) {
#ifndef _WITH_MEMCACHED_
  reload_session_vars();
#endif /* _WITH_MEMCACHED_ */
  int ret = -1;

  pthread_rwlock_rdlock(&m_session_var_rwlock);
  map<session_var_key, session_var*>::iterator iter =
      m_session_vars.find(session_var_key(uid, name));
  if (iter != m_session_vars.end()) {
    value = iter->second->getValue();
    pthread_rwlock_unlock(&m_session_var_rwlock);
    ret = 0;
  } else {
    pthread_rwlock_unlock(&m_session_var_rwlock);
#ifdef _WITH_MEMCACHED_
    if (m_memcached) {
      string keyname;
      keyname = uid;
      keyname += "#";
      keyname += name;
      char szMD5dst[41];
      MD5_CTX_OBJ context;

      context.MD5Update((unsigned char*)keyname.c_str(), keyname.length());
      unsigned char digest[16];
      context.MD5Final(digest);
      sprintf(
          szMD5dst,
          "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
          digest[0], digest[1], digest[2], digest[3], digest[4], digest[5],
          digest[6], digest[7], digest[8], digest[9], digest[10], digest[11],
          digest[12], digest[13], digest[14], digest[15]);
      szMD5dst[32] = '.';
      szMD5dst[33] = 's';
      szMD5dst[34] = 'e';
      szMD5dst[35] = 's';
      szMD5dst[36] = 's';
      szMD5dst[37] = 'v';
      szMD5dst[38] = 'a';
      szMD5dst[39] = 'r';
      szMD5dst[40] = '\0';

      memcached_return memc_rc;
      size_t memc_value_length;
      uint32_t memc_flags = 0;
      if (m_memcached_map.find(pthread_self()) == m_memcached_map.end())
        m_memcached_map[pthread_self()] = memcached_clone(NULL, m_memcached);
      char* value_buf =
          memcached_get(m_memcached_map[pthread_self()], szMD5dst, 40,
                        &memc_value_length, &memc_flags, &memc_rc);

      if (memc_rc == MEMCACHED_SUCCESS && value_buf) {
        value = value_buf;
        free(value_buf);

        ret = 0;
      }
    }
#endif /* _WITH_MEMCACHED_ */
  }

  return ret;
}

void memory_cache::_save_session_vars_() {
  if (m_session_vars.size() > 0) {
    char szSessionVarFile[256];
    char szSessionVarFilePrefix[256];
    sprintf(szSessionVarFilePrefix, "%s-%d@", m_service_name.c_str(),
            m_process_seq);

    sprintf(szSessionVarFile, "%s/variable/%s%s.session", m_dirpath.c_str(),
            szSessionVarFilePrefix, m_localhostname.c_str());

    unsigned long nVersion = 0;
    map<string, unsigned long>::iterator iter_ver =
        m_session_vars_file_versions.find(szSessionVarFile);
    if (iter_ver != m_session_vars_file_versions.end())
      nVersion = iter_ver->second;

    ofstream* session_var_stream = NULL;

    map<session_var_key, session_var*>::iterator iter_s;
    bool bSave = true;
    for (iter_s = m_session_vars.begin(); iter_s != m_session_vars.end();
         iter_s++) {
      bSave = true;
      if (time(NULL) - iter_s->second->getAccessTime() >
          90) /* No-inactive http request, 90s for timeout*/
      {
        bSave = false;
      }
      if (bSave) {
        if (!session_var_stream) {
          session_var_stream = new ofstream(
              szSessionVarFile, ios::out | ios::binary | ios::trunc);

          if (!session_var_stream || !session_var_stream->is_open())
            break;
          // Write the version in the first line
          nVersion++;
          m_session_vars_file_versions[szSessionVarFile] = nVersion;

          char szVersion[256];
          sprintf(szVersion, "#%lu", nVersion);
          session_var_stream->write(szVersion, strlen(szVersion));
          session_var_stream->write("\n", 1);
        }

        char szTime[256];
        session_var_stream->write(iter_s->second->getUID(),
                                  strlen(iter_s->second->getUID()));
        session_var_stream->write(";", 1);

        sprintf(szTime, "%d", iter_s->second->getCreateTime());
        session_var_stream->write(szTime, strlen(szTime));
        session_var_stream->write(";", 1);

        sprintf(szTime, "%d", iter_s->second->getAccessTime());
        session_var_stream->write(szTime, strlen(szTime));
        session_var_stream->write(";", 1);

        session_var_stream->write(iter_s->second->getName(),
                                  strlen(iter_s->second->getName()));
        session_var_stream->write("=", 1);
        session_var_stream->write(iter_s->second->getValue(),
                                  strlen(iter_s->second->getValue()));
        session_var_stream->write("\n", 1);
      }
    }
    if (session_var_stream && session_var_stream->is_open()) {
      session_var_stream->close();
    }
    if (session_var_stream)
      delete session_var_stream;
  }
}

void memory_cache::_reload_session_vars_() {
#if 0    
    map<session_var_key, session_var *>::iterator iter_ses;
	for(iter_ses = m_session_vars.begin(); iter_ses != m_session_vars.end(); iter_ses++)
	{
	    if(iter_ses->second)
    		delete iter_ses->second;
	}
	m_session_vars.clear();
#endif

  char szVarFolder[256];
  sprintf(szVarFolder, "%s/variable", m_dirpath.c_str());

  struct dirent* dirp;
  DIR* dp = opendir(szVarFolder);
  if (dp) {
    while ((dirp = readdir(dp)) != NULL) {
      /* variable file name should have postfix as ".session" */
      if (dirp->d_type == DT_REG) {
        int name_len = strlen(dirp->d_name);
        int exte_len = sizeof(".session") - 1;

        if (name_len > exte_len && dirp->d_name[name_len - 1] == 'n' &&
            dirp->d_name[name_len - 2] == 'o' &&
            dirp->d_name[name_len - 3] == 'i' &&
            dirp->d_name[name_len - 4] == 's' &&
            dirp->d_name[name_len - 5] == 's' &&
            dirp->d_name[name_len - 6] == 'e' &&
            dirp->d_name[name_len - 7] == 's' &&
            dirp->d_name[name_len - 8] == '.') {
          string strFileName = dirp->d_name;
          string strFilePath = m_dirpath.c_str();
          strFilePath += "/variable/";
          strFilePath += strFileName;

          unsigned long nOldVersion = 0;
          map<string, unsigned long>::iterator iter_ver =
              m_session_vars_file_versions.find(strFilePath);
          if (iter_ver != m_session_vars_file_versions.end())
            nOldVersion = iter_ver->second;

          ifstream* session_vars_stream =
              new ifstream(strFilePath.c_str(), ios_base::binary);
          ;
          string strline;
          if (session_vars_stream && !session_vars_stream->is_open()) {
            delete session_vars_stream;
            continue;
          }
          map<session_var_key, session_var*>::iterator iter;
          while (session_vars_stream &&
                 getline(*session_vars_stream, strline)) {
            if (strline == "") {
              continue;
            } else if (strline[0] == '#') {
              unsigned long nNewVersion;
              sscanf(strline.c_str(), "#%lu", &nNewVersion);

              if (nOldVersion == nNewVersion) /* Skip when version are same. */
              {
                break;
              } else {
                m_session_vars_file_versions[strFilePath.c_str()] = nNewVersion;
                continue;
              }
            }
            session_var* session_var_instance =
                new session_var(strline.c_str());
            iter = m_session_vars.find(
                session_var_key(session_var_instance->getUID(),
                                session_var_instance->getName()));
            if (iter != m_session_vars.end()) {
              if (iter->second->getCreateTime() <
                  session_var_instance->getCreateTime()) {
                if (iter->second)
                  delete iter->second; /* Remove the older one */
                iter->second = session_var_instance;
              } else
                delete session_var_instance;
            } else {
              m_session_vars.insert(
                  map<session_var_key, session_var*>::value_type(
                      session_var_key(session_var_instance->getUID(),
                                      session_var_instance->getName()),
                      session_var_instance));
            }
          }
          if (session_vars_stream && session_vars_stream->is_open()) {
            session_vars_stream->close();
          }
          if (session_vars_stream)
            delete session_vars_stream;
        }
      }
    }
    closedir(dp);
  }
}

void memory_cache::reload_session_vars() {
  pthread_rwlock_wrlock(&m_session_var_rwlock);
  _reload_session_vars_();
  pthread_rwlock_unlock(&m_session_var_rwlock);
}

void memory_cache::clear_session_vars() {
  pthread_rwlock_wrlock(&m_session_var_rwlock);
  map<session_var_key, session_var*>::iterator iter_ses;
  for (iter_ses = m_session_vars.begin(); iter_ses != m_session_vars.end();
       iter_ses++) {
    if (iter_ses->second)
      delete iter_ses->second;
  }
  m_session_vars.clear();
  pthread_rwlock_unlock(&m_session_var_rwlock);
}

// Server variable
void memory_cache::push_server_var(const char* name, const char* value) {
  pthread_rwlock_wrlock(&m_server_var_rwlock);
  server_var* pVar = new server_var(name, value);
  map<string, server_var*>::iterator iter = m_server_vars.find(name);
  if (iter != m_server_vars.end()) {
    iter->second = pVar;
  } else
    m_server_vars.insert(map<string, server_var*>::value_type(name, pVar));
  // Save server vars for other process for synchronization
  _save_server_vars_();
  pthread_rwlock_unlock(&m_server_var_rwlock);

#ifdef _WITH_MEMCACHED_
  if (m_memcached) {
    string keyname;
    keyname = name;
    char szMD5dst[41];
    MD5_CTX_OBJ context;
    context.MD5Update((unsigned char*)keyname.c_str(), keyname.length());
    unsigned char digest[16];
    context.MD5Final(digest);
    sprintf(szMD5dst,
            "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
            digest[0], digest[1], digest[2], digest[3], digest[4], digest[5],
            digest[6], digest[7], digest[8], digest[9], digest[10], digest[11],
            digest[12], digest[13], digest[14], digest[15]);
    szMD5dst[32] = '.';
    szMD5dst[33] = 's';
    szMD5dst[34] = 'e';
    szMD5dst[35] = 'r';
    szMD5dst[36] = 'v';
    szMD5dst[37] = 'v';
    szMD5dst[38] = 'a';
    szMD5dst[39] = 'r';
    szMD5dst[40] = '\0';
    if (m_memcached_map.find(pthread_self()) == m_memcached_map.end())
      m_memcached_map[pthread_self()] = memcached_clone(NULL, m_memcached);
    memcached_return memc_rc =
        memcached_set(m_memcached_map[pthread_self()], szMD5dst, 40, value,
                      strlen(value) + 1, (time_t)0, (uint32_t)0);
  }
#endif /* _WITH_MEMCACHED_ */
}

void memory_cache::save_server_vars() {
  pthread_rwlock_wrlock(&m_server_var_rwlock);
  _save_server_vars_();
  pthread_rwlock_unlock(&m_server_var_rwlock);
}

int memory_cache::get_server_var(const char* name, string& value) {
  int ret = -1;
  reload_server_vars();

  pthread_rwlock_rdlock(&m_server_var_rwlock);
  map<string, server_var*>::iterator iter = m_server_vars.find(name);
  if (iter != m_server_vars.end()) {
    value = iter->second->getValue();
    pthread_rwlock_unlock(&m_server_var_rwlock);
    ret = 0;
  } else {
    pthread_rwlock_unlock(&m_server_var_rwlock);
#ifdef _WITH_MEMCACHED_
    if (m_memcached) {
      string keyname;
      keyname = name;
      char szMD5dst[40];
      MD5_CTX_OBJ context;

      context.MD5Update((unsigned char*)keyname.c_str(), keyname.length());
      unsigned char digest[16];
      context.MD5Final(digest);
      sprintf(
          szMD5dst,
          "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
          digest[0], digest[1], digest[2], digest[3], digest[4], digest[5],
          digest[6], digest[7], digest[8], digest[9], digest[10], digest[11],
          digest[12], digest[13], digest[14], digest[15]);
      szMD5dst[32] = '.';
      szMD5dst[33] = 's';
      szMD5dst[34] = 'e';
      szMD5dst[35] = 'r';
      szMD5dst[36] = 'v';
      szMD5dst[37] = 'v';
      szMD5dst[38] = 'a';
      szMD5dst[39] = 'r';
      szMD5dst[40] = '\0';

      memcached_return memc_rc;
      size_t memc_value_length;
      uint32_t memc_flags = 0;
      if (m_memcached_map.find(pthread_self()) == m_memcached_map.end())
        m_memcached_map[pthread_self()] = memcached_clone(NULL, m_memcached);
      char* value_buf =
          memcached_get(m_memcached_map[pthread_self()], szMD5dst, 40,
                        &memc_value_length, &memc_flags, &memc_rc);

      if (memc_rc == MEMCACHED_SUCCESS && value_buf) {
        value = value_buf;
        free(value_buf);

        ret = 0;
      }
    }
#endif /* _WITH_MEMCACHED_ */
  }
  return ret;
}

void memory_cache::_save_server_vars_() {
  if (m_server_vars.size() > 0) {
    char szServerVarFile[256];
    char szServerVarFilePrefix[256];
    sprintf(szServerVarFilePrefix, "%s-%d@", m_service_name.c_str(),
            m_process_seq);

    sprintf(szServerVarFile, "%s/variable/%s%s.server", m_dirpath.c_str(),
            szServerVarFilePrefix, m_localhostname.c_str());

    unsigned long nVersion = 0;
    map<string, unsigned long>::iterator iter_ver =
        m_server_vars_file_versions.find(szServerVarFile);
    if (iter_ver != m_server_vars_file_versions.end())
      nVersion = iter_ver->second;
    ofstream* server_var_stream = NULL;

    map<string, server_var*>::iterator iter_s;
    for (iter_s = m_server_vars.begin(); iter_s != m_server_vars.end();
         iter_s++) {
      if (!server_var_stream) {
        server_var_stream =
            new ofstream(szServerVarFile, ios::out | ios::binary | ios::trunc);
        if (!server_var_stream || !server_var_stream->is_open())
          break;
        // Write the version in the first line
        nVersion++;
        m_server_vars_file_versions[szServerVarFile] = nVersion;

        char szVersion[256];
        sprintf(szVersion, "#%lu", nVersion);
        server_var_stream->write(szVersion, strlen(szVersion));
        server_var_stream->write("\n", 1);
      }
      char szTime[256];
      sprintf(szTime, "%d", iter_s->second->getCreateTime());
      server_var_stream->write(szTime, strlen(szTime));
      server_var_stream->write(";", 1);

      sprintf(szTime, "%d", iter_s->second->getAccessTime());
      server_var_stream->write(szTime, strlen(szTime));
      server_var_stream->write(";", 1);

      server_var_stream->write(iter_s->second->getName(),
                               strlen(iter_s->second->getName()));
      server_var_stream->write("=", 1);
      server_var_stream->write(iter_s->second->getValue(),
                               strlen(iter_s->second->getValue()));
      server_var_stream->write("\n", 1);
    }
    if (server_var_stream && server_var_stream->is_open()) {
      server_var_stream->close();
    }
    if (server_var_stream)
      delete server_var_stream;
  }
}

void memory_cache::_reload_users_() {
  TiXmlDocument xmlFileterDoc;
  xmlFileterDoc.LoadFile(CHttpBase::m_users_list_file.c_str());
  TiXmlElement* pRootElement = xmlFileterDoc.RootElement();
  if (pRootElement) {
    m_users_list.clear();

    TiXmlNode* pChildNode = pRootElement->FirstChild("user");
    while (pChildNode) {
      if (pChildNode && pChildNode->ToElement()) {
        if (pChildNode->ToElement()->Attribute("id") &&
            strcmp(pChildNode->ToElement()->Attribute("id"), "") != 0) {
          string strPwd;
          if (!pChildNode->ToElement()->Attribute("password") ||
              strcmp(pChildNode->ToElement()->Attribute("password"), "") == 0) {
            strPwd = "";
          } else {
            Security::Decrypt(
                pChildNode->ToElement()->Attribute("password"),
                strlen(pChildNode->ToElement()->Attribute("password")), strPwd);
          }

          m_users_list[pChildNode->ToElement()->Attribute("id")] = strPwd;
        }
      }
      pChildNode = pChildNode->NextSibling("user");
    }
  }
}

bool memory_cache::_find_user_(const char* id, string& pwd) {
  bool ret = false;
  map<string, string>::iterator iter = m_users_list.find(id);
  if (iter != m_users_list.end()) {
    pwd = iter->second;
    ret = true;
  }

  return ret;
}

bool memory_cache::find_user(const char* id, string& pwd) {
  pthread_rwlock_rdlock(&m_users_rwlock);
  bool ret = _find_user_(id, pwd);
  pthread_rwlock_unlock(&m_users_rwlock);

  return ret;
}

void memory_cache::_reload_server_vars_() {
#if 0    
    map<string, server_var *>::iterator iter_srv;
	for(iter_srv = m_server_vars.begin(); iter_srv != m_server_vars.end(); iter_srv++)
	{
	    if(iter_srv->second)
    		delete iter_srv->second;
	}
	m_server_vars.clear();
#endif
  char szVarFolder[256];
  sprintf(szVarFolder, "%s/variable", m_dirpath.c_str());

  struct dirent* dirp;
  DIR* dp = opendir(szVarFolder);
  if (dp) {
    while ((dirp = readdir(dp)) != NULL) {
      /* variable file name should have postfix as ".server" */
      if (dirp->d_type == DT_REG) {
        int name_len = strlen(dirp->d_name);
        int exte_len = sizeof(".server") - 1;

        if (name_len > exte_len && dirp->d_name[name_len - 1] == 'r' &&
            dirp->d_name[name_len - 2] == 'e' &&
            dirp->d_name[name_len - 3] == 'v' &&
            dirp->d_name[name_len - 4] == 'r' &&
            dirp->d_name[name_len - 5] == 'e' &&
            dirp->d_name[name_len - 6] == 's' &&
            dirp->d_name[name_len - 7] == '.') {
          string strFileName = dirp->d_name;
          string strFilePath = m_dirpath.c_str();
          strFilePath += "/variable/";
          strFilePath += strFileName;

          unsigned long nOldVersion = 0;
          map<string, unsigned long>::iterator iter_ver =
              m_server_vars_file_versions.find(strFilePath);
          if (iter_ver != m_server_vars_file_versions.end())
            nOldVersion = iter_ver->second;

          ifstream* server_vars_stream =
              new ifstream(strFilePath.c_str(), ios_base::binary);
          ;
          string strline;
          while (server_vars_stream && server_vars_stream->is_open() &&
                 getline(*server_vars_stream, strline)) {
            strtrim(strline);
            if (strline == "") {
              continue;
            } else if (strline[0] == '#') {
              unsigned long nNewVersion;
              sscanf(strline.c_str(), "#%lu", &nNewVersion);

              if (nOldVersion == nNewVersion) /* Skip when version are same. */
              {
                break;
              } else {
                m_server_vars_file_versions[strFilePath.c_str()] = nNewVersion;
                continue;
              }
            }
            server_var* server_var_instance = new server_var(strline.c_str());
            map<string, server_var*>::iterator iter =
                m_server_vars.find(server_var_instance->getName());
            if (iter != m_server_vars.end()) {
              if (iter->second->getCreateTime() <
                  server_var_instance->getCreateTime()) {
                if (iter->second)
                  delete iter->second; /* Remove the older one */
                iter->second = server_var_instance;
              } else
                delete server_var_instance;
            } else {
              m_server_vars.insert(map<string, server_var*>::value_type(
                  server_var_instance->getName(), server_var_instance));
            }
          }
          if (server_vars_stream && server_vars_stream->is_open()) {
            server_vars_stream->close();
          }
          if (server_vars_stream)
            delete server_vars_stream;
        }
      }
    }
    closedir(dp);
  }
}

void memory_cache::reload_server_vars() {
  pthread_rwlock_wrlock(&m_server_var_rwlock);
  _reload_server_vars_();
  pthread_rwlock_unlock(&m_server_var_rwlock);
}

void memory_cache::reload_users() {
  pthread_rwlock_wrlock(&m_users_rwlock);
  _reload_users_();
  pthread_rwlock_unlock(&m_users_rwlock);
}

void memory_cache::clear_server_vars() {
  pthread_rwlock_wrlock(&m_server_var_rwlock);
  map<string, server_var*>::iterator iter_srv;
  for (iter_srv = m_server_vars.begin(); iter_srv != m_server_vars.end();
       iter_srv++) {
    if (iter_srv->second)
      delete iter_srv->second;
  }
  m_server_vars.clear();
  pthread_rwlock_unlock(&m_server_var_rwlock);
}

// File
map<string, file_cache*>::iterator memory_cache::_find_oldest_file_() {
  map<string, file_cache*>::iterator o_it = m_file_cache.end();
  map<string, file_cache*>::iterator c_it = m_file_cache.begin();
  for (c_it = m_file_cache.begin(); c_it != m_file_cache.end(); ++c_it) {
    if (o_it == m_file_cache.end()) {
      o_it = c_it;
    } else {
      if (o_it != c_it)  // avoid re-entrying the lock
      {
        FILE_CACHE_DATA *curr_data, *oldest_data;
        curr_data = c_it->second->file_rdlock();
        oldest_data = o_it->second->file_rdlock();

        if (curr_data->t_access < oldest_data->t_access)
          o_it = c_it;
        else if (curr_data->t_access =
                     oldest_data->t_access && curr_data->len < oldest_data->len)
          o_it = c_it;

        c_it->second->file_unlock();
        o_it->second->file_unlock();
      }
    }
  }
  return o_it;
}

bool memory_cache::_find_file_(const char* name, file_cache** f_out) {
  bool ret = false;
  *f_out = NULL;
  map<string, file_cache*>::iterator iter = m_file_cache.find(name);
  if (iter != m_file_cache.end()) {
    if (!iter->second->file_fresh()) {
      ret = false;
    } else {
      *f_out = iter->second;
      ret = true;
    }
  }

  return ret;
}

bool memory_cache::_push_file_(const char* name,
                               char* buf,
                               unsigned int len,
                               time_t t_modify,
                               unsigned char* etag,
                               file_cache** f_out) {
  bool ret = false;

  map<string, file_cache*>::iterator iter = m_file_cache.find(name);
  if (iter != m_file_cache.end()) {
    if (iter->second) {
      FILE_CACHE_DATA* cache_data = iter->second->file_rdlock();
      m_file_cache_size -= cache_data->len;
      iter->second->file_unlock();
      delete iter->second;
    }
    iter->second = new file_cache(buf, len, t_modify, etag);
    m_file_cache_size += len;

    *f_out = iter->second;
    ret = true;
  } else {
    file_cache* fc = new file_cache(buf, len, t_modify, etag);

    if (m_file_cache_size < CHttpBase::m_total_localfile_cache_size) {
      map<string, file_cache*>::iterator iter = m_file_cache.find(name);
      if (iter != m_file_cache.end()) {
        FILE_CACHE_DATA* cache_data = iter->second->file_rdlock();
        m_file_cache_size -= cache_data->len;
        iter->second->file_unlock();

        delete iter->second;
        m_file_cache.erase(iter);
      }
    } else {
      map<string, file_cache*>::iterator oldest_file = _find_oldest_file_();
      FILE_CACHE_DATA* oldest_data = oldest_file->second->file_rdlock();
      m_file_cache_size -= oldest_data->len;
      oldest_file->second->file_unlock();
      delete oldest_file->second;
      m_file_cache.erase(oldest_file);
    }

    pair<map<string, file_cache*>::iterator, bool> inspos;

    inspos =
        m_file_cache.insert(map<string, file_cache*>::value_type(name, fc));
    if (inspos.second == true) {
      *f_out = inspos.first->second;
      m_file_cache_size += len;
      ret = true;
    }
  }

  return ret;
}

file_cache* memory_cache::lock_file(const char* name,
                                    FILE_CACHE_DATA** cache_data) {
  file_cache* ret = NULL;
  pthread_rwlock_rdlock(&m_file_rwlock);

  map<string, file_cache*>::iterator iter = m_file_cache.find(name);
  if (iter != m_file_cache.end()) {
    ret = iter->second;
    *cache_data = iter->second->file_rdlock();
  }

  pthread_rwlock_unlock(&m_file_rwlock);
  return ret;
}

void memory_cache::unlock_file(file_cache* fc) {
  if (fc)
    fc->file_unlock();
}

void memory_cache::clear_files() {
  pthread_rwlock_wrlock(&m_file_rwlock);
  map<string, file_cache*>::iterator iter_file;
  for (iter_file = m_file_cache.begin(); iter_file != m_file_cache.end();
       iter_file++) {
    if (iter_file->second)
      delete iter_file->second;
  }
  m_file_cache.clear();
  pthread_rwlock_unlock(&m_file_rwlock);
}

// Tunneling
map<string, tunneling_cache*>::iterator
memory_cache::_find_oldest_tunneling_() {
  map<string, tunneling_cache*>::iterator o_it = m_tunneling_cache.end();
  map<string, tunneling_cache*>::iterator c_it = m_tunneling_cache.begin();
  for (c_it = m_tunneling_cache.begin(); c_it != m_tunneling_cache.end();
       ++c_it) {
    if (o_it == m_tunneling_cache.end()) {
      o_it = c_it;
    } else {
      if (o_it != c_it)  // avoid re-entrying the lock
      {
        TUNNELING_CACHE_DATA *curr_data, *oldest_data;
        curr_data = c_it->second->tunneling_rdlock();
        oldest_data = o_it->second->tunneling_rdlock();

        if (curr_data->t_access < oldest_data->t_access)
          o_it = c_it;
        else if (curr_data->t_access =
                     oldest_data->t_access && curr_data->len < oldest_data->len)
          o_it = c_it;

        c_it->second->tunneling_unlock();
        o_it->second->tunneling_unlock();
      }
    }
  }
  return o_it;
}

bool memory_cache::_find_tunneling_(const char* name, tunneling_cache** t_out) {
  bool ret = false;
  *t_out = NULL;
  map<string, tunneling_cache*>::iterator iter = m_tunneling_cache.find(name);
  if (iter != m_tunneling_cache.end()) {
    if (!iter->second->tunneling_fresh()) {
      ret = false;
    } else {
      *t_out = iter->second;

      ret = true;
    }
  }

  return ret;
}

bool memory_cache::_push_tunneling_(const char* name,
                                    char* buf,
                                    unsigned int len,
                                    const char* type,
                                    const char* cache,
                                    const char* allow,
                                    const char* encoding,
                                    const char* language,
                                    const char* last_modify,
                                    const char* etag,
                                    const char* expires,
                                    const char* server,
                                    const char* via,
                                    unsigned int max_age,
                                    tunneling_cache** t_out) {
  bool ret = false;

  map<string, tunneling_cache*>::iterator iter = m_tunneling_cache.find(name);
  if (iter != m_tunneling_cache.end()) {
    if (iter->second) {
      TUNNELING_CACHE_DATA* cache_data = iter->second->tunneling_rdlock();
      m_tunneling_cache_size -= cache_data->len;
      iter->second->tunneling_unlock();
      delete iter->second;
    }
    iter->second =
        new tunneling_cache(buf, len, type, cache, allow, encoding, language,
                            etag, last_modify, expires, server, via, max_age);
    m_tunneling_cache_size += len;

    *t_out = iter->second;
    ret = true;
  } else {
    tunneling_cache* t_cache =
        new tunneling_cache(buf, len, type, cache, allow, encoding, language,
                            etag, last_modify, expires, server, via, max_age);

    if (m_tunneling_cache_size < CHttpBase::m_total_tunneling_cache_size) {
      map<string, tunneling_cache*>::iterator iter =
          m_tunneling_cache.find(name);
      if (iter != m_tunneling_cache.end()) {
        if (iter->second) {
          TUNNELING_CACHE_DATA* cache_data = iter->second->tunneling_rdlock();
          m_tunneling_cache_size -= cache_data->len;
          iter->second->tunneling_unlock();

          delete iter->second;
          m_tunneling_cache.erase(iter);
        }
      }
    } else {
      map<string, tunneling_cache*>::iterator oldest_file =
          _find_oldest_tunneling_();
      TUNNELING_CACHE_DATA* oldest_data =
          oldest_file->second->tunneling_rdlock();
      m_tunneling_cache_size -= oldest_data->len;
      oldest_file->second->tunneling_unlock();
      delete oldest_file->second;
      m_tunneling_cache.erase(oldest_file);
    }

    pair<map<string, tunneling_cache*>::iterator, bool> ins_pos;

    ins_pos = m_tunneling_cache.insert(
        map<string, tunneling_cache*>::value_type(name, t_cache));
    if (ins_pos.second == true) {
      *t_out = ins_pos.first->second;
      m_tunneling_cache_size += len;
      ret = true;
    }
  }

  return ret;
}

void memory_cache::unlock_tunneling_cache(tunneling_cache* fc) {
  if (fc)
    fc->tunneling_unlock();
}

void memory_cache::clear_tunnelings() {
  pthread_rwlock_wrlock(&m_tunneling_rwlock);
  map<string, tunneling_cache*>::iterator iter_file;
  for (iter_file = m_tunneling_cache.begin();
       iter_file != m_tunneling_cache.end(); iter_file++) {
    if (iter_file->second)
      delete iter_file->second;
  }
  m_tunneling_cache.clear();
  pthread_rwlock_unlock(&m_tunneling_rwlock);
}

///////////////////////////////////////////////////////////////////////////////
void memory_cache::_reload_cookies_() {
#if 0    
   map<string, Cookie*>::iterator iter_c;
	for(iter_c = m_cookies.begin(); iter_c != m_cookies.end(); iter_c++)
	{
	    if(iter_c->second)
    		delete iter_c->second;
	}
	m_cookies.clear();
#endif
  char szCookieFolder[256];
  sprintf(szCookieFolder, "%s/cookie", m_dirpath.c_str());

  struct dirent* dirp;
  DIR* dp = opendir(szCookieFolder);
  if (dp) {
    while ((dirp = readdir(dp)) != NULL) {
      /* cookie file name should have postfix as ".cookie" */
      if (dirp->d_type == DT_REG) {
        int name_len = strlen(dirp->d_name);
        int exte_len = sizeof(".cookie") - 1;

        if (name_len > exte_len && dirp->d_name[name_len - 1] == 'e' &&
            dirp->d_name[name_len - 2] == 'i' &&
            dirp->d_name[name_len - 3] == 'k' &&
            dirp->d_name[name_len - 4] == 'o' &&
            dirp->d_name[name_len - 5] == 'o' &&
            dirp->d_name[name_len - 6] == 'c' &&
            dirp->d_name[name_len - 7] == '.') {
          string strFileName = dirp->d_name;
          string strFilePath = m_dirpath.c_str();
          strFilePath += "/cookie/";
          strFilePath += strFileName;
          ifstream* cookie_stream =
              new ifstream(strFilePath.c_str(), ios_base::binary);
          ;
          string strline;
          if (cookie_stream && !cookie_stream->is_open()) {
            delete cookie_stream;
            continue;
          }
          while (cookie_stream && getline(*cookie_stream, strline)) {
            strtrim(strline);
            if (strline == "")
              continue;
            Cookie* cookie_instance = new Cookie(strline.c_str());
            map<string, Cookie*>::iterator iter =
                m_cookies.find(cookie_instance->getName());
            if (iter != m_cookies.end() &&
                iter->second->getCreateTime() <
                    cookie_instance->getCreateTime()) {
              if (iter->second)
                delete iter->second;
              iter->second = cookie_instance;
            } else {
              m_cookies.insert(map<string, Cookie*>::value_type(
                  cookie_instance->getName(), cookie_instance));
            }
          }
        }
      }
    }
    closedir(dp);
  }
}

void memory_cache::reload_cookies() {
  pthread_rwlock_wrlock(&m_cookie_rwlock);
  _reload_cookies_();
  pthread_rwlock_unlock(&m_cookie_rwlock);
}

void memory_cache::_save_cookies_() {
  if (m_cookies.size() > 0) {
    char szCookieFile[256];
    char szCookieFilePrefix[256];
    sprintf(szCookieFilePrefix, "%s-%d@", m_service_name.c_str(),
            m_process_seq);

    sprintf(szCookieFile, "%s/cookie/%s%s.cookie", m_dirpath.c_str(),
            szCookieFilePrefix, m_localhostname.c_str());
    ofstream* cookie_stream = NULL;

    map<string, Cookie*>::iterator iter_c;
    bool bSave = true;
    for (iter_c = m_cookies.begin(); iter_c != m_cookies.end(); iter_c++) {
      bSave = true;
      const char* szExpires = iter_c->second->getExpires();

      if (szExpires[0] != '\0' &&
          time(NULL) > ParseGMTorUTCTimeString(szExpires)) {
        bSave = false;
      }
      if (iter_c->second->getMaxAge() == -1 &&
          (time(NULL) - iter_c->second->getAccessTime() >
           90)) /* No-inactive http request, 90s for timeout*/
      {
        bSave = false;
      } else if (iter_c->second->getMaxAge() == 0) {
        bSave = false;
      } else if (iter_c->second->getMaxAge() > 0 &&
                 (time(NULL) - iter_c->second->getCreateTime() >
                  iter_c->second->getMaxAge())) {
        bSave = false;
      }
      if (bSave) {
        if (!cookie_stream) {
          cookie_stream =
              new ofstream(szCookieFile, ios::out | ios::binary | ios::trunc);
        }
        if (!cookie_stream->is_open())
          break;
        string strCookie;
        iter_c->second->toString(strCookie);

        char szTime[256];
        sprintf(szTime, "%d", iter_c->second->getCreateTime());
        cookie_stream->write(szTime, strlen(szTime));
        cookie_stream->write(";", 1);

        sprintf(szTime, "%d", iter_c->second->getAccessTime());
        cookie_stream->write(szTime, strlen(szTime));
        cookie_stream->write(";", 1);

        cookie_stream->write(strCookie.c_str(), strCookie.length());
        cookie_stream->write("\n", 1);
      }
    }
    if (cookie_stream && cookie_stream->is_open()) {
      cookie_stream->close();
    }
    if (cookie_stream)
      delete cookie_stream;
  }
}

void memory_cache::save_cookies() {
  pthread_rwlock_wrlock(&m_cookie_rwlock);
  _save_cookies_();
  pthread_rwlock_unlock(&m_cookie_rwlock);
}

void memory_cache::clear_cookies() {
  pthread_rwlock_wrlock(&m_cookie_rwlock);
  map<string, Cookie*>::iterator iter_c;
  for (iter_c = m_cookies.begin(); iter_c != m_cookies.end(); iter_c++) {
    if (iter_c->second)
      delete iter_c->second;
  }
  m_cookies.clear();
  pthread_rwlock_unlock(&m_cookie_rwlock);
}

void memory_cache::access_cookie(const char* name) {
  pthread_rwlock_wrlock(&m_cookie_rwlock);
  map<string, Cookie*>::iterator iter = m_cookies.find(name);
  if (iter != m_cookies.end())
    iter->second->setAccessTime(time(NULL));
  pthread_rwlock_unlock(&m_cookie_rwlock);
}

void memory_cache::unload() {
  save_session_vars();
  save_server_vars();

  clear_session_vars();
  clear_server_vars();

  clear_files();

  clear_tunnelings();
}

void memory_cache::load() {
  unload();
  reload_users();

  reload_server_vars();
  reload_session_vars();

  // create the multitype list
  m_type_table.insert(map<string, string>::value_type("323", "text/h323"));
  m_type_table.insert(map<string, string>::value_type(
      "acx", "application/internet-property-stream"));
  m_type_table.insert(
      map<string, string>::value_type("ai", "application/postscript"));
  m_type_table.insert(map<string, string>::value_type("aif", "audio/x-aiff"));
  m_type_table.insert(map<string, string>::value_type("aifc", "audio/x-aiff"));
  m_type_table.insert(map<string, string>::value_type("aiff", "audio/x-aiff"));
  m_type_table.insert(map<string, string>::value_type("asf", "video/x-ms-asf"));
  m_type_table.insert(map<string, string>::value_type("asr", "video/x-ms-asf"));
  m_type_table.insert(map<string, string>::value_type("asx", "video/x-ms-asf"));
  m_type_table.insert(map<string, string>::value_type("au", "audio/basic"));
  m_type_table.insert(
      map<string, string>::value_type("avi", "video/x-msvideo"));
  m_type_table.insert(
      map<string, string>::value_type("axs", "application/olescript"));
  m_type_table.insert(map<string, string>::value_type("bas", "text/plain"));
  m_type_table.insert(
      map<string, string>::value_type("bcpio", "application/x-bcpio"));
  m_type_table.insert(
      map<string, string>::value_type("bin", "application/octet-stream"));
  m_type_table.insert(map<string, string>::value_type("bmp", "image/bmp"));
  m_type_table.insert(map<string, string>::value_type("c", "text/plain"));
  m_type_table.insert(
      map<string, string>::value_type("cat", "application/vnd.ms-pkiseccat"));
  m_type_table.insert(
      map<string, string>::value_type("cdf", "application/x-cdf"));
  m_type_table.insert(
      map<string, string>::value_type("cer", "application/x-x509-ca-cert"));
  m_type_table.insert(
      map<string, string>::value_type("class", "application/octet-stream"));
  m_type_table.insert(
      map<string, string>::value_type("clp", "application/x-msclip"));
  m_type_table.insert(map<string, string>::value_type("cmx", "image/x-cmx"));
  m_type_table.insert(map<string, string>::value_type("cod", "image/cis-cod"));
  m_type_table.insert(
      map<string, string>::value_type("cpio", "application/x-cpio"));
  m_type_table.insert(
      map<string, string>::value_type("crd", "application/x-mscardfile"));
  m_type_table.insert(
      map<string, string>::value_type("crl", "application/pkix-crl"));
  m_type_table.insert(
      map<string, string>::value_type("crt", "application/x-x509-ca-cert"));
  m_type_table.insert(
      map<string, string>::value_type("csh", "application/x-csh"));
  m_type_table.insert(map<string, string>::value_type("css", "text/css"));
  m_type_table.insert(
      map<string, string>::value_type("dcr", "application/x-director"));
  m_type_table.insert(
      map<string, string>::value_type("der", "application/x-x509-ca-cert"));
  m_type_table.insert(
      map<string, string>::value_type("dir", "application/x-director"));
  m_type_table.insert(
      map<string, string>::value_type("dll", "application/x-msdownload"));
  m_type_table.insert(
      map<string, string>::value_type("dms", "application/octet-stream"));
  m_type_table.insert(
      map<string, string>::value_type("doc", "application/msword"));
  m_type_table.insert(
      map<string, string>::value_type("dot", "application/msword"));
  m_type_table.insert(
      map<string, string>::value_type("dvi", "application/x-dvi"));
  m_type_table.insert(
      map<string, string>::value_type("dxr", "application/x-director"));
  m_type_table.insert(
      map<string, string>::value_type("eps", "application/postscript"));
  m_type_table.insert(map<string, string>::value_type("etx", "text/x-setext"));
  m_type_table.insert(
      map<string, string>::value_type("evy", "application/envoy"));
  m_type_table.insert(
      map<string, string>::value_type("exe", "application/octet-stream"));
  m_type_table.insert(
      map<string, string>::value_type("fif", "application/fractals"));
  m_type_table.insert(map<string, string>::value_type("flr", "x-world/x-vrml"));
  m_type_table.insert(map<string, string>::value_type("gif", "image/gif"));
  m_type_table.insert(
      map<string, string>::value_type("gtar", "application/x-gtar"));
  m_type_table.insert(
      map<string, string>::value_type("gz", "application/x-gzip"));
  m_type_table.insert(map<string, string>::value_type("h", "text/plain"));
  m_type_table.insert(
      map<string, string>::value_type("hdf", "application/x-hdf"));
  m_type_table.insert(
      map<string, string>::value_type("hlp", "application/winhlp"));
  m_type_table.insert(
      map<string, string>::value_type("hqx", "application/mac-binhex40"));
  m_type_table.insert(
      map<string, string>::value_type("hta", "application/hta"));
  m_type_table.insert(
      map<string, string>::value_type("htc", "text/x-component"));
  m_type_table.insert(map<string, string>::value_type("htm", "text/html"));
  m_type_table.insert(map<string, string>::value_type("html", "text/html"));
  m_type_table.insert(
      map<string, string>::value_type("htt", "text/webviewhtml"));
  m_type_table.insert(map<string, string>::value_type("ico", "image/x-icon"));
  m_type_table.insert(map<string, string>::value_type("ief", "image/ief"));
  m_type_table.insert(
      map<string, string>::value_type("iii", "application/x-iphone"));
  m_type_table.insert(
      map<string, string>::value_type("ins", "application/x-internet-signup"));
  m_type_table.insert(
      map<string, string>::value_type("isp", "application/x-internet-signup"));
  m_type_table.insert(map<string, string>::value_type("jfif", "image/pipeg"));
  m_type_table.insert(map<string, string>::value_type("jpe", "image/jpeg"));
  m_type_table.insert(map<string, string>::value_type("jpeg", "image/jpeg"));
  m_type_table.insert(map<string, string>::value_type("jpg", "image/jpeg"));
  m_type_table.insert(
      map<string, string>::value_type("js", "application/x-javascript"));
  m_type_table.insert(
      map<string, string>::value_type("latex", "application/x-latex"));
  m_type_table.insert(
      map<string, string>::value_type("lha", "application/octet-stream"));
  m_type_table.insert(map<string, string>::value_type("lsf", "video/x-la-asf"));
  m_type_table.insert(map<string, string>::value_type("lsx", "video/x-la-asf"));
  m_type_table.insert(
      map<string, string>::value_type("lzh", "application/octet-stream"));
  m_type_table.insert(
      map<string, string>::value_type("m13", "application/x-msmediaview"));
  m_type_table.insert(
      map<string, string>::value_type("m14", "application/x-msmediaview"));
  m_type_table.insert(
      map<string, string>::value_type("m3u", "audio/x-mpegurl"));
  m_type_table.insert(
      map<string, string>::value_type("man", "application/x-troff-man"));
  m_type_table.insert(
      map<string, string>::value_type("mdb", "application/x-msaccess"));
  m_type_table.insert(
      map<string, string>::value_type("me", "application/x-troff-me"));
  m_type_table.insert(map<string, string>::value_type("mht", "message/rfc822"));
  m_type_table.insert(
      map<string, string>::value_type("mhtml", "message/rfc822"));
  m_type_table.insert(map<string, string>::value_type("mid", "audio/mid"));
  m_type_table.insert(
      map<string, string>::value_type("mny", "application/x-msmoney"));
  m_type_table.insert(
      map<string, string>::value_type("mov", "video/quicktime"));
  m_type_table.insert(
      map<string, string>::value_type("movie", "video/x-sgi-movie"));
  m_type_table.insert(map<string, string>::value_type("mp2", "video/mpeg"));
  m_type_table.insert(map<string, string>::value_type("mp3", "audio/mpeg"));
  m_type_table.insert(map<string, string>::value_type("mpa", "video/mpeg"));
  m_type_table.insert(map<string, string>::value_type("mpe", "video/mpeg"));
  m_type_table.insert(map<string, string>::value_type("mpeg", "video/mpeg"));
  m_type_table.insert(map<string, string>::value_type("mpg", "video/mpeg"));
  m_type_table.insert(
      map<string, string>::value_type("mpp", "application/vnd.ms-project"));
  m_type_table.insert(map<string, string>::value_type("mpv2", "video/mpeg"));
  m_type_table.insert(
      map<string, string>::value_type("ms", "application/x-troff-ms"));
  m_type_table.insert(
      map<string, string>::value_type("mvb", "application/x-msmediaview"));
  m_type_table.insert(map<string, string>::value_type("nws", "message/rfc822"));
  m_type_table.insert(
      map<string, string>::value_type("oda", "application/oda"));
  m_type_table.insert(
      map<string, string>::value_type("p10", "application/pkcs10"));
  m_type_table.insert(
      map<string, string>::value_type("p12", "application/x-pkcs12"));
  m_type_table.insert(map<string, string>::value_type(
      "p7b", "application/x-pkcs7-certificates"));
  m_type_table.insert(
      map<string, string>::value_type("p7c", "application/x-pkcs7-mime"));
  m_type_table.insert(
      map<string, string>::value_type("p7m", "application/x-pkcs7-mime"));
  m_type_table.insert(map<string, string>::value_type(
      "p7r", "application/x-pkcs7-certreqresp"));
  m_type_table.insert(
      map<string, string>::value_type("p7s", "application/x-pkcs7-signature"));
  m_type_table.insert(
      map<string, string>::value_type("pbm", "image/x-portable-bitmap"));
  m_type_table.insert(
      map<string, string>::value_type("pdf", "application/pdf"));
  m_type_table.insert(
      map<string, string>::value_type("pfx", "application/x-pkcs12"));
  m_type_table.insert(
      map<string, string>::value_type("pgm", "image/x-portable-graymap"));
  m_type_table.insert(
      map<string, string>::value_type("pko", "application/ynd.ms-pkipko"));
  m_type_table.insert(
      map<string, string>::value_type("pma", "application/x-perfmon"));
  m_type_table.insert(
      map<string, string>::value_type("pmc", "application/x-perfmon"));
  m_type_table.insert(
      map<string, string>::value_type("pml", "application/x-perfmon"));
  m_type_table.insert(
      map<string, string>::value_type("pmr", "application/x-perfmon"));
  m_type_table.insert(
      map<string, string>::value_type("pmw", "application/x-perfmon"));
  m_type_table.insert(
      map<string, string>::value_type("pnm", "image/x-portable-anymap"));
  m_type_table.insert(map<string, string>::value_type("png", "image/x-png"));
  m_type_table.insert(
      map<string, string>::value_type("pot,", "application/vnd.ms-powerpoint"));
  m_type_table.insert(
      map<string, string>::value_type("ppm", "image/x-portable-pixmap"));
  m_type_table.insert(
      map<string, string>::value_type("pps", "application/vnd.ms-powerpoint"));
  m_type_table.insert(
      map<string, string>::value_type("ppt", "application/vnd.ms-powerpoint"));
  m_type_table.insert(
      map<string, string>::value_type("prf", "application/pics-rules"));
  m_type_table.insert(
      map<string, string>::value_type("ps", "application/postscript"));
  m_type_table.insert(
      map<string, string>::value_type("pub", "application/x-mspublisher"));
  m_type_table.insert(map<string, string>::value_type("qt", "video/quicktime"));
  m_type_table.insert(
      map<string, string>::value_type("ra", "audio/x-pn-realaudio"));
  m_type_table.insert(
      map<string, string>::value_type("ram", "audio/x-pn-realaudio"));
  m_type_table.insert(
      map<string, string>::value_type("ras", "image/x-cmu-raster"));
  m_type_table.insert(map<string, string>::value_type("rgb", "image/x-rgb"));
  m_type_table.insert(map<string, string>::value_type("rmi", "audio/mid"));
  m_type_table.insert(
      map<string, string>::value_type("roff", "application/x-troff"));
  m_type_table.insert(
      map<string, string>::value_type("rtf", "application/rtf"));
  m_type_table.insert(map<string, string>::value_type("rtx", "text/richtext"));
  m_type_table.insert(
      map<string, string>::value_type("scd", "application/x-msschedule"));
  m_type_table.insert(map<string, string>::value_type("sct", "text/scriptlet"));
  m_type_table.insert(map<string, string>::value_type(
      "setpay", "application/set-payment-initiation"));
  m_type_table.insert(map<string, string>::value_type(
      "setreg", "application/set-registration-initiation"));
  m_type_table.insert(
      map<string, string>::value_type("sh", "application/x-sh"));
  m_type_table.insert(
      map<string, string>::value_type("shar", "application/x-shar"));
  m_type_table.insert(
      map<string, string>::value_type("sit", "application/x-stuffit"));
  m_type_table.insert(map<string, string>::value_type("snd", "audio/basic"));
  m_type_table.insert(map<string, string>::value_type(
      "spc", "application/x-pkcs7-certificates"));
  m_type_table.insert(
      map<string, string>::value_type("spl", "application/futuresplash"));
  m_type_table.insert(
      map<string, string>::value_type("src", "application/x-wais-source"));
  m_type_table.insert(map<string, string>::value_type(
      "sst", "application/vnd.ms-pkicertstore"));
  m_type_table.insert(
      map<string, string>::value_type("stl", "application/vnd.ms-pkistl"));
  m_type_table.insert(map<string, string>::value_type("stm", "text/html"));
  m_type_table.insert(map<string, string>::value_type("svg", "image/svg+xml"));
  m_type_table.insert(
      map<string, string>::value_type("sv4cpio", "application/x-sv4cpio"));
  m_type_table.insert(
      map<string, string>::value_type("sv4crc", "application/x-sv4crc"));
  m_type_table.insert(
      map<string, string>::value_type("swf", "application/x-shockwave-flash"));
  m_type_table.insert(
      map<string, string>::value_type("t", "application/x-troff"));
  m_type_table.insert(
      map<string, string>::value_type("tar", "application/x-tar"));
  m_type_table.insert(
      map<string, string>::value_type("tcl", "application/x-tcl"));
  m_type_table.insert(
      map<string, string>::value_type("tex", "application/x-tex"));
  m_type_table.insert(
      map<string, string>::value_type("texi", "application/x-texinfo"));
  m_type_table.insert(
      map<string, string>::value_type("texinfo", "application/x-texinfo"));
  m_type_table.insert(
      map<string, string>::value_type("tgz", "application/x-compressed"));
  m_type_table.insert(map<string, string>::value_type("tif", "image/tiff"));
  m_type_table.insert(map<string, string>::value_type("tiff", "image/tiff"));
  m_type_table.insert(
      map<string, string>::value_type("tr", "application/x-troff"));
  m_type_table.insert(
      map<string, string>::value_type("trm", "application/x-msterminal"));
  m_type_table.insert(
      map<string, string>::value_type("tsv", "text/tab-separated-values"));
  m_type_table.insert(map<string, string>::value_type("txt", "text/plain"));
  m_type_table.insert(map<string, string>::value_type("uls", "text/iuls"));
  m_type_table.insert(
      map<string, string>::value_type("ustar", "application/x-ustar"));
  m_type_table.insert(map<string, string>::value_type("vcf", "text/x-vcard"));
  m_type_table.insert(
      map<string, string>::value_type("vrml", "x-world/x-vrml"));
  m_type_table.insert(map<string, string>::value_type("wav", "audio/x-wav"));
  m_type_table.insert(
      map<string, string>::value_type("wcm", "application/vnd.ms-works"));
  m_type_table.insert(
      map<string, string>::value_type("wdb", "application/vnd.ms-works"));
  m_type_table.insert(
      map<string, string>::value_type("wks", "application/vnd.ms-works"));
  m_type_table.insert(
      map<string, string>::value_type("wmf", "application/x-msmetafile"));
  m_type_table.insert(
      map<string, string>::value_type("wps", "application/vnd.ms-works"));
  m_type_table.insert(
      map<string, string>::value_type("wri", "application/x-mswrite"));
  m_type_table.insert(map<string, string>::value_type("wrl", "x-world/x-vrml"));
  m_type_table.insert(map<string, string>::value_type("wrz", "x-world/x-vrml"));
  m_type_table.insert(map<string, string>::value_type("xaf", "x-world/x-vrml"));
  m_type_table.insert(
      map<string, string>::value_type("xbm", "image/x-xbitmap"));
  m_type_table.insert(
      map<string, string>::value_type("xla", "application/vnd.ms-excel"));
  m_type_table.insert(
      map<string, string>::value_type("xlc", "application/vnd.ms-excel"));
  m_type_table.insert(
      map<string, string>::value_type("xlm", "application/vnd.ms-excel"));
  m_type_table.insert(
      map<string, string>::value_type("xls", "application/vnd.ms-excel"));
  m_type_table.insert(
      map<string, string>::value_type("xlt", "application/vnd.ms-excel"));
  m_type_table.insert(
      map<string, string>::value_type("xlw", "application/vnd.ms-excel"));
  m_type_table.insert(map<string, string>::value_type("xof", "x-world/x-vrml"));
  m_type_table.insert(
      map<string, string>::value_type("xpm", "image/x-xpixmap"));
  m_type_table.insert(
      map<string, string>::value_type("xwd", "image/x-xwindowdump"));
  m_type_table.insert(
      map<string, string>::value_type("z", "application/x-compress"));
  m_type_table.insert(
      map<string, string>::value_type("zip", "application/zip"));

  load_http2_push_list("/_http2push/");
}

void memory_cache::load_http2_push_list(const char* dir_path) {
  string full_path = m_dirpath.c_str();
  full_path += "/html/";
  full_path += dir_path;

  struct dirent* dirp;
  DIR* dp = opendir(full_path.c_str());
  if (dp) {
    while ((dirp = readdir(dp)) != NULL) {
      /* variable file name should have postfix as ".server" */
      if (dirp->d_type == DT_REG) {
        int name_len = strlen(dirp->d_name);
        int css_len = sizeof(".css") - 1;
        int js_len = sizeof(".js") - 1;

        BOOL is_pushed = FALSE;
        if (name_len > css_len &&
            (dirp->d_name[name_len - 1] == 's' ||
             dirp->d_name[name_len - 1] == 'S') &&
            (dirp->d_name[name_len - 2] == 's' ||
             dirp->d_name[name_len - 2] == 'S') &&
            (dirp->d_name[name_len - 3] == 'c' ||
             dirp->d_name[name_len - 2] == 'C') &&
            dirp->d_name[name_len - 4] == '.') {
          is_pushed = TRUE;
        } else if (name_len > js_len &&
                   (dirp->d_name[name_len - 1] == 's' ||
                    dirp->d_name[name_len - 1] == 'S') &&
                   (dirp->d_name[name_len - 2] == 'j' ||
                    dirp->d_name[name_len - 2] == 'J') &&
                   dirp->d_name[name_len - 3] == '.') {
          is_pushed = TRUE;
        }
        if (is_pushed) {
          http2_push_file_t push_file;
          push_file.path = dir_path;
          push_file.path += "/";
          push_file.path += dirp->d_name;

          push_file.full_path = m_dirpath.c_str();
          push_file.full_path += "/html/";
          push_file.full_path += push_file.path;

          GlobalReplace(push_file.path, "//", "/");
          GlobalReplace(push_file.full_path, "//", "/");

          m_http2_push_list.push_back(push_file);
        }
      } else if (dirp->d_type == DT_DIR) {
        if (strcmp(dirp->d_name, ".") != 0 && strcmp(dirp->d_name, "..") != 0) {
          string next_path = dir_path;
          next_path += "/";
          next_path += dirp->d_name;

          load_http2_push_list(next_path.c_str());
        }
      }
    }
    closedir(dp);
  }
}
