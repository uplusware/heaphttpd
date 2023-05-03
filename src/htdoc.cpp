/*
        Copyright (c) openheap, uplusware
        uplusware@gmail.com
*/

#include "htdoc.h"
#include <pthread.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "fastcgi.h"
#include "httpcomm.h"
#include "httprequest.h"
#include "httpresponse.h"
#include "scgi.h"
#include "util/md5.h"
#include "util/sha1.h"
#include "util/sysdep.h"
#include "util/uuid.h"
#include "webdoc.h"
#include "websocket.h"
#include "wwwauth.h"

void Htdoc::Response() {
  // default is complete, can be reset in the following code per the requirement
  m_htdoc_state = htdocComplete;

  if (m_session->GetWebSocketHandShake()) {
    if (m_session->GetWebSocketHandShake() == Websocket_Nak) {
      CHttpResponseHdr header(m_session->GetResponseHeader()->GetMap());
      header.SetStatusCode(SC400);

      header.SetField("Content-Type", "text/html");
      header.SetField("Content-Length", header.GetDefaultHTMLLength());
      m_session->SendHeader(header.Text(), header.Length());
      m_session->SendContent(header.GetDefaultHTML(),
                             header.GetDefaultHTMLLength());
      return;
    } else {
      string strwskey = m_session->GetRequestField("Sec-WebSocket-Key");
      strtrim(strwskey);
      if (strwskey != "") {
        string strwsaccept = strwskey;
        strwsaccept += "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";  // a fixed UUID

        unsigned char Message_Digest[20];
        SHA1Context sha;
        SHA1Reset(&sha);
        SHA1Input(&sha, (unsigned char*)strwsaccept.c_str(),
                  strwsaccept.length());
        SHA1Result(&sha, Message_Digest);

        int outlen = BASE64_ENCODE_OUTPUT_MAX_LEN(sizeof(Message_Digest));
        char* szwsaccept_b64 = (char*)malloc(outlen);
        CBase64::Encode((char*)Message_Digest, sizeof(Message_Digest),
                        szwsaccept_b64, &outlen);
        szwsaccept_b64[outlen] = '\0';
        string strwsaccept_b64 = szwsaccept_b64;
        free(szwsaccept_b64);

        CHttpResponseHdr header(m_session->GetResponseHeader()->GetMap());
        header.SetStatusCode(SC101);
        header.SetField("Upgrade", "websocket");
        header.SetField("Connection", "Upgrade");
        header.SetField("Sec-WebSocket-Accept", strwsaccept_b64.c_str());
        m_session->SendHeader(header.Text(), header.Length());

        m_session->SetWebSocketHandShake(Websocket_Ack);

        /* - WebSocket handshake finishes, begin to communication -*/
        string strResource = m_session->GetResource();
        strtrim(strResource, "/");

        if (strResource != "") {
          string strWSFunctionName = "ws_";
          strWSFunctionName += strResource;
          strWSFunctionName += "_main";

          string strLibName = m_work_path.c_str();
          strLibName += "/ws/lib";
          strLibName += strResource.c_str();
          strLibName += ".so";

          struct stat statbuf;
          if (stat(strLibName.c_str(), &statbuf) < 0) {
            CHttpResponseHdr header(m_session->GetResponseHeader()->GetMap());
            header.SetStatusCode(SC404);

            header.SetField("Content-Type", "text/html");
            header.SetField("Content-Length", header.GetDefaultHTMLLength());
            m_session->SendHeader(header.Text(), header.Length());
            m_session->SendContent(header.GetDefaultHTML(),
                                   header.GetDefaultHTMLLength());
            return;
          }
          void* lhandle = dlopen(strLibName.c_str(), RTLD_LOCAL | RTLD_NOW);
          if (!lhandle) {
            fprintf(stderr, "dlopen %s\n", dlerror());
            CHttpResponseHdr header(m_session->GetResponseHeader()->GetMap());
            header.SetStatusCode(SC500);

            header.SetField("Content-Type", "text/html");
            header.SetField("Content-Length", header.GetDefaultHTMLLength());
            m_session->SendHeader(header.Text(), header.Length());
            m_session->SendContent(header.GetDefaultHTML(),
                                   header.GetDefaultHTMLLength());

          } else {
            dlerror();
            void* (*ws_main)(int, SSL*);
            ws_main =
                (void* (*)(int, SSL*))dlsym(lhandle, strWSFunctionName.c_str());
            const char* errmsg;
            if ((errmsg = dlerror()) == NULL) {
              ws_main(m_session->GetSocket(), m_session->GetSSL());
            } else {
              fprintf(stderr, "dlsym %s\n", errmsg);

              CHttpResponseHdr header(m_session->GetResponseHeader()->GetMap());
              header.SetStatusCode(SC500);

              header.SetField("Content-Type", "text/html");
              header.SetField("Content-Length", header.GetDefaultHTMLLength());
              m_session->SendHeader(header.Text(), header.Length());
              m_session->SendContent(header.GetDefaultHTML(),
                                     header.GetDefaultHTMLLength());
            }
            dlclose(lhandle);
          }
        }
        return;
      }
    }
  }

  if (m_session->GetMethod() != hmPost && m_session->GetMethod() != hmGet &&
      m_session->GetMethod() != hmHead) {
    CHttpResponseHdr header(m_session->GetResponseHeader()->GetMap());
    header.SetStatusCode(SC501);

    header.SetField("Content-Type", "text/html");
    header.SetField("Content-Length", header.GetDefaultHTMLLength());
    m_session->SendHeader(header.Text(), header.Length());
    m_session->SendContent(header.GetDefaultHTML(),
                           header.GetDefaultHTMLLength());
    return;
  }
  string strResource = m_session->GetResource();
  string strResp;
  strResp.clear();
  vector<string> vPath;

  if (strResource != "") {
    vSplitString(strResource, vPath, "/", TRUE, 0x7FFFFFFF);

    int nCDUP = 0;
    int nCDIN = 0;
    for (int x = 0; x < vPath.size(); x++) {
      if (vPath[x] == "..") {
        nCDUP++;
      } else if (vPath[x] == ".") {
        // do nothing
      } else {
        nCDIN++;
      }
    }
    if (nCDUP >= nCDIN) {
      strResource = "/";
    }
  }

  m_session->SetResource(strResource.c_str());

  if (strncmp(strResource.c_str(), "/api/", 5) == 0) {
    string strApiName = strResource.c_str() + 5; /* length of "/api/" */
    string strLibName = m_work_path.c_str();
    strLibName += "/api/lib";
    strLibName += strApiName;
    strLibName += ".so";

    struct stat statbuf;
    if (stat(strLibName.c_str(), &statbuf) < 0) {
      CHttpResponseHdr header(m_session->GetResponseHeader()->GetMap());
      header.SetStatusCode(SC404);

      header.SetField("Content-Type", "text/html");
      header.SetField("Content-Length", header.GetDefaultHTMLLength());
      m_session->SendHeader(header.Text(), header.Length());
      m_session->SendContent(header.GetDefaultHTML(),
                             header.GetDefaultHTMLLength());
      return;
    }

    void* lhandle = dlopen(strLibName.c_str(), RTLD_LOCAL | RTLD_NOW);
    if (!lhandle) {
      CHttpResponseHdr header(m_session->GetResponseHeader()->GetMap());
      header.SetStatusCode(SC500);

      header.SetField("Content-Type", "text/html");
      header.SetField("Content-Length", header.GetDefaultHTMLLength());

      m_session->SendHeader(header.Text(), header.Length());
      m_session->SendContent(header.GetDefaultHTML(),
                             header.GetDefaultHTMLLength());
      fprintf(stderr, "dlopen %s\n", dlerror());
    } else {
      dlerror();
      string strApiFunctionName = "api_";
      strApiFunctionName += strApiName;
      strApiFunctionName += "_response";

      http_request request_inst(m_session);
      http_response response_inst(m_session);

      void* (*api_response)(http_request*, http_response*);
      api_response = (void* (*)(http_request*, http_response*))dlsym(
          lhandle, strApiFunctionName.c_str());
      const char* errmsg;
      if ((errmsg = dlerror()) == NULL) {
        api_response(&request_inst, &response_inst);
      } else {
        CHttpResponseHdr header(m_session->GetResponseHeader()->GetMap());
        header.SetStatusCode(SC500);

        header.SetField("Content-Type", "text/html");
        header.SetField("Content-Length", header.GetDefaultHTMLLength());

        m_session->SendHeader(header.Text(), header.Length());
        m_session->SendContent(header.GetDefaultHTML(),
                               header.GetDefaultHTMLLength());
        fprintf(stderr, "dlsym %s\n", errmsg);
      }
      dlclose(lhandle);
    }

  } else if (strncmp(strResource.c_str(), "/cgi-bin/", 8) == 0) {
    string script_name;
    string path_info;

    strcut(m_session->GetResource(), NULL, ".cgi", script_name);
    strcut(m_session->GetResource(), ".cgi", NULL, path_info);
    script_name += ".cgi";

    string strcgipath = m_work_path.c_str();
    strcgipath += script_name;

    if (access(strcgipath.c_str(), 01) != -1) {
      m_session->SetMetaVar("SCRIPT_NAME", script_name.c_str());
      m_session->SetMetaVar("PATH_INFO", path_info.c_str());

      int fds[2];
      socketpair(AF_UNIX, SOCK_STREAM, 0, fds);

      pid_t cgipid = fork();
      if (cgipid > 0) {
        close(fds[1]);

        forkcgi* cgi_instance = new forkcgi(fds[0]);
        cgi_instance->SendData(m_session->m_cgi.GetData(),
                               m_session->m_cgi.GetDataLen());

        shutdown(fds[0], SHUT_WR);

        BOOL continue_recv;
        vector<char> binaryResponse;
        string strHeader;
        cgi_instance->ReadAppData(binaryResponse, continue_recv);
        if (binaryResponse.size() > 0) {
          const char* pbody;
          unsigned int body_len;
          memsplit2str(&binaryResponse[0], binaryResponse.size(), "\r\n\r\n",
                       strHeader, pbody, body_len);

          /*
             memsplit2str removed the \r\n\r\n.
             they are the last non-empty line's \r\n and the last empty line in
             header. So need to appending \r\n
          */
          if (strHeader != "") {
            strHeader += "\r\n";
          }

          CHttpResponseHdr header(m_session->GetResponseHeader()->GetMap());
          if (strncasecmp(strHeader.c_str(),
                          "Status: ", sizeof("Status: ") - 1) == 0) {
            unsigned int nStatusCode = 200;
            sscanf(strHeader.c_str(), "Status: %d %*", &nStatusCode);

            char szStatusCode[64];
            sprintf(szStatusCode, "%u", nStatusCode);
            header.SetStatusCode(szStatusCode);

            header.SetFields(strstr(strHeader.c_str(), "\r\n") +
                             2 /* 2 is length of \r\n */);
          } else {
            header.SetStatusCode(SC200);
            header.SetFields(strHeader.c_str());
          }

          m_session->SendHeader(header.Text(), header.Length());

          if (body_len > 0 && m_session->GetMethod() != hmHead)
            m_session->SendContent(pbody, body_len);

          while (continue_recv) {
            binaryResponse.clear();
            cgi_instance->ReadAppData(binaryResponse, continue_recv);
            if (binaryResponse.size() > 0 && m_session->GetMethod() != hmHead)
              m_session->SendContent(&binaryResponse[0], binaryResponse.size());
          }
        } else {
          CHttpResponseHdr header(m_session->GetResponseHeader()->GetMap());
          header.SetStatusCode(SC500);

          header.SetField("Content-Type", "text/html");
          header.SetField("Content-Length", header.GetDefaultHTMLLength());

          m_session->SendHeader(header.Text(), header.Length());
          m_session->SendContent(header.GetDefaultHTML(),
                                 header.GetDefaultHTMLLength());
        }
        close(fds[0]);
        if (cgi_instance)
          delete cgi_instance;
        waitpid(cgipid, NULL, 0);
      } else if (cgipid == 0) {
        close(fds[0]);
        m_session->SetMetaVarsToEnv();
        dup2(fds[1], STDIN_FILENO);
        dup2(fds[1], STDOUT_FILENO);
        dup2(fds[1], STDERR_FILENO);
        close(fds[1]);
        delete m_session;
        execl(strcgipath.c_str(), NULL);
        exit(-1);
      } else {
        close(fds[0]);
        close(fds[1]);
      }
    } else {
      CHttpResponseHdr header(m_session->GetResponseHeader()->GetMap());
      header.SetStatusCode(SC403);

      header.SetField("Content-Type", "text/html");
      header.SetField("Content-Length", header.GetDefaultHTMLLength());

      m_session->SendHeader(header.Text(), header.Length());
      m_session->SendContent(header.GetDefaultHTML(),
                             header.GetDefaultHTMLLength());
    }
  } else {
    string cgi_uniq_name;
    map<string, cgi_cfg_t>::iterator cgi_cfg;
    for (cgi_cfg = (*m_cgi_list).begin(); cgi_cfg != (*m_cgi_list).end();
         cgi_cfg++) {
      string fcgi_name = "/";
      fcgi_name += cgi_cfg->second.cgi_name;
      fcgi_name += "/";

      if (strncmp(strResource.c_str(), fcgi_name.c_str(), fcgi_name.length()) ==
          0) {
        cgi_uniq_name = cgi_cfg->second.cgi_name;
        break;
      }
    }

    if (cgi_cfg != (*m_cgi_list).end()) {
      string pyfile = m_work_path.c_str();

      if (pyfile[pyfile.length() - 1] == '/')
        pyfile += "html";
      else
        pyfile += "/html";

      if (m_session->GetResource()[0] != '/')
        pyfile += "/";

      pyfile += m_session->GetResource();

      m_session->SetMetaVar("SCRIPT_FILENAME", cgi_cfg->second.cgi_pgm.c_str());
      m_session->SetMetaVar("REDIRECT_STATUS", "200");

      if (cgi_cfg->second.cgi_type == fastcgi_e) {
        fastcgi* fcgi_instance = NULL;

        map<string, fastcgi*>::iterator iter_instance =
            m_fastcgi_instances->find(cgi_uniq_name);

        if (iter_instance == m_fastcgi_instances->end()) {
          if (cgi_cfg->second.cgi_socktype == unix_socket)
            fcgi_instance = new fastcgi(cgi_cfg->second.cgi_sockfile.c_str());
          else if (cgi_cfg->second.cgi_socktype == inet_socket)
            fcgi_instance = new fastcgi(cgi_cfg->second.cgi_addr.c_str(),
                                        cgi_cfg->second.cgi_port);
          else {
            CHttpResponseHdr header(m_session->GetResponseHeader()->GetMap());
            header.SetStatusCode(SC500);

            header.SetField("Content-Type", "text/html");
            header.SetField("Content-Length", header.GetDefaultHTMLLength());

            m_session->SendHeader(header.Text(), header.Length());
            m_session->SendContent(header.GetDefaultHTML(),
                                   header.GetDefaultHTMLLength());
            return;
          }

          if (fcgi_instance) {
            m_fastcgi_instances->insert(map<string, fastcgi*>::value_type(
                cgi_uniq_name, fcgi_instance));
          } else {
            CHttpResponseHdr header(m_session->GetResponseHeader()->GetMap());
            header.SetStatusCode(SC500);

            header.SetField("Content-Type", "text/html");
            header.SetField("Content-Length", header.GetDefaultHTMLLength());

            m_session->SendHeader(header.Text(), header.Length());
            m_session->SendContent(header.GetDefaultHTML(),
                                   header.GetDefaultHTMLLength());

            return;
          }
        } else {
          fcgi_instance = iter_instance->second;
        }

        if (fcgi_instance && fcgi_instance->Connect() == 0 &&
            fcgi_instance->BeginRequest() == 0) {
          if (m_session->m_cgi.m_meta_var.size() > 0)
            fcgi_instance->SendParams(m_session->m_cgi.m_meta_var);
          fcgi_instance->SendEmptyParams();
          if (m_session->m_cgi.GetDataLen() > 0) {
            fcgi_instance->SendRequestData(m_session->m_cgi.GetData(),
                                           m_session->m_cgi.GetDataLen());
          }
          fcgi_instance->SendEmptyRequestData();

          vector<char> binaryResponse;
          string strHeader;
          string strerr;
          unsigned int appstatus;
          unsigned char protocolstatus;
          BOOL continue_recv;

          fcgi_instance->RecvAppData(binaryResponse, strerr, appstatus,
                                     protocolstatus, continue_recv);

          if (binaryResponse.size() > 0) {
            const char* pbody;
            unsigned int body_len;
            memsplit2str(&binaryResponse[0], binaryResponse.size(), "\r\n\r\n",
                         strHeader, pbody, body_len);

            /* memsplit2str removed the \r\n\r\n.
               they are the last non-empty line's \r\n and the last empty line
               in header. So need to appending \r\n
            */
            if (strHeader != "") {
              strHeader += "\r\n";
            }

            CHttpResponseHdr header(m_session->GetResponseHeader()->GetMap());
            if (strncasecmp(strHeader.c_str(),
                            "Status: ", sizeof("Status: ") - 1) == 0) {
              unsigned int nStatusCode = 200;
              sscanf(strHeader.c_str(), "Status: %d %*", &nStatusCode);

              char szStatusCode[64];
              sprintf(szStatusCode, "%u", nStatusCode);
              header.SetStatusCode(szStatusCode);

              header.SetFields(strstr(strHeader.c_str(), "\r\n") +
                               2 /* 2 is length of \r\n */);
            } else {
              header.SetStatusCode(SC200);
              header.SetFields(strHeader.c_str());
            }

            m_session->SendHeader(header.Text(), header.Length());

            if (body_len > 0 && m_session->GetMethod() != hmHead)
              m_session->SendContent(pbody, body_len);

            while (continue_recv) {
              binaryResponse.clear();
              fcgi_instance->RecvAppData(binaryResponse, strerr, appstatus,
                                         protocolstatus, continue_recv);
              if (binaryResponse.size() > 0 &&
                  m_session->GetMethod() != hmHead) {
                m_session->SendContent(&binaryResponse[0],
                                       binaryResponse.size());
              }
            }
          } else {
            CHttpResponseHdr header(m_session->GetResponseHeader()->GetMap());
            header.SetStatusCode(SC500);

            header.SetField("Content-Type", "text/html");
            header.SetField("Content-Length", header.GetDefaultHTMLLength());

            m_session->SendHeader(header.Text(), header.Length());
            m_session->SendContent(header.GetDefaultHTML(),
                                   header.GetDefaultHTMLLength());
            return;
          }
        } else {
          CHttpResponseHdr header(m_session->GetResponseHeader()->GetMap());
          header.SetStatusCode(SC500);

          header.SetField("Content-Type", "text/html");
          header.SetField("Content-Length", header.GetDefaultHTMLLength());

          m_session->SendHeader(header.Text(), header.Length());
          m_session->SendContent(header.GetDefaultHTML(),
                                 header.GetDefaultHTMLLength());
          return;
        }
      } else if (cgi_cfg->second.cgi_type == scgi_e) {
        scgi* scgi_instance = NULL;

        map<string, scgi*>::iterator iter_instance =
            m_scgi_instances->find(cgi_uniq_name);

        if (iter_instance == m_scgi_instances->end()) {
          if (cgi_cfg->second.cgi_socktype == unix_socket)
            scgi_instance = new scgi(cgi_cfg->second.cgi_sockfile.c_str());
          else if (cgi_cfg->second.cgi_socktype == inet_socket)
            scgi_instance = new scgi(cgi_cfg->second.cgi_addr.c_str(),
                                     cgi_cfg->second.cgi_port);
          else {
            CHttpResponseHdr header(m_session->GetResponseHeader()->GetMap());
            header.SetStatusCode(SC500);

            header.SetField("Content-Type", "text/html");
            header.SetField("Content-Length", header.GetDefaultHTMLLength());

            m_session->SendHeader(header.Text(), header.Length());
            m_session->SendContent(header.GetDefaultHTML(),
                                   header.GetDefaultHTMLLength());

            return;
          }

          if (scgi_instance) {
            m_scgi_instances->insert(
                map<string, scgi*>::value_type(cgi_uniq_name, scgi_instance));
          } else {
            CHttpResponseHdr header(m_session->GetResponseHeader()->GetMap());
            header.SetStatusCode(SC500);

            header.SetField("Content-Type", "text/html");
            header.SetField("Content-Length", header.GetDefaultHTMLLength());

            m_session->SendHeader(header.Text(), header.Length());
            m_session->SendContent(header.GetDefaultHTML(),
                                   header.GetDefaultHTMLLength());

            return;
          }
        } else {
          scgi_instance = iter_instance->second;
        }

        if (scgi_instance && scgi_instance->Connect() == 0) {
          vector<char> binaryResponse;
          string strHeader;
          BOOL continue_recv;
          if (scgi_instance->SendParamsAndData(
                  m_session->m_cgi.m_meta_var, m_session->m_cgi.GetData(),
                  m_session->m_cgi.GetDataLen()) >= 0 &&
              scgi_instance->RecvAppData(binaryResponse, continue_recv) > 0) {
            const char* pbody;
            unsigned int body_len;
            memsplit2str(&binaryResponse[0], binaryResponse.size(), "\r\n\r\n",
                         strHeader, pbody, body_len);

            /* memsplit2str removed the \r\n\r\n.
               they are the last non-empty line's \r\n and the last empty line
               in header. So need to appending \r\n
            */
            if (strHeader != "") {
              strHeader += "\r\n";
            }

            CHttpResponseHdr header(m_session->GetResponseHeader()->GetMap());
            if (strncasecmp(strHeader.c_str(),
                            "Status: ", sizeof("Status: ") - 1) == 0) {
              unsigned int nStatusCode = 200;
              sscanf(strHeader.c_str(), "Status: %d %*", &nStatusCode);

              char szStatusCode[64];
              sprintf(szStatusCode, "%u", nStatusCode);
              header.SetStatusCode(szStatusCode);

              header.SetFields(strstr(strHeader.c_str(), "\r\n") +
                               2 /* 2 is length of \r\n */);
            } else {
              header.SetStatusCode(SC200);
              header.SetFields(strHeader.c_str());
            }

            header.SetField("Content-Length", body_len);

            m_session->SendHeader(header.Text(), header.Length());
            if (body_len > 0 && m_session->GetMethod() != hmHead)
              m_session->SendContent(pbody, body_len);

            while (continue_recv) {
              binaryResponse.clear();
              scgi_instance->RecvAppData(binaryResponse, continue_recv);
              if (binaryResponse.size() > 0 && m_session->GetMethod() != hmHead)
                m_session->SendContent(&binaryResponse[0],
                                       binaryResponse.size());
            }
          } else {
            CHttpResponseHdr header(m_session->GetResponseHeader()->GetMap());
            header.SetStatusCode(SC500);

            header.SetField("Content-Type", "text/html");
            header.SetField("Content-Length", header.GetDefaultHTMLLength());

            m_session->SendHeader(header.Text(), header.Length());
            m_session->SendContent(header.GetDefaultHTML(),
                                   header.GetDefaultHTMLLength());
          }
        } else {
          CHttpResponseHdr header(m_session->GetResponseHeader()->GetMap());
          header.SetStatusCode(SC500);

          header.SetField("Content-Type", "text/html");
          header.SetField("Content-Length", header.GetDefaultHTMLLength());

          m_session->SendHeader(header.Text(), header.Length());
          m_session->SendContent(header.GetDefaultHTML(),
                                 header.GetDefaultHTMLLength());
        }

        return;
      } else {
        CHttpResponseHdr header(m_session->GetResponseHeader()->GetMap());
        header.SetStatusCode(SC500);

        header.SetField("Content-Type", "text/html");
        header.SetField("Content-Length", header.GetDefaultHTMLLength());

        m_session->SendHeader(header.Text(), header.Length());
        m_session->SendContent(header.GetDefaultHTML(),
                               header.GetDefaultHTMLLength());
      }
    } else {
      string resoucefile = m_work_path.c_str();

      if (resoucefile[resoucefile.length() - 1] == '/')
        resoucefile += "html";
      else
        resoucefile += "/html";

      if (m_session->GetResource()[0] != '/')
        resoucefile += "/";

      resoucefile += m_session->GetResource();

      struct stat resource_statbuf;
      if (stat(resoucefile.c_str(), &resource_statbuf) == 0) {
        if (S_ISDIR(resource_statbuf.st_mode)) {
          if (resoucefile[resoucefile.length() - 1] != '/') {
            CHttpResponseHdr header(m_session->GetResponseHeader()->GetMap());
            header.SetStatusCode(SC403);

            header.SetField("Content-Type", "text/html");
            header.SetField("Content-Length", header.GetDefaultHTMLLength());

            m_session->SendHeader(header.Text(), header.Length());
            m_session->SendContent(header.GetDefaultHTML(),
                                   header.GetDefaultHTMLLength());
            return;
          }
          vector<string>::iterator it_dftpage;
          for (it_dftpage = (*m_default_webpages).begin();
               it_dftpage != (*m_default_webpages).end(); it_dftpage++) {
            string default_resoucefile = resoucefile;
            if (default_resoucefile[default_resoucefile.length() - 1] != '/')
              default_resoucefile += "/";
            default_resoucefile += (*it_dftpage).c_str();
            if (stat(default_resoucefile.c_str(), &resource_statbuf) == 0 &&
                !S_ISDIR(resource_statbuf.st_mode)) {
              string strDefaultResource = m_session->GetResource();

              if (strDefaultResource[strDefaultResource.length() - 1] != '/')
                strDefaultResource += "/";

              strDefaultResource += (*it_dftpage).c_str();
              m_session->SetResource(strDefaultResource.c_str());
              break;
            }
          }
        }
      }

      string strResource, strExtName;
      lowercase(m_session->GetResource(), strResource);
      get_extend_name(strResource.c_str(), strExtName);

      if (strExtName == "php") {
        if (strcasecmp(m_php_mode.c_str(), "cgi") == 0) {
          int fds[2];
          socketpair(AF_UNIX, SOCK_STREAM, 0, fds);

          pid_t cgipid = fork();
          if (cgipid > 0) {
            close(fds[1]);

            forkcgi* cgi_instance = new forkcgi(fds[0]);
            cgi_instance->SendData(m_session->m_cgi.GetData(),
                                   m_session->m_cgi.GetDataLen());
            shutdown(fds[0], SHUT_WR);

            BOOL continue_recv;
            vector<char> binaryResponse;
            string strHeader;
            cgi_instance->ReadAppData(binaryResponse, continue_recv);
            if (binaryResponse.size() > 0) {
              const char* pbody;
              unsigned int body_len;
              memsplit2str(&binaryResponse[0], binaryResponse.size(),
                           "\r\n\r\n", strHeader, pbody, body_len);

              /* memsplit2str removed the \r\n\r\n.
                 they are the last non-empty line's \r\n and the last empty line
                 in header. So need to appending \r\n
              */
              if (strHeader != "") {
                strHeader += "\r\n";
              }

              string strWanning, strParseError;

              strcut(strHeader.c_str(), "PHP Warning:", "\n", strWanning);
              strcut(strHeader.c_str(), "PHP Parse error:", "\n",
                     strParseError);

              strtrim(strWanning);
              strtrim(strParseError);

              if (strWanning != "")
                fprintf(stderr, "PHP Warning: %s\n", strWanning.c_str());
              if (strParseError != "")
                fprintf(stderr, "PHP Parse error:: %s\n",
                        strParseError.c_str());

              CHttpResponseHdr header(m_session->GetResponseHeader()->GetMap());
              if (strncasecmp(strHeader.c_str(),
                              "Status: ", sizeof("Status: ") - 1) == 0) {
                unsigned int nStatusCode = 200;
                sscanf(strHeader.c_str(), "Status: %d %*", &nStatusCode);

                char szStatusCode[64];
                sprintf(szStatusCode, "%u", nStatusCode);
                header.SetStatusCode(szStatusCode);

                header.SetFields(strstr(strHeader.c_str(), "\r\n") +
                                 2 /* 2 is length of \r\n */);
              } else {
                header.SetStatusCode(SC200);
                header.SetFields(strHeader.c_str());
              }

              m_session->SendHeader(header.Text(), header.Length());

              if (body_len > 0 && m_session->GetMethod() != hmHead)
                m_session->SendContent(pbody, body_len);

              while (continue_recv) {
                binaryResponse.clear();
                cgi_instance->ReadAppData(binaryResponse, continue_recv);
                if (binaryResponse.size() > 0 &&
                    m_session->GetMethod() != hmHead)
                  m_session->SendContent(&binaryResponse[0],
                                         binaryResponse.size());
              }
            } else {
              CHttpResponseHdr header(m_session->GetResponseHeader()->GetMap());
              header.SetStatusCode(SC500);

              header.SetField("Content-Type", "text/html");
              header.SetField("Content-Length", header.GetDefaultHTMLLength());

              m_session->SendHeader(header.Text(), header.Length());
              m_session->SendContent(header.GetDefaultHTML(),
                                     header.GetDefaultHTMLLength());
            }
            close(fds[0]);
            if (cgi_instance)
              delete cgi_instance;
            waitpid(cgipid, NULL, 0);
          } else if (cgipid == 0) {
            string phpfile = m_work_path.c_str();

            if (phpfile[phpfile.length() - 1] == '/')
              phpfile += "html";
            else
              phpfile += "/html";

            if (m_session->GetResource()[0] != '/')
              phpfile += "/";

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
          } else {
            close(fds[0]);
            close(fds[1]);
          }
        } else if (strcasecmp(m_php_mode.c_str(), "fpm") == 0) {
          string phpfile = m_work_path.c_str();

          if (phpfile[phpfile.length() - 1] == '/')
            phpfile += "html";
          else
            phpfile += "/html";

          if (m_session->GetResource()[0] != '/')
            phpfile += "/";

          phpfile += m_session->GetResource();

          m_session->SetMetaVar("SCRIPT_FILENAME", phpfile.c_str());
          m_session->SetMetaVar("REDIRECT_STATUS", "200");

          if (!m_php_fpm_instance) {
            if (m_fpm_socktype == unix_socket)
              m_php_fpm_instance = new fastcgi(m_fpm_sockfile.c_str());
            else if (m_fpm_socktype == inet_socket)
              m_php_fpm_instance = new fastcgi(m_fpm_addr.c_str(), m_fpm_port);
            else {
              CHttpResponseHdr header(m_session->GetResponseHeader()->GetMap());
              header.SetStatusCode(SC500);

              header.SetField("Content-Type", "text/html");
              header.SetField("Content-Length", header.GetDefaultHTMLLength());

              m_session->SendHeader(header.Text(), header.Length());
              m_session->SendContent(header.GetDefaultHTML(),
                                     header.GetDefaultHTMLLength());
              return;
            }
          }
          if (m_php_fpm_instance && m_php_fpm_instance->Connect() == 0 &&
              m_php_fpm_instance->BeginRequest() == 0) {
            if (m_session->m_cgi.m_meta_var.size() > 0)
              m_php_fpm_instance->SendParams(m_session->m_cgi.m_meta_var);
            m_php_fpm_instance->SendEmptyParams();
            if (m_session->m_cgi.GetDataLen() > 0) {
              m_php_fpm_instance->SendRequestData(
                  m_session->m_cgi.GetData(), m_session->m_cgi.GetDataLen());
            }
            m_php_fpm_instance->SendEmptyRequestData();

            vector<char> binaryResponse;
            string strHeader, strDebug;
            string strerr;
            unsigned int appstatus;
            unsigned char protocolstatus;
            BOOL continue_recv;

            m_php_fpm_instance->RecvAppData(binaryResponse, strerr, appstatus,
                                            protocolstatus, continue_recv);

            if (binaryResponse.size() > 0) {
              const char* pbody;
              unsigned int body_len;
              memsplit2str(&binaryResponse[0], binaryResponse.size(),
                           "\r\n\r\n", strHeader, pbody, body_len);

              /* memsplit2str removed the \r\n\r\n.
                 they are the last non-empty line's \r\n and the last empty line
                 in header. So need to appending \r\n
              */
              if (strHeader != "") {
                strHeader += "\r\n";
              }

              CHttpResponseHdr header(m_session->GetResponseHeader()->GetMap());

              if (strncasecmp(strHeader.c_str(),
                              "Status: ", sizeof("Status: ") - 1) == 0) {
                unsigned int nStatusCode = 200;
                sscanf(strHeader.c_str(), "Status: %d %*", &nStatusCode);

                char szStatusCode[64];
                sprintf(szStatusCode, "%u", nStatusCode);
                header.SetStatusCode(szStatusCode);

                header.SetFields(strstr(strHeader.c_str(), "\r\n") +
                                 2 /* 2 is length of \r\n */);
              } else {
                header.SetStatusCode(SC200);
                header.SetFields(strHeader.c_str());
              }

              string strWanning, strParseError;

              strcut(strHeader.c_str(), "PHP Warning:", "\n", strWanning);
              strcut(strHeader.c_str(), "PHP Parse error:", "\n",
                     strParseError);

              strtrim(strWanning);
              strtrim(strParseError);

              if (strWanning != "")
                fprintf(stderr, "PHP Warning: %s\n", strWanning.c_str());
              if (strParseError != "")
                fprintf(stderr, "PHP Parse error:: %s\n",
                        strParseError.c_str());

              m_session->SendHeader(header.Text(), header.Length());
              if (body_len > 0 && m_session->GetMethod() != hmHead) {
                m_session->SendContent(pbody, body_len);
                if (strDebug.length() > 0)
                  m_session->SendContent(strDebug.c_str(), strDebug.length());
              }
              while (continue_recv) {
                binaryResponse.clear();
                m_php_fpm_instance->RecvAppData(binaryResponse, strerr,
                                                appstatus, protocolstatus,
                                                continue_recv);
                if (binaryResponse.size() > 0 &&
                    m_session->GetMethod() != hmHead)
                  m_session->SendContent(&binaryResponse[0],
                                         binaryResponse.size());
              }
            } else {
              CHttpResponseHdr header(m_session->GetResponseHeader()->GetMap());
              header.SetStatusCode(SC500);

              header.SetField("Content-Type", "text/html");
              header.SetField("Content-Length", header.GetDefaultHTMLLength());

              m_session->SendHeader(header.Text(), header.Length());
              m_session->SendContent(header.GetDefaultHTML(),
                                     header.GetDefaultHTMLLength());
            }
          } else {
            CHttpResponseHdr header(m_session->GetResponseHeader()->GetMap());
            header.SetStatusCode(SC500);

            header.SetField("Content-Type", "text/html");
            header.SetField("Content-Length", header.GetDefaultHTMLLength());

            m_session->SendHeader(header.Text(), header.Length());
            m_session->SendContent(header.GetDefaultHTML(),
                                   header.GetDefaultHTMLLength());
          }
        } else {
          CHttpResponseHdr header(m_session->GetResponseHeader()->GetMap());
          header.SetStatusCode(SC501);

          header.SetField("Content-Type", "text/html");
          header.SetField("Content-Length", header.GetDefaultHTMLLength());

          m_session->SendHeader(header.Text(), header.Length());
          m_session->SendContent(header.GetDefaultHTML(),
                                 header.GetDefaultHTMLLength());
        }
      } else {
        web_doc* doc_inst = new web_doc(m_session, m_work_path.c_str());
        doc_inst->Response();
        delete doc_inst;
      }
    }
  }
}
