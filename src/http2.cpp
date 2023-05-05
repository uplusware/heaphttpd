/*
        Copyright (c) openheap, uplusware
        uplusware@gmail.com
*/

#include "http2.h"
#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h> /* See NOTES */
#include <time.h>
#include <unistd.h>
#include <algorithm>
#include "hpack.h"
#include "util/huffman.h"

#include "debug.h"

/* #define _http2_debug_ 1 */

#ifdef _http2_debug_
const char* error_table[] = {"NO_ERROR",
                             "PROTOCOL_ERROR",
                             "INTERNAL_ERROR",
                             "FLOW_CONTROL_ERROR",
                             "SETTINGS_TIMEOUT",
                             "STREAM_CLOSED",
                             "FRAME_SIZE_ERROR",
                             "REFUSED_STREAM",
                             "CANCEL",
                             "COMPRESSION_ERROR",
                             "CONNECT_ERROR",
                             "ENHANCE_YOUR_CALM",
                             "INADEQUATE_SECURITY",
                             "HTTP_1_1_REQUIRED"};

const char* frame_names[] = {
    "DATA",         "HEADERS", "PRIORITY", "RST_STREAM",    "SETTINGS",
    "PUSH_PROMISE", "PING",    "GOAWAY",   "WINDOW_UPDATE", "CONTINUATION"};

const char* setting_table[] = {"undefined",           "HEADER_TABLE_SIZE",
                               "ENABLE_PUSH",         "MAX_CONCURRENT_STREAMS",
                               "INITIAL_WINDOW_SIZE", "MAX_FRAME_SIZE",
                               "MAX_HEADER_LIST_SIZE"};

const char* stream_state_desc[] = {"idle",
                                   "open",
                                   "reserved(local)",
                                   "reserved(remote)",
                                   "half closed(remote)",
                                   "half closed(local)",
                                   "closed"};
#endif /* _http2_debug_ */

#define HTTP2_DATA_PADDING_LEN 8
#define PRE_MALLOC_SIZE 1024

#define HTTP2_MAX_WINDOW_SIZE (2147483647L)
#define HTTP2_DEFAULT_WINDOW_SIZE 65535

CHttp2::CHttp2(int epoll_fd,
               map<int, backend_session*>* backend_list,
               time_t connection_first_request_time,
               time_t connection_keep_alive_timeout,
               unsigned int connection_keep_alive_request_tickets,
               http_tunneling* tunneling,
               fastcgi* php_fpm_instance,
               map<string, fastcgi*>* fastcgi_instances,
               map<string, scgi*>* scgi_instances,
               ServiceObjMap* srvobj,
               int sockfd,
               const char* servername,
               unsigned short serverport,
               const char* clientip,
               X509* client_cert,
               memory_cache* ch,
               const char* work_path,
               vector<string>* default_webpages,
               vector<http_extension_t>* ext_list,
               vector<http_extension_t>* reverse_ext_list,
               const char* php_mode,
               cgi_socket_t fpm_socktype,
               const char* fpm_sockfile,
               const char* fpm_addr,
               unsigned short fpm_port,
               const char* phpcgi_path,
               map<string, cgi_cfg_t>* cgi_list,
               BOOL push_promise,
               const char* private_path,
               AUTH_SCHEME wwwauth_scheme,
               AUTH_SCHEME proxyauth_scheme,
               SSL* ssl) {
  m_http2_state = http2Prefix;

  m_backend_list = backend_list;
  m_connection_first_request_time = connection_first_request_time;
  m_connection_keep_alive_timeout = connection_keep_alive_timeout;
  m_connection_keep_alive_request_tickets =
      connection_keep_alive_request_tickets;

  m_http_tunneling = tunneling;
  m_php_fpm_instance = php_fpm_instance;
  m_fastcgi_instances = fastcgi_instances;
  m_scgi_instances = scgi_instances;

  m_peer_control_window_size = HTTP2_DEFAULT_WINDOW_SIZE;
  m_local_control_window_size =
      HTTP2_DEFAULT_WINDOW_SIZE;  // hardcode per rfc7540

  m_header_table_size = 4096;      // hardcode per rfc7540
  m_enable_push = push_promise;    // TRUE per rfc7540
  m_max_concurrent_streams = 101;  // hardcode per rfc7540

  m_initial_local_window_size = HTTP2_MAX_WINDOW_SIZE;  // hardcode per rfc7540
  m_initial_peer_window_size =
      HTTP2_DEFAULT_WINDOW_SIZE;  // hardcode per rfc7540

  m_max_frame_size = 16384;  // hardcode per rfc7540
  m_max_header_list_size = 0xFFFFFFFF;

  m_pushed_request = FALSE;
  m_pushed_response = FALSE;

  m_srvobj = srvobj;
  m_sockfd = sockfd;
  m_epoll_fd = epoll_fd;
  m_servername = servername;
  m_serverport = serverport;
  m_clientip = clientip;
  m_client_cert = client_cert;
  m_ch = ch;
  m_work_path = work_path;
  m_ext_list = ext_list;
  m_reverse_ext_list = reverse_ext_list;
  m_default_webpages = default_webpages;
  m_php_mode = php_mode;
  m_fpm_socktype = fpm_socktype;
  m_fpm_sockfile = fpm_sockfile;
  m_fpm_addr = fpm_addr;
  m_fpm_port = fpm_port;
  m_phpcgi_path = phpcgi_path;

  m_cgi_list = cgi_list;
  m_private_path = private_path;
  m_wwwauth_scheme = wwwauth_scheme;
  m_proxyauth_scheme = proxyauth_scheme;
  m_ssl = ssl;

  m_lsockfd = NULL;
  m_lssl = NULL;

  if (m_ssl) {
    int flags = fcntl(m_sockfd, F_GETFL, 0);
    fcntl(m_sockfd, F_SETFL, flags | O_NONBLOCK);

    m_lssl = new linessl(m_sockfd, m_ssl);
  } else {
    int flags = fcntl(m_sockfd, F_GETFL, 0);
    fcntl(m_sockfd, F_SETFL, flags | O_NONBLOCK);

    m_lsockfd = new linesock(m_sockfd);
  }

  m_frame_hdr = new HTTP2_Frame;
  m_frame_hdr_valid_len = 0;

  m_payload = NULL;
  m_payload_valid_len = 0;

  init_header_table();
#ifndef _WITH_ASYNC_
  memset(m_preface, 0, HTTP2_PREFACE_LEN);
  int ret = HttpRecv(m_preface, HTTP2_PREFACE_LEN);
  m_preface[HTTP2_PREFACE_LEN] = '\0';
#ifdef _http2_debug_
  printf("[%s]\n", m_preface);
#endif /* _http2_debug_ */
  if (ret != HTTP2_PREFACE_LEN) {
    // clear resource before throw the exception
    for (int x; x < m_stream_list.size(); x++) {
      delete m_stream_list[x];
      m_stream_list[x] = NULL;
    }

    if (m_lssl)
      delete m_lssl;

    if (m_lsockfd)
      delete m_lsockfd;

    throw(new string("http2: couldn't get client preface."));
    return;
  }

  char client_preface_str[] = {0x50, 0x52, 0x49, 0x20, 0x2a, 0x20, 0x48,
                               0x54, 0x54, 0x50, 0x2f, 0x32, 0x2e, 0x30,
                               0x0d, 0x0a, 0x0d, 0x0a, 0x53, 0x4d, 0x0d,
                               0x0a, 0x0d, 0x0a, 0x00};
  if (strcmp(client_preface_str, m_preface) != 0) {
    send_goaway(0, HTTP2_PROTOCOL_ERROR);

    // clear resource before throw the exception
    for (int x; x < m_stream_list.size(); x++) {
      delete m_stream_list[x];
      m_stream_list[x] = NULL;
    }

    if (m_lssl)
      delete m_lssl;

    if (m_lsockfd)
      delete m_lsockfd;

    throw(new string("http2: wrong client preface content."));
    return;
  }
  char* server_preface =
      (char*)malloc(sizeof(HTTP2_Frame) + sizeof(HTTP2_Setting));

  HTTP2_Frame* preface_frame = (HTTP2_Frame*)server_preface;
  preface_frame->length.len3b[0] = 0x00;
  preface_frame->length.len3b[1] = 0x00;
  preface_frame->length.len3b[2] = 0x06;  // length is 6
  preface_frame->type = HTTP2_FRAME_TYPE_SETTINGS;
  preface_frame->flags = HTTP2_FRAME_FLAG_UNSET;
  preface_frame->r = HTTP2_FRAME_R_UNSET;
  preface_frame->identifier = HTTP2_FRAME_IDENTIFIER_WHOLE;

  HTTP2_Setting* preface_setting =
      (HTTP2_Setting*)(server_preface + sizeof(HTTP2_Frame));
  preface_setting->identifier = htons(HTTP2_SETTINGS_MAX_CONCURRENT_STREAMS);
  preface_setting->value = htonl(m_max_concurrent_streams);
  if (HttpSend(server_preface, sizeof(HTTP2_Frame) + sizeof(HTTP2_Setting)) !=
      0) {
    free(server_preface);

    // clear resource before throw the exception
    for (int x; x < m_stream_list.size(); x++) {
      delete m_stream_list[x];
      m_stream_list[x] = NULL;
    }

    if (m_lssl)
      delete m_lssl;

    if (m_lsockfd)
      delete m_lsockfd;

    throw(new string("HTTP2: Wrong server preface."));
    return;
  }
  free(server_preface);

  send_window_update(0, HTTP2_MAX_WINDOW_SIZE);
  send_initial_window_size(m_initial_local_window_size);
#endif /* _WITH_ASYNC_ */
}

CHttp2::~CHttp2() {
  for (int x; x < m_stream_list.size(); x++) {
    delete m_stream_list[x];
    m_stream_list[x] = NULL;
  }

  if (m_frame_hdr)
    delete m_frame_hdr;

  if (m_lssl)
    delete m_lssl;

  if (m_lsockfd)
    delete m_lsockfd;
}

void CHttp2::send_setting_ack(uint_32 stream_ind) {
  char* http2_buf = (char*)malloc(sizeof(HTTP2_Frame));
  HTTP2_Frame* setting_ack = (HTTP2_Frame*)http2_buf;

  setting_ack->length.len3b[0] = 0x00;
  setting_ack->length.len3b[1] = 0x00;
  setting_ack->length.len3b[2] = 0x00;  // length is 0
  setting_ack->type = HTTP2_FRAME_TYPE_SETTINGS;
  setting_ack->flags = HTTP2_FRAME_FLAG_SETTING_ACK;
  setting_ack->r = HTTP2_FRAME_R_UNSET;
  setting_ack->identifier = htonl(stream_ind) >> 1;

#ifdef _http2_debug_
  printf("  Send SETTING Ack for %d\n", stream_ind);
#endif /* _http2_debug_ */
  HttpSend((const char*)http2_buf, sizeof(HTTP2_Frame));

  free(http2_buf);
}

void CHttp2::send_rst_stream(uint_32 stream_ind) {
  http2_stream* http2_stream_inst = get_stream_instance(stream_ind);

  char* http2_buf =
      (char*)malloc(sizeof(HTTP2_Frame) + sizeof(HTTP2_Frame_Rst_Stream));
  HTTP2_Frame* http2_frm = (HTTP2_Frame*)http2_buf;
  http2_frm->length.len3b[0] = 0x00;
  http2_frm->length.len3b[1] = 0x00;
  http2_frm->length.len3b[2] = sizeof(HTTP2_Frame_Rst_Stream);
  http2_frm->type = HTTP2_FRAME_TYPE_RST_STREAM;
  http2_frm->flags = HTTP2_FRAME_FLAG_UNSET;
  http2_frm->r = HTTP2_FRAME_R_UNSET;
  http2_frm->identifier = htonl(stream_ind) >> 1;

  HTTP2_Frame_Rst_Stream* rst_stream =
      (HTTP2_Frame_Rst_Stream*)(http2_buf + sizeof(HTTP2_Frame));
  rst_stream->error_code = HTTP2_NO_ERROR;
#ifdef _http2_debug_
  printf("  Send RST_STREAM for %d\n", stream_ind);
#endif /* _http2_debug_ */
  HttpSend((const char*)http2_buf,
           sizeof(HTTP2_Frame) + sizeof(HTTP2_Frame_Rst_Stream));

  free(http2_buf);

  // set the current stream as closed
  http2_stream_inst->SetStreamState(stream_closed);
}

void CHttp2::send_window_update(uint_32 stream_ind,
                                uint_32 increament_window_size) {
  http2_stream* http2_stream_inst = get_stream_instance(stream_ind);

  if (http2_stream_inst &&
      (http2_stream_inst->GetStreamState() == stream_half_closed_local ||
       http2_stream_inst->GetStreamState() == stream_closed)) {
    return;
  }
  char* http2_buf =
      (char*)malloc(sizeof(HTTP2_Frame) + sizeof(HTTP2_Frame_Window_Update));
  HTTP2_Frame* http2_frm = (HTTP2_Frame*)http2_buf;

  http2_frm->length.len3b[0] = 0x00;
  http2_frm->length.len3b[1] = 0x00;
  http2_frm->length.len3b[2] = sizeof(HTTP2_Frame_Window_Update);  // length is
                                                                   // 4
  http2_frm->type = HTTP2_FRAME_TYPE_WINDOW_UPDATE;
  http2_frm->flags = HTTP2_FRAME_FLAG_UNSET;
  http2_frm->r = HTTP2_FRAME_R_UNSET;
  http2_frm->identifier = htonl(stream_ind) >> 1;

  HTTP2_Frame_Window_Update* window_update =
      (HTTP2_Frame_Window_Update*)(http2_buf + sizeof(HTTP2_Frame));
  window_update->r = 0;
  window_update->win_size = htonl(increament_window_size) >> 1;

  HttpSend((const char*)http2_buf,
           sizeof(HTTP2_Frame) + sizeof(HTTP2_Frame_Window_Update));

#ifdef _http2_debug_
  printf("  Send WINDOW_UPDATE[%d] as %d\n", stream_ind,
         increament_window_size);
#endif /* _http2_debug_ */
  free(http2_buf);

  if (http2_stream_inst) {
    http2_stream_inst->IncreaseLocalWindowSize(increament_window_size);
  } else
    m_local_control_window_size += increament_window_size;
#ifdef _http2_debug_
  printf("+  Current local Connection Window Size: %u %d\n",
         m_local_control_window_size, increament_window_size);
#endif /* _http2_debug_ */
}

void CHttp2::send_initial_window_size(uint_32 window_size) {
  char* frame_initial_window_size_buf =
      (char*)malloc(sizeof(HTTP2_Frame) + sizeof(HTTP2_Setting));

  HTTP2_Frame* initial_window_size_frame =
      (HTTP2_Frame*)frame_initial_window_size_buf;
  initial_window_size_frame->length.len3b[0] = 0x00;
  initial_window_size_frame->length.len3b[1] = 0x00;
  initial_window_size_frame->length.len3b[2] = 0x06;  // length is 6
  initial_window_size_frame->type = HTTP2_FRAME_TYPE_SETTINGS;
  initial_window_size_frame->flags = HTTP2_FRAME_FLAG_UNSET;
  initial_window_size_frame->r = HTTP2_FRAME_R_UNSET;
  initial_window_size_frame->identifier = HTTP2_FRAME_IDENTIFIER_WHOLE;

  HTTP2_Setting* initial_window_size_setting =
      (HTTP2_Setting*)(frame_initial_window_size_buf + sizeof(HTTP2_Frame));
  initial_window_size_setting->identifier =
      htons(HTTP2_SETTINGS_INITIAL_WINDOW_SIZE);
  initial_window_size_setting->value = htonl(window_size);
  HttpSend(frame_initial_window_size_buf,
           sizeof(HTTP2_Frame) + sizeof(HTTP2_Setting));
  free(frame_initial_window_size_buf);

  // Sent out, set the value to local
  m_initial_local_window_size = window_size;

#ifdef _http2_debug_
  printf("  Send INITIAL_WINDOW_SZIE as %d\n", m_initial_local_window_size);
#endif /* _http2_debug_ */
}

void CHttp2::send_push_promise_request(uint_32 stream_ind) {
  if (stream_ind > 0) {
    for (int x = 0; x < m_ch->m_http2_push_list.size(); x++) {
      PushPromise(stream_ind, m_ch->m_http2_push_list[x].path.c_str());
    }
  }

  map<uint_32, http2_stream*>::iterator it = m_stream_list.find(stream_ind);
  if (it != m_stream_list.end() && it->second) {
    for (int x = 0; x < it->second->GetHttp2PushPromiseList()->size(); x++) {
      PushPromise(stream_ind,
                  (*it->second->GetHttp2PushPromiseList())[x].c_str());
    }
  }
}

void CHttp2::send_goaway(uint_32 last_stream_ind, uint_32 error_code) {
#ifdef _http2_debug_
  printf("  Send GOAWAY for %d as %d: %s\n", last_stream_ind, error_code,
         error_code <= HTTP2_HTTP_1_1_REQUIRED ? error_table[error_code]
                                               : "Unassigned error");
#endif /* _http2_debug_ */

  char* http2_buf =
      (char*)malloc(sizeof(HTTP2_Frame) + sizeof(HTTP2_Frame_Goaway));
  HTTP2_Frame* http2_frm = (HTTP2_Frame*)http2_buf;
  // HTTP2_Frame go_away;
  http2_frm->length.len3b[0] = 0x00;
  http2_frm->length.len3b[1] = 0x00;
  http2_frm->length.len3b[2] = sizeof(HTTP2_Frame_Goaway);  // length is 8
  http2_frm->type = HTTP2_FRAME_TYPE_GOAWAY;
  http2_frm->flags = HTTP2_FRAME_FLAG_UNSET;
  http2_frm->r = HTTP2_FRAME_R_UNSET;
  http2_frm->identifier = 0;

  HTTP2_Frame_Goaway* go_away =
      (HTTP2_Frame_Goaway*)(http2_buf + sizeof(HTTP2_Frame));
  go_away->r = 0;
  go_away->last_stream_id = htonl(last_stream_ind) >> 1;
  go_away->error_code = htonl(error_code);

  HttpSend((const char*)http2_buf,
           sizeof(HTTP2_Frame) + sizeof(HTTP2_Frame_Goaway));
  free(http2_buf);
}

http2_stream* CHttp2::create_stream_instance(uint_32 stream_ind) {
  if (stream_ind > 0) {
    // send_window_update(0, HTTP2_MAX_WINDOW_SIZE);

    return new http2_stream(
        m_epoll_fd, m_backend_list, stream_ind, m_initial_local_window_size,
        m_initial_peer_window_size, this, m_connection_first_request_time,
        m_connection_keep_alive_timeout,
        m_connection_keep_alive_request_tickets, m_http_tunneling,
        m_php_fpm_instance, m_fastcgi_instances, m_scgi_instances, m_srvobj,
        m_sockfd, m_servername.c_str(), m_serverport, m_clientip.c_str(),
        m_client_cert, m_ch, m_work_path.c_str(), m_default_webpages,
        m_ext_list, m_reverse_ext_list, m_php_mode.c_str(), m_fpm_socktype,
        m_fpm_sockfile.c_str(), m_fpm_addr.c_str(), m_fpm_port,
        m_phpcgi_path.c_str(), m_cgi_list, m_private_path.c_str(),
        m_wwwauth_scheme, m_proxyauth_scheme, m_ssl);
  } else
    return NULL;
}

http2_stream* CHttp2::get_stream_instance(uint_32 stream_ind) {
  if (stream_ind > 0) {
    map<uint_32, http2_stream*>::iterator it = m_stream_list.find(stream_ind);
    if (it == m_stream_list.end()) {
      return NULL;
    } else
      return it->second;
  }
  return NULL;
}

uint_32 CHttp2::get_initial_local_window_size() {
  return m_initial_local_window_size;
}

int CHttp2::HttpSend(const char* buf, int len) {
#ifdef _WITH_ASYNC_
  return AsyncHttpSend(buf, len);
#else
  if (m_ssl)
    return SSLWrite(m_sockfd, m_ssl, buf, len,
                    CHttpBase::m_connection_idle_timeout);
  else
    return _Send_(m_sockfd, buf, len, CHttpBase::m_connection_idle_timeout);
#endif /* _WITH_ASYNC_ */
}

int CHttp2::HttpRecv(char* buf, int len) {
#ifdef _WITH_ASYNC_
  return AsyncHttpRecv(buf, len);
#else
  if (m_ssl) {
    return m_lssl->drecv(buf, len);
  } else
    return m_lsockfd->drecv(buf, len);
#endif /* _WITH_ASYNC_ */
}

int CHttp2::AsyncHttpFlush() {
  if (m_async_send_data_len > 0) {
    AsyncSend();
  }
  return m_async_send_data_len;
}

int CHttp2::AsyncSend() {
  int len = 0;
  if (m_async_send_buf && m_async_send_data_len > 0) {
    len = m_ssl ? SSL_write(m_ssl, m_async_send_buf, m_async_send_data_len)
                : send(m_sockfd, m_async_send_buf, m_async_send_data_len, 0);

    if (len > 0) {
      memmove(m_async_send_buf, m_async_send_buf + len,
              m_async_send_data_len - len);
      m_async_send_data_len -= len;

      struct epoll_event event;
      event.data.fd = m_sockfd;
      event.events = m_async_send_data_len == 0
                         ? (EPOLLIN | EPOLLHUP | EPOLLERR)
                         : EPOLLIN | EPOLLOUT | EPOLLHUP | EPOLLERR;
      epoll_ctl(m_epoll_fd, EPOLL_CTL_MOD, m_sockfd, &event);
    } else {
      if (m_ssl) {
        int ret = SSL_get_error(m_ssl, len);
        if (ret == SSL_ERROR_WANT_READ || ret == SSL_ERROR_WANT_WRITE)
          len = 0;
      } else {
        if (errno == EAGAIN)
          len = 0;
      }
    }
  }

  return len;
}

int CHttp2::AsyncRecv() {
  char buf[1024];

  int len = len =
      m_ssl ? SSL_read(m_ssl, buf, 1024) : recv(m_sockfd, buf, 1024, 0);
  ;

  if (len > 0) {
    m_async_recv_buf_size = m_async_recv_data_len + len;
    char* new_buf = (char*)malloc(m_async_recv_buf_size);

    if (m_async_recv_buf) {
      memcpy(new_buf, m_async_recv_buf, m_async_recv_data_len);
      free(m_async_recv_buf);
    }
    memcpy(new_buf + m_async_recv_data_len, buf, len);
    m_async_recv_data_len += len;
    m_async_recv_buf = new_buf;
  } else {
    if (m_ssl) {
      int ret = SSL_get_error(m_ssl, len);
      if (ret == SSL_ERROR_WANT_READ || ret == SSL_ERROR_WANT_WRITE)
        len = 0;
    } else {
      if (errno == EAGAIN)
        len = 0;
    }
  }

  return len;
}

int CHttp2::AsyncHttpSend(const char* buf, int len) {
  if (len > 0) {
    m_async_send_buf_size = m_async_send_data_len + len;
    char* new_buf = (char*)malloc(m_async_send_buf_size);

    if (m_async_send_buf) {
      memcpy(new_buf, m_async_send_buf, m_async_send_data_len);
      free(m_async_send_buf);
    }
    memcpy(new_buf + m_async_send_data_len, buf, len);
    m_async_send_data_len += len;
    m_async_send_buf = new_buf;

    AsyncSend();
  }

  return len;
}

int CHttp2::AsyncHttpRecv(char* buf, int len) {
  int min_len = 0;
  if (m_async_recv_buf && m_async_recv_data_len > 0) {
    min_len = len < m_async_recv_data_len ? len : m_async_recv_data_len;
    memcpy(buf, m_async_recv_buf, min_len);
    if (min_len > 0) {
      memmove(m_async_recv_buf, m_async_recv_buf + min_len,
              m_async_recv_data_len - min_len);
      m_async_recv_data_len -= min_len;
    }
  }

  return min_len;
}

int CHttp2::ProtParse(const HTTP2_Frame& frame_hdr,
                      char* payload,
                      uint_32 payload_len,
                      uint_32 stream_ind) {
  if (stream_ind > 0) {
    map<uint_32, http2_stream*>::iterator curr_it =
        m_stream_list.find(stream_ind);

    if (curr_it == m_stream_list.end() || curr_it->second == NULL) {
      if (m_stream_list.size() > 0 &&
          m_stream_list.size() >= m_max_concurrent_streams) {
        map<uint_32, http2_stream*>::iterator oldest_one =
            m_stream_list.begin();

        for (map<uint_32, http2_stream*>::iterator each_one =
                 m_stream_list.begin();
             each_one != m_stream_list.end(); ++each_one) {
          if (each_one->second &&
              (each_one->second->GetStreamState() == stream_closed ||
               oldest_one->second->GetLastUsedTime() >
                   each_one->second->GetLastUsedTime())) {
            oldest_one = each_one;
          } else if (!each_one->second) {
            oldest_one = each_one;
            break;
          }
        }
        send_rst_stream(oldest_one->first);

        if (oldest_one->second)
          delete oldest_one->second;
        m_stream_list.erase(oldest_one);
      }

      std::pair<map<uint_32, http2_stream*>::iterator, bool> new_stream_ret =
          m_stream_list.insert(map<uint_32, http2_stream*>::value_type(
              stream_ind, create_stream_instance(stream_ind)));
      if (!new_stream_ret.second || !new_stream_ret.first->second) {
        send_goaway(stream_ind, HTTP2_PROTOCOL_ERROR);
        return -1;
      }
    }

    http2_stream* http2_stream_inst = get_stream_instance(stream_ind);

    http2_stream_inst->RefreshLastUsedTime();

    stream_state_e state = http2_stream_inst->GetStreamState();
#ifdef _http2_debug_
    printf("  State: {%s}\n", stream_state_desc[state]);
#endif /* _http2_debug_ */

    if (state == stream_idle && frame_hdr.type != HTTP2_FRAME_TYPE_HEADERS &&
        frame_hdr.type != HTTP2_FRAME_TYPE_PRIORITY) {
      send_goaway(stream_ind, HTTP2_PROTOCOL_ERROR);
      goto END_SESSION;
    } else if (state == stream_reserved_local &&
               frame_hdr.type != HTTP2_FRAME_TYPE_RST_STREAM &&
               frame_hdr.type != HTTP2_FRAME_TYPE_PRIORITY &&
               frame_hdr.type != HTTP2_FRAME_TYPE_WINDOW_UPDATE) {
      send_goaway(stream_ind, HTTP2_STREAM_CLOSED);
      goto END_SESSION;
    } else if (state == stream_half_closed_remote &&
               frame_hdr.type != HTTP2_FRAME_TYPE_RST_STREAM &&
               frame_hdr.type != HTTP2_FRAME_TYPE_PRIORITY &&
               frame_hdr.type != HTTP2_FRAME_TYPE_WINDOW_UPDATE) {
      send_goaway(stream_ind, HTTP2_STREAM_CLOSED);
      goto END_SESSION;
    } else if (state == stream_closed &&
               frame_hdr.type != HTTP2_FRAME_TYPE_PRIORITY &&
               frame_hdr.type != HTTP2_FRAME_TYPE_WINDOW_UPDATE &&
               frame_hdr.type != HTTP2_FRAME_TYPE_RST_STREAM) {
      send_goaway(stream_ind, HTTP2_STREAM_CLOSED);
      goto END_SESSION;
    }
  }

  if (frame_hdr.type == HTTP2_FRAME_TYPE_GOAWAY) {
    HTTP2_Frame_Goaway* frm_goaway = (HTTP2_Frame_Goaway*)payload;
    uint_32 last_stream_id = frm_goaway->last_stream_id;
    last_stream_id = ntohl(last_stream_id << 1) & 0x7FFFFFFFU;
    uint_32 error_code = frm_goaway->error_code;
    error_code = ntohl(error_code);
#ifdef _http2_debug_
    printf("  Recieved HTTP2_FRAME_TYPE_GOAWAY(%s) on Last Stream(%u)\n",
           error_table[error_code], last_stream_id);

    for (int c = 0; c < payload_len - sizeof(HTTP2_Frame_Goaway); c++) {
      if (c == 0)
        printf("  Reason: ");
      printf("%c", frm_goaway->debug_data[c]);
      if (c == payload_len - sizeof(HTTP2_Frame_Goaway) - 1)
        printf("\n");
    }

#endif /* _http2_debug_ */
    goto END_SESSION;
  } else if (frame_hdr.type == HTTP2_FRAME_TYPE_PRIORITY) {
    if (stream_ind == 0) {
      send_goaway(stream_ind, HTTP2_PROTOCOL_ERROR);
      goto END_SESSION;
    }

    http2_stream* http2_stream_inst = get_stream_instance(stream_ind);

    if (!http2_stream_inst) {
      goto END_SESSION;
    }

    HTTP2_Frame_Priority* prority = (HTTP2_Frame_Priority*)payload;
    uint_32 dep_ind = ntohl(prority->dependency << 1) & 0x7FFFFFFFU;
    http2_stream_inst->SetDependencyStream(dep_ind);
    http2_stream_inst->SetPriorityWeight(prority->weight);
    if (prority->e == HTTP2_STREAM_DEPENDENCY_E_SET && dep_ind != 0) {
      for (map<uint_32, http2_stream*>::iterator each_it =
               m_stream_list.begin();
           each_it != m_stream_list.end(); ++each_it) {
        if (each_it->second &&
            each_it->second->GetDependencyStream() == dep_ind) {
          each_it->second->SetDependencyStream(stream_ind);
        }
      }
    }
#ifdef _http2_debug_
    printf("  Recv PRIORITY for %d as Weight %d depends on stream %u\n",
           stream_ind, prority->weight, dep_ind);
#endif /* _http2_debug_ */

  } else if (frame_hdr.type == HTTP2_FRAME_TYPE_HEADERS) {
    if (stream_ind > 0) {
      map<uint_32, http2_stream*>::iterator curr_it =
          m_stream_list.find(stream_ind);

      if (curr_it == m_stream_list.end() || curr_it->second == NULL ||
          curr_it->second->GetStreamState() == stream_closed) {
        if (curr_it != m_stream_list.end() && curr_it->second) {
          delete curr_it->second;
          m_stream_list.erase(curr_it);
        }

        std::pair<map<uint_32, http2_stream*>::iterator, bool> new_stream_ret =
            m_stream_list.insert(map<uint_32, http2_stream*>::value_type(
                stream_ind, create_stream_instance(stream_ind)));
        if (!new_stream_ret.second || !new_stream_ret.first->second) {
          send_goaway(stream_ind, HTTP2_PROTOCOL_ERROR);
          return -1;
        }
      }
    }

    uint_32 padding_len = 0;
    uint_32 dep_ind = 0;
    uint_8 weight;
    int offset = 0;
    if ((frame_hdr.flags & HTTP2_FRAME_FLAG_PADDED) ==
        HTTP2_FRAME_FLAG_PADDED) {
#ifdef _http2_debug_
      printf("  There's some >> PAD <<!\n");
#endif /* _http2_debug_ */
      HTTP2_Frame_Header_Pad* header1 = (HTTP2_Frame_Header_Pad*)payload;
      padding_len = header1->pad_length;
      offset += sizeof(HTTP2_Frame_Header_Pad);
    }

    if ((frame_hdr.flags & HTTP2_FRAME_FLAG_PRIORITY) ==
        HTTP2_FRAME_FLAG_PRIORITY) {
      HTTP2_Frame_Header_Weight* header_weight =
          (HTTP2_Frame_Header_Weight*)(payload + offset);
      dep_ind = ntohl(header_weight->dependency << 1) & 0x7FFFFFFFU;
      weight = header_weight->weight;
      offset += sizeof(HTTP2_Frame_Header_Weight);

      http2_stream* http2_stream_inst = get_stream_instance(stream_ind);

      if (stream_ind > 0 && http2_stream_inst) {
        http2_stream_inst->SetDependencyStream(dep_ind);
        http2_stream_inst->SetPriorityWeight(weight);
        if (header_weight->e == HTTP2_STREAM_DEPENDENCY_E_SET && dep_ind != 0) {
          for (map<uint_32, http2_stream*>::iterator each_it =
                   m_stream_list.begin();
               each_it != m_stream_list.end(); ++each_it) {
            if (each_it->second->GetDependencyStream() == dep_ind) {
              each_it->second->SetDependencyStream(stream_ind);
            }
          }
        }
#ifdef _http2_debug_
        printf(
            "  There's >> PRIORITY <<, Weight: %d, e: %d, depends on stream "
            "%u\n",
            weight, header_weight->e, dep_ind);
#endif /* _http2_debug_ */
      }
    }

    int fragment_len = payload_len - offset - padding_len;

    HTTP2_Frame_Header_Fragment* header3 =
        (HTTP2_Frame_Header_Fragment*)(payload + offset);

    http2_stream* http2_stream_inst = get_stream_instance(stream_ind);

    if (stream_ind > 0 && http2_stream_inst) {
      http2_stream_inst->SetStreamState(stream_open);

      if (http2_stream_inst->hpack_parse(
              (HTTP2_Header_Field*)header3->block_fragment, fragment_len) < 0) {
        send_goaway(stream_ind, HTTP2_COMPRESSION_ERROR);
        goto END_SESSION;
      }

      if ((frame_hdr.flags & HTTP2_FRAME_FLAG_END_HEADERS) ==
          HTTP2_FRAME_FLAG_END_HEADERS) {
#ifdef _http2_debug_
        printf("  Recieved HTTP2_FRAME_FLAG_END_HEADERS\n");
#endif /* _http2_debug_ */
        TransHttp2ParseHttp1Header(stream_ind, http2_stream_inst->GetHpack());
        http2_stream_inst->ClearHpack();
      }

      if ((frame_hdr.flags & HTTP2_FRAME_FLAG_END_STREAM) ==
          HTTP2_FRAME_FLAG_END_STREAM) {
#ifdef _http2_debug_
        printf(
            "  Recieved HTTP2_FRAME_FLAG_END_STREAM. Close Stream(%d). Line: "
            "%d\n",
            stream_ind, __LINE__);
        printf("  State Before: {%s}\n",
               stream_state_desc[http2_stream_inst->GetStreamState()]);
#endif /* _http2_debug_ */
        if (http2_stream_inst->GetStreamState() != stream_half_closed_local) {
          http2_stream_inst->SetStreamState(stream_half_closed_remote);
        } else {
          http2_stream_inst->SetStreamState(stream_closed);
        }
#ifdef _http2_debug_
        printf("  State After: {%s}\n",
               stream_state_desc[http2_stream_inst->GetStreamState()]);
#endif /* _http2_debug_ */
      }
    }
  } else if (frame_hdr.type == HTTP2_FRAME_TYPE_CONTINUATION) {
    HTTP2_Frame_Continuation* continuation =
        (HTTP2_Frame_Continuation*)(payload);

    int fragment_len = payload_len;

    http2_stream* http2_stream_inst = get_stream_instance(stream_ind);

    if (!http2_stream_inst) {
      send_goaway(stream_ind, HTTP2_PROTOCOL_ERROR);
      goto END_SESSION;
    } else {
      if (http2_stream_inst->hpack_parse(
              (HTTP2_Header_Field*)continuation->block_fragment, fragment_len) <
          0) {
        send_goaway(stream_ind, HTTP2_COMPRESSION_ERROR);
        goto END_SESSION;
      }
      if ((frame_hdr.flags & HTTP2_FRAME_FLAG_END_HEADERS) ==
          HTTP2_FRAME_FLAG_END_HEADERS) {
#ifdef _http2_debug_
        printf("  Recieved HTTP2_FRAME_FLAG_END_HEADERS\n");
#endif /* _http2_debug_ */
        TransHttp2ParseHttp1Header(stream_ind, http2_stream_inst->GetHpack());
        http2_stream_inst->ClearHpack();
      }
    }
  } else if (frame_hdr.type ==
             HTTP2_FRAME_TYPE_PUSH_PROMISE)  // PUSH_PROMISE wouldn't been sent
                                             // by the client
  {
    send_goaway(stream_ind, HTTP2_PROTOCOL_ERROR);
    goto END_SESSION;
  } else if (frame_hdr.type == HTTP2_FRAME_TYPE_SETTINGS) {
    HTTP2_Setting* client_setting = (HTTP2_Setting*)payload;
    if ((frame_hdr.flags & HTTP2_FRAME_FLAG_SETTING_ACK) !=
        HTTP2_FRAME_FLAG_SETTING_ACK)  // non setting ack
    {
      uint_16 identifier = ntohs(client_setting->identifier);
      uint_32 value = ntohl(client_setting->value);
      switch (identifier) {
        case HTTP2_SETTINGS_HEADER_TABLE_SIZE:
          m_header_table_size = value;
#ifdef _http2_debug_
          printf("  Recieved HTTP2_SETTINGS_HEADER_TABLE_SIZE %d\n", value);
#endif /* _http2_debug_ */
          break;
        case HTTP2_SETTINGS_ENABLE_PUSH:
          m_enable_push = (value == 0 ? FALSE : TRUE);
#ifdef _http2_debug_
          printf("  Recieved HTTP2_SETTINGS_ENABLE_PUSH %d\n", value);
#endif /* _http2_debug_ */
          break;
        case HTTP2_SETTINGS_MAX_CONCURRENT_STREAMS:
          m_max_concurrent_streams = value;
#ifdef _http2_debug_
          printf("  Recieved HTTP2_SETTINGS_MAX_CONCURRENT_STREAMS %d\n",
                 value);
#endif /* _http2_debug_ */
          break;
        case HTTP2_SETTINGS_INITIAL_WINDOW_SIZE:
          m_initial_peer_window_size = value;
#ifdef _http2_debug_
          printf("  Recieved HTTP2_SETTINGS_INITIAL_WINDOW_SIZE %d\n", value);
#endif /* _http2_debug_ */
          break;
        case HTTP2_SETTINGS_MAX_FRAME_SIZE:
          m_max_frame_size = value;
#ifdef _http2_debug_
          printf("  Recieved HTTP2_SETTINGS_MAX_FRAME_SIZE %d\n", value);
#endif /* _http2_debug_ */
          break;
        case HTTP2_SETTINGS_MAX_HEADER_LIST_SIZE:
          m_max_header_list_size = value;
#ifdef _http2_debug_
          printf("  Recieved HTTP2_SETTINGS_MAX_HEADER_LIST_SIZE %d\n", value);
#endif /* _http2_debug_ */
          break;
        default:
          break;
      }
#ifdef _http2_debug_
      printf("  Send Ack for %s = %d\n", setting_table[identifier], value);
#endif /* _http2_debug_ */
      send_setting_ack(ntohl(frame_hdr.identifier << 1) & 0x7FFFFFFFU);
    } else {
#ifdef _http2_debug_
      printf("  This is a SETTING Ack\n");
#endif /* _http2_debug_ */
    }
  } else if (frame_hdr.type == HTTP2_FRAME_TYPE_DATA) {
    http2_stream* http2_stream_inst = get_stream_instance(stream_ind);

    m_local_control_window_size -= payload_len;
    if (http2_stream_inst) {
      http2_stream_inst->DecreaseLocalWindowSize(payload_len);
    }
#ifdef _http2_debug_
    printf("    Current Local Window Size: [0]: %d; [%d]: %d\n",
           m_local_control_window_size, stream_ind,
           http2_stream_inst ? http2_stream_inst->GetLocalWindowSize()
                             : m_local_control_window_size);
#endif /* _http2_debug_ */
    /*if(m_local_control_window_size < 1024 && m_initial_local_window_size >
    m_local_control_window_size) send_window_update(0,
    m_initial_local_window_size - m_local_control_window_size);
    if(http2_stream_inst && http2_stream_inst->GetLocalWindowSize() < 1024 &&
    m_initial_local_window_size > http2_stream_inst->GetLocalWindowSize())
        send_window_update(stream_ind, m_initial_local_window_size -
    http2_stream_inst->GetLocalWindowSize());*/

    // send_window_update(0, payload_len + 1);
    send_window_update(stream_ind, payload_len);

    if (stream_ind == 0 ||
        (http2_stream_inst &&
         http2_stream_inst->GetStreamState() != stream_open)) {
      send_goaway(stream_ind, HTTP2_STREAM_CLOSED);
      goto END_SESSION;
    }

    if (http2_stream_inst) {
      int offset = 0;
      uint_32 padding_len = 0;
      if ((frame_hdr.flags & HTTP2_FRAME_FLAG_PADDED) ==
          HTTP2_FRAME_FLAG_PADDED) {
        HTTP2_Frame_Data1* data1 = (HTTP2_Frame_Data1*)payload;
        padding_len = data1->pad_length;
        offset += sizeof(HTTP2_Frame_Data1);

#ifdef _http2_debug_
        printf("  Recv Post Data: %d\n", payload_len - padding_len);
#endif /* _http2_debug_ */
        if (!http2_stream_inst->PushPostData(data1->data_padding,
                                             payload_len - padding_len))
          http2_stream_inst->Response();

      } else {
        HTTP2_Frame_Data2* data2 = (HTTP2_Frame_Data2*)payload;
        offset += sizeof(HTTP2_Frame_Data2);
#ifdef _http2_debug_
        printf("  Recv Post Data: %d\n", payload_len);
#endif /* _http2_debug_ */

        if (!http2_stream_inst->PushPostData(data2->data, payload_len))
          http2_stream_inst->Response();
      }

      if ((frame_hdr.flags & HTTP2_FRAME_FLAG_END_STREAM) ==
          HTTP2_FRAME_FLAG_END_STREAM) {
#ifdef _http2_debug_
        printf(
            "  Recieved HTTP2_FRAME_FLAG_END_STREAM. Close Stream(%d): Line "
            "%d\n",
            stream_ind, __LINE__);
        printf("  State: {%s}\n",
               stream_state_desc[http2_stream_inst->GetStreamState()]);
#endif /* _http2_debug_ */
        if (stream_ind == 0 ||
            http2_stream_inst->GetStreamState() != stream_half_closed_local) {
          http2_stream_inst->SetStreamState(stream_half_closed_remote);
        } else {
          http2_stream_inst->SetStreamState(stream_closed);
        }
#ifdef _http2_debug_
        printf("  State: {%s}\n",
               stream_state_desc[http2_stream_inst->GetStreamState()]);
#endif /* _http2_debug_ */
      }
    }
  } else if (frame_hdr.type == HTTP2_FRAME_TYPE_WINDOW_UPDATE) {
    HTTP2_Frame_Window_Update* win_update = (HTTP2_Frame_Window_Update*)payload;
    uint_32 win_size = ntohl(win_update->win_size << 1) & 0x7FFFFFFFU;
    if (win_size == 0) {
      send_goaway(stream_ind, HTTP2_PROTOCOL_ERROR);
      goto CONTINUE_SESSION;

    } else {
      if (stream_ind == 0) {
        m_peer_control_window_size += win_size;
#ifdef _http2_debug_
        printf("+  Current Peer Connection Window Size: %u %d\n",
               m_peer_control_window_size, win_size);
#endif /* _http2_debug_ */
      } else {
        http2_stream* http2_stream_inst = get_stream_instance(stream_ind);
        if (http2_stream_inst) {
          http2_stream_inst->IncreasePeerWindowSize(win_size);
#ifdef _http2_debug_
          printf("+  Current Peer Stream[%u] Window Size: %u %d\n", stream_ind,
                 http2_stream_inst->GetPeerWindowSize(), win_size);
#endif /* _http2_debug_ */
        }
      }
    }
  } else if (frame_hdr.type == HTTP2_FRAME_TYPE_RST_STREAM) {
    http2_stream* http2_stream_inst = get_stream_instance(stream_ind);

    if (stream_ind > 0 && http2_stream_inst) {
      if (http2_stream_inst->GetStreamState() == stream_idle) {
        send_goaway(stream_ind, HTTP2_PROTOCOL_ERROR);
      } else {
        http2_stream_inst->SetStreamState(stream_closed);

        HTTP2_Frame_Rst_Stream* rst_stream = (HTTP2_Frame_Rst_Stream*)payload;
        map<uint_32, http2_stream*>::iterator it_hpack =
            m_stream_list.find(stream_ind);
        if (it_hpack != m_stream_list.end()) {
          it_hpack->second->SetStreamState(stream_closed);
          m_stream_list.erase(it_hpack);
#ifdef _http2_debug_
          printf("  Reset HTTP2 Stream(%u) for %s\n", stream_ind,
                 ntohl(rst_stream->error_code) <= HTTP2_HTTP_1_1_REQUIRED
                     ? error_table[ntohl(rst_stream->error_code)]
                     : "Unassigned error");
#endif /* _http2_debug_ */
          if (rst_stream->error_code > HTTP2_NO_ERROR)
            send_goaway(stream_ind, ntohl(rst_stream->error_code));
        }
      }
    } else {
      send_goaway(stream_ind, HTTP2_PROTOCOL_ERROR);
    }
  } else if (frame_hdr.type == HTTP2_FRAME_TYPE_PING) {
    if ((frame_hdr.flags & HTTP2_FRAME_FLAG_PING_ACK) !=
        HTTP2_FRAME_FLAG_PING_ACK)  // non setting ack
    {
      HTTP2_Frame* ping_ack =
          (HTTP2_Frame*)malloc(sizeof(HTTP2_Frame) + sizeof(HTTP2_Frame_Ping));
      ping_ack->length.len3b[0] = 0x00;
      ping_ack->length.len3b[1] = 0x00;
      ping_ack->length.len3b[2] = sizeof(HTTP2_Frame_Ping);  // length is 8
      ping_ack->type = HTTP2_FRAME_TYPE_PING;
      ping_ack->flags = HTTP2_FRAME_FLAG_PING_ACK;
      ping_ack->r = HTTP2_FRAME_R_UNSET;
      ping_ack->identifier = frame_hdr.identifier;

      HTTP2_Frame_Ping* ping_ack_frm =
          (HTTP2_Frame_Ping*)((char*)ping_ack + sizeof(HTTP2_Frame));
      HTTP2_Frame_Ping* ping_frm = (HTTP2_Frame_Ping*)payload;
      memcpy(ping_ack_frm->data, ping_frm->data, sizeof(HTTP2_Frame_Ping));
#ifdef _http2_debug_
      printf(
          "  Send PING Ack with payload[%02X %02X %02X %02X %02X %02X %02X "
          "%02X]\n",
          ping_frm[0], ping_frm[1], ping_frm[2], ping_frm[3], ping_frm[4],
          ping_frm[5], ping_frm[6], ping_frm[7]);
#endif /* _http2_debug_ */
      HttpSend((const char*)ping_ack,
               sizeof(HTTP2_Frame) + sizeof(HTTP2_Frame_Ping));
      free(ping_ack);
    } else {
#ifdef _http2_debug_
      printf("  This is a PING Ack\n");
#endif /* _http2_debug_ */
    }
  }
CONTINUE_SESSION:
  return payload_len;

END_SESSION:
  return -1;
}

int CHttp2::ProtRecv() {
  HTTP2_Frame frame_hdr;  // = (HTTP2_Frame*)malloc(sizeof(HTTP2_Frame));
  memset(&frame_hdr, 0, sizeof(HTTP2_Frame));

#ifdef _http2_debug_
  printf("\n\n>>>> Waiting the comming frame...\n");
#endif /* _http2_debug_ */
  char* payload = NULL;
  int ret = HttpRecv((char*)&frame_hdr, sizeof(HTTP2_Frame));
  if (ret == sizeof(HTTP2_Frame)) {
    uint_32 payload_len = frame_hdr.length.len24;
    payload_len = ntohl(payload_len << 8) & 0x00FFFFFFU;
    uint_32 stream_ind = frame_hdr.identifier;

    stream_ind = ntohl(stream_ind << 1) & 0x7FFFFFFFU;

#ifdef _http2_debug_
    printf(">>>> FRAME(%03d): length(%05d) type(%s)\n", stream_ind, payload_len,
           frame_names[frame_hdr.type]);
#endif /* _http2_debug_ */

    if (payload_len > 0) {
      payload = (char*)malloc(payload_len + 1);
      memset(payload, 0, payload_len + 1);
      ret = HttpRecv(payload, payload_len);
      if (ret != payload_len)
        goto END_SESSION;
    }

    ret = ProtParse(frame_hdr, payload, payload_len, stream_ind);

    if (payload)
      free(payload);

    return ret;
  }
END_SESSION:

  if (payload)
    free(payload);
  return -1;
}

Http_Connection CHttp2::Processing() {
  Http_Connection httpConn = httpKeepAlive;
  while (1) {
    int result = ProtRecv();
    if (result <= 0) {
      httpConn = httpClose;
      break;
    }
  }
  return httpConn;
}

Http_Connection CHttp2::AsyncProcessing() {
  Http_Connection httpConn = httpContinue;

  while (1) {
    int recv_len = AsyncRecv();
    if (recv_len == 0) {
      httpConn = httpContinue;
      break;
    } else if (recv_len < 0) {
      httpConn = httpClose;
      break;
    }
  }

  if (m_http2_state == http2Prefix) {
    if (m_async_recv_data_len >= HTTP2_PREFACE_LEN) {
      memset(m_preface, 0, HTTP2_PREFACE_LEN);
      int ret = HttpRecv(m_preface, HTTP2_PREFACE_LEN);
      m_preface[HTTP2_PREFACE_LEN] = '\0';
#ifdef _http2_debug_
      printf("[%s]\n", m_preface);
#endif /* _http2_debug_ */

      char client_preface_str[] = {0x50, 0x52, 0x49, 0x20, 0x2a, 0x20, 0x48,
                                   0x54, 0x54, 0x50, 0x2f, 0x32, 0x2e, 0x30,
                                   0x0d, 0x0a, 0x0d, 0x0a, 0x53, 0x4d, 0x0d,
                                   0x0a, 0x0d, 0x0a, 0x00};
      if (strcmp(client_preface_str, m_preface) != 0) {
        send_goaway(0, HTTP2_PROTOCOL_ERROR);
        return httpClose;
      }

      char* server_preface =
          (char*)malloc(sizeof(HTTP2_Frame) + sizeof(HTTP2_Setting));

      HTTP2_Frame* preface_frame = (HTTP2_Frame*)server_preface;
      preface_frame->length.len3b[0] = 0x00;
      preface_frame->length.len3b[1] = 0x00;
      preface_frame->length.len3b[2] = 0x06;  // length is 6
      preface_frame->type = HTTP2_FRAME_TYPE_SETTINGS;
      preface_frame->flags = HTTP2_FRAME_FLAG_UNSET;
      preface_frame->r = HTTP2_FRAME_R_UNSET;
      preface_frame->identifier = HTTP2_FRAME_IDENTIFIER_WHOLE;

      HTTP2_Setting* preface_setting =
          (HTTP2_Setting*)(server_preface + sizeof(HTTP2_Frame));
      preface_setting->identifier =
          htons(HTTP2_SETTINGS_MAX_CONCURRENT_STREAMS);
      preface_setting->value = htonl(m_max_concurrent_streams);
      HttpSend(server_preface, sizeof(HTTP2_Frame) + sizeof(HTTP2_Setting));
      free(server_preface);

      send_initial_window_size(m_initial_local_window_size);

      m_http2_state = http2Header;
    }

    return httpContinue;

  } else {
    char* new_payload1 = new char[m_payload_valid_len + m_async_recv_data_len];
    memcpy(new_payload1, m_payload, m_payload_valid_len);
    memcpy(new_payload1 + m_payload_valid_len, m_async_recv_buf,
           m_async_recv_data_len);
    if (m_payload)
      delete[] m_payload;
    m_payload = new_payload1;
    m_payload_valid_len += m_async_recv_data_len;
    m_async_recv_data_len = 0;

    if (m_http2_state == http2Header) {
      uint_32 copy_len =
          m_payload_valid_len > (sizeof(HTTP2_Frame) - m_frame_hdr_valid_len)
              ? (sizeof(HTTP2_Frame) - m_frame_hdr_valid_len)
              : m_payload_valid_len;
      memcpy(m_frame_hdr + m_frame_hdr_valid_len, m_payload, copy_len);
      m_frame_hdr_valid_len += copy_len;
      if (m_frame_hdr_valid_len >= sizeof(HTTP2_Frame))
        m_http2_state = http2Payload;
      if (copy_len < m_payload_valid_len) {
        char* new_payload2 = new char[m_payload_valid_len - copy_len];
        memcpy(new_payload2, m_payload + copy_len,
               m_payload_valid_len - copy_len);
        if (m_payload)
          delete[] m_payload;
        m_payload = new_payload2;
      }
      m_payload_valid_len -= copy_len;
    }

    if (m_http2_state == http2Payload) {
      uint_32 payload_len = m_frame_hdr->length.len24;
      payload_len = ntohl(payload_len << 8) & 0x00FFFFFFU;
      uint_32 stream_ind = m_frame_hdr->identifier;

      stream_ind = ntohl(stream_ind << 1) & 0x7FFFFFFFU;

#ifdef _http2_debug_
      printf(">>>> FRAME(%03d): length(%05d) type(%s)\n", stream_ind,
             payload_len, frame_names[m_frame_hdr->type]);
#endif /* _http2_debug_ */

      if (m_payload_valid_len >= payload_len) {
        if (ProtParse(*m_frame_hdr, m_payload, payload_len, stream_ind) == -1) {
          httpConn = httpClose;
        }

        if (payload_len > 0) {
          char* new_payload3 = new char[m_payload_valid_len - payload_len];
          memcpy(new_payload3, m_payload + payload_len,
                 m_payload_valid_len - payload_len);
          if (m_payload)
            delete[] m_payload;
          m_payload = new_payload3;
          m_payload_valid_len -= payload_len;
        }
        m_http2_state = http2Header;
        m_frame_hdr_valid_len = 0;
        if (m_payload_valid_len >= sizeof(HTTP2_Frame)) {
          AsyncProcessing();
        }
      }
    }
  }
  return httpConn;
}

void CHttp2::TransHttp2ParseHttp1Header(uint_32 stream_ind, hpack* hdr) {
  http2_stream* http2_stream_inst = get_stream_instance(stream_ind);

  if (!http2_stream_inst)
    return;

  string str_line;
  for (int x = 0; x < hdr->m_decoded_headers.size(); x++) {
    BOOL found_it = FALSE;
    map<int, pair<string, string> >::iterator it;
    if (hdr->m_decoded_headers[x].index > 0) {
      if (hdr->m_decoded_headers[x].index > m_header_static_table.size()) {
        it = m_header_dynamic_table.find(hdr->m_decoded_headers[x].index);
        if (it != m_header_dynamic_table.end()) {
          found_it = TRUE;
          hdr->m_decoded_headers[x].name = it->second.first.c_str();
          if (hdr->m_decoded_headers[x].index_type == type_indexed)
            hdr->m_decoded_headers[x].value = it->second.second.c_str();
        }
      } else {
        it = m_header_static_table.find(hdr->m_decoded_headers[x].index);
        if (it != m_header_static_table.end()) {
          found_it = TRUE;
          hdr->m_decoded_headers[x].name = it->second.first.c_str();
          if (hdr->m_decoded_headers[x].index_type == type_indexed)
            hdr->m_decoded_headers[x].value = it->second.second.c_str();
        }
      }
    }

    if (found_it) {
      if (strcasecmp(it->second.first.c_str(), ":method") == 0) {
        if (hdr->m_decoded_headers[x].index_type == type_indexed)
          http2_stream_inst->m_method = it->second.second.c_str();
        else if (hdr->m_decoded_headers[x].index_type ==
                     type_with_indexing_indexed_name ||
                 hdr->m_decoded_headers[x].index_type ==
                     type_without_indexing_indexed_name ||
                 hdr->m_decoded_headers[x].index_type ==
                     type_never_indexed_indexed_name)
          http2_stream_inst->m_method = hdr->m_decoded_headers[x].value.c_str();
        if (http2_stream_inst->m_method != "" &&
            http2_stream_inst->m_path != "") {
          str_line = http2_stream_inst->m_method;
          str_line += " ";
          str_line += http2_stream_inst->m_path;
          str_line += " HTTP/1.1\r\n";

          http2_stream_inst->http1_parse(str_line.c_str());
        }
      } else if (strcasecmp(it->second.first.c_str(), ":scheme") == 0) {
        http2_stream_inst->m_scheme = hdr->m_decoded_headers[x].value.c_str();
      } else if (strcasecmp(it->second.first.c_str(), ":path") == 0) {
        if (hdr->m_decoded_headers[x].index_type == type_indexed)
          http2_stream_inst->m_path = it->second.second.c_str();
        else if (hdr->m_decoded_headers[x].index_type ==
                     type_with_indexing_indexed_name ||
                 hdr->m_decoded_headers[x].index_type ==
                     type_without_indexing_indexed_name ||
                 hdr->m_decoded_headers[x].index_type ==
                     type_never_indexed_indexed_name)
          http2_stream_inst->m_path = hdr->m_decoded_headers[x].value.c_str();

        if (http2_stream_inst->m_method != "" &&
            http2_stream_inst->m_path != "") {
          str_line = http2_stream_inst->m_method;
          str_line += " ";
          str_line += http2_stream_inst->m_path;
          str_line += " HTTP/1.1\r\n";

          http2_stream_inst->http1_parse(str_line.c_str());
        }
      } else if (strcasecmp(it->second.first.c_str(), ":authority") == 0) {
        if (hdr->m_decoded_headers[x].index_type == type_indexed)
          http2_stream_inst->m_authority = it->second.second.c_str();
        else if (hdr->m_decoded_headers[x].index_type ==
                     type_with_indexing_indexed_name ||
                 hdr->m_decoded_headers[x].index_type ==
                     type_without_indexing_indexed_name ||
                 hdr->m_decoded_headers[x].index_type ==
                     type_never_indexed_indexed_name)
          http2_stream_inst->m_authority =
              hdr->m_decoded_headers[x].value.c_str();
      } else {
        str_line = it->second.first.c_str();
        str_line += ": ";
        if (hdr->m_decoded_headers[x].index_type == type_indexed)
          str_line += it->second.second.c_str();
        else if (hdr->m_decoded_headers[x].index_type ==
                     type_with_indexing_indexed_name ||
                 hdr->m_decoded_headers[x].index_type ==
                     type_without_indexing_indexed_name ||
                 hdr->m_decoded_headers[x].index_type ==
                     type_never_indexed_indexed_name)
          str_line += hdr->m_decoded_headers[x].value.c_str();
        str_line += "\r\n";
        http2_stream_inst->http1_parse(str_line.c_str());
      }
    } else {
      if (strcasecmp(hdr->m_decoded_headers[x].name.c_str(), ":method") == 0) {
        http2_stream_inst->m_method = hdr->m_decoded_headers[x].value.c_str();

        if (http2_stream_inst->m_method != "" &&
            http2_stream_inst->m_path != "") {
          str_line = http2_stream_inst->m_method;
          str_line += " ";
          str_line += http2_stream_inst->m_path;
          str_line += " HTTP/1.1\r\n";

          http2_stream_inst->http1_parse(str_line.c_str());
        }
      } else if (strcasecmp(hdr->m_decoded_headers[x].name.c_str(),
                            ":scheme") == 0) {
        http2_stream_inst->m_scheme = hdr->m_decoded_headers[x].value.c_str();
      } else if (strcasecmp(hdr->m_decoded_headers[x].name.c_str(), ":path") ==
                 0) {
        http2_stream_inst->m_path = hdr->m_decoded_headers[x].value.c_str();

        if (http2_stream_inst->m_method != "" &&
            http2_stream_inst->m_path != "") {
          str_line = http2_stream_inst->m_method;
          str_line += " ";
          str_line += http2_stream_inst->m_path;
          str_line += " HTTP/1.1\r\n";

          http2_stream_inst->http1_parse(str_line.c_str());
        }
      } else if (strcasecmp(hdr->m_decoded_headers[x].name.c_str(),
                            ":authority") == 0) {
        http2_stream_inst->m_authority =
            hdr->m_decoded_headers[x].value.c_str();
      } else {
        if (strcmp(hdr->m_decoded_headers[x].name.c_str(), "") != 0 &&
            strcmp(hdr->m_decoded_headers[x].value.c_str(), "") != 0) {
          str_line = hdr->m_decoded_headers[x].name.c_str();
          str_line += ": ";
          str_line += hdr->m_decoded_headers[x].value.c_str();
          str_line += "\r\n";
          http2_stream_inst->http1_parse(str_line.c_str());
        }
      }
    }

    int stable_size = m_header_static_table.size();

    if (hdr->m_decoded_headers[x].index_type ==
            type_with_indexing_indexed_name ||
        hdr->m_decoded_headers[x].index_type == type_with_indexing_new_name) {
      if (strcmp(hdr->m_decoded_headers[x].name.c_str(), "") != 0) {
        // default table element size is 4096 (not octets here)
        int dtable_size = m_header_dynamic_table.size();

        for (int y = (dtable_size > 4095 ? 4095 : dtable_size); y >= 1; y--) {
          usleep(100);
          map<int, pair<string, string> >::iterator it_y;
          it_y = m_header_dynamic_table.find(stable_size + y);
          if (it_y != m_header_dynamic_table.end()) {
            m_header_dynamic_table[stable_size + y + 1] =
                m_header_dynamic_table[stable_size + y];
          }
        }
        m_header_dynamic_table[stable_size + 1] = make_pair(
            hdr->m_decoded_headers[x].name, hdr->m_decoded_headers[x].value);
      }
    }
  }

  if (m_enable_push && !m_pushed_request) {
    m_pushed_request = TRUE;
    send_push_promise_request(stream_ind);
    m_pushed_response = FALSE;
  }

  if (http2_stream_inst->GetMethod() != hmPost) {
    http2_stream_inst->Response();
  }
}

int CHttp2::TransHttp1SendHttp2Header(
    uint_32 stream_ind,
    const char* buf,
    int len,
    uint_8 frame_type,
    uint_32 promised_stream_ind /* for push_promise frame */) {
  if (stream_ind == 0 || (frame_type != HTTP2_FRAME_TYPE_PUSH_PROMISE &&
                          frame_type != HTTP2_FRAME_TYPE_HEADERS &&
                          frame_type != HTTP2_FRAME_TYPE_CONTINUATION))
    return -1;

  http2_stream* http2_stream_inst = get_stream_instance(stream_ind);

  if (!http2_stream_inst)
    return -1;

  hpack hk;
  hk.build(buf, len, m_header_static_table);

  int max_buf_size =
      sizeof(HTTP2_Frame) * 2     /*One is for HEAD empty DATA frame*/
      + sizeof(HTTP2_Frame_Data2) /*For HEAD empty DATA*/
      + sizeof(HTTP2_Frame_Push_Promise_Without_Pad) +
      sizeof(HTTP2_Frame_Continuation) + hk.get_length();

  // allocate max size buf.
  char* response_buf = (char*)malloc(max_buf_size);

  HTTP2_Frame* http2_frm_hdr = (HTTP2_Frame*)response_buf;
  uint_32 response_len = sizeof(HTTP2_Frame);
  uint_32 response_buff_size = max_buf_size;

  if (frame_type == HTTP2_FRAME_TYPE_PUSH_PROMISE) {
    HTTP2_Frame_Push_Promise_Without_Pad* push_promise_frm =
        (HTTP2_Frame_Push_Promise_Without_Pad*)((char*)response_buf +
                                                response_len);
    push_promise_frm->r = HTTP2_FRAME_R_UNSET;
    push_promise_frm->promised_stream_ind = htonl(promised_stream_ind) >> 1;
    response_len += sizeof(HTTP2_Frame_Push_Promise_Without_Pad);
  }

  memcpy(response_buf + response_len, hk.get_field(), hk.get_length());

  response_len += hk.get_length();

  uint_32 frame_len = response_len - sizeof(HTTP2_Frame);

  http2_frm_hdr->length.len24 = htonl(frame_len) >> 8;

  http2_frm_hdr->type = frame_type;

  http2_frm_hdr->flags = HTTP2_FRAME_FLAG_END_HEADERS;
  http2_frm_hdr->r = HTTP2_FRAME_R_UNSET;

  http2_frm_hdr->identifier = htonl(stream_ind) >> 1;

  if (http2_stream_inst->GetMethod() == hmHead) {
    // Send a empty DATA for HEAD
    HTTP2_Frame* http2_frm_data = (HTTP2_Frame*)(response_buf + response_len);
    response_len += sizeof(HTTP2_Frame);

    http2_frm_data->length.len24 = 0;
    http2_frm_data->type = HTTP2_FRAME_TYPE_DATA;
    http2_frm_data->flags = HTTP2_FRAME_FLAG_UNSET;
    http2_frm_data->r = HTTP2_FRAME_R_UNSET;
    http2_frm_data->identifier = htonl(stream_ind) >> 1;
  }
  int ret = HttpSend(response_buf, response_len);

#ifdef _http2_debug_
  printf("  Send DATA on stream %u @TransHttp1SendHttp2Header\n", stream_ind);
#endif /* _http2_debug_ */

  http2_stream_inst->SetStreamState(stream_open);
  if (response_buf)
    free(response_buf);
  return ret;
}

int CHttp2::TransHttp1SendHttp2Content(uint_32 stream_ind,
                                       const char* buf,
                                       uint_32 len) {
  http2_stream* http2_stream_inst = get_stream_instance(stream_ind);

  if (!http2_stream_inst)
    return -1;

  int ret = 0;
  int sent_len = 0;
  while (1) {
    int response_len = 0;
    char* response_buf = NULL;
    uint_32 pre_send_len = 0;

    if (m_max_frame_size >
        (HTTP2_DATA_PADDING_LEN + sizeof(HTTP2_Frame_Data1))) {
      pre_send_len =
          (len - sent_len) > (m_max_frame_size - HTTP2_DATA_PADDING_LEN -
                              sizeof(HTTP2_Frame_Data1))
              ? (m_max_frame_size - HTTP2_DATA_PADDING_LEN -
                 sizeof(HTTP2_Frame_Data1))
              : (len - sent_len);

      pre_send_len = http2_stream_inst->GetPeerWindowSize() > pre_send_len
                         ? pre_send_len
                         : http2_stream_inst->GetPeerWindowSize();
      int curr_padding_len =
          HTTP2_DATA_PADDING_LEN - pre_send_len % HTTP2_DATA_PADDING_LEN;

      response_len = sizeof(HTTP2_Frame) + sizeof(HTTP2_Frame_Data1) +
                     pre_send_len + curr_padding_len;
      response_buf = (char*)malloc(response_len);
      memset(response_buf, 0, response_len);
      HTTP2_Frame* http2_frm_data = (HTTP2_Frame*)response_buf;
      http2_frm_data->length.len24 =
          htonl(response_len - sizeof(HTTP2_Frame)) >> 8;
      http2_frm_data->type = HTTP2_FRAME_TYPE_DATA;
      http2_frm_data->flags = HTTP2_FRAME_FLAG_PADDED;
      http2_frm_data->r = HTTP2_FRAME_R_UNSET;

      http2_frm_data->identifier = htonl(stream_ind) >> 1;

      HTTP2_Frame_Data1* padding_data =
          (HTTP2_Frame_Data1*)(response_buf + sizeof(HTTP2_Frame));
      padding_data->pad_length = curr_padding_len;
      memcpy(padding_data->data_padding, buf + sent_len, pre_send_len);
#ifdef _http2_debug_
      printf(
          "  Send DATA Frame as Length: %d/%d/%d on stream %u "
          "@TransHttp1SendHttp2Content LEN: %d with  PADDING %d\n",
          pre_send_len, http2_stream_inst->GetPeerWindowSize(), len, stream_ind,
          response_len - sizeof(HTTP2_Frame), curr_padding_len);
#endif /* _http2_debug_ */

    } else {
      pre_send_len = (len - sent_len) > m_max_frame_size ? m_max_frame_size
                                                         : (len - sent_len);

      pre_send_len = http2_stream_inst->GetPeerWindowSize() > pre_send_len
                         ? pre_send_len
                         : http2_stream_inst->GetPeerWindowSize();

      response_buf = (char*)malloc(sizeof(HTTP2_Frame) +
                                   sizeof(HTTP2_Frame_Data2) + pre_send_len);
      response_len =
          sizeof(HTTP2_Frame) + sizeof(HTTP2_Frame_Data2) + pre_send_len;
      HTTP2_Frame* http2_frm_data = (HTTP2_Frame*)response_buf;
      http2_frm_data->length.len24 = htonl(pre_send_len) >> 8;
      http2_frm_data->type = HTTP2_FRAME_TYPE_DATA;
      http2_frm_data->flags = HTTP2_FRAME_FLAG_UNSET;
      http2_frm_data->r = HTTP2_FRAME_R_UNSET;

      http2_frm_data->identifier = htonl(stream_ind) >> 1;

      HTTP2_Frame_Data2* data =
          (HTTP2_Frame_Data2*)(response_buf + sizeof(HTTP2_Frame));
      memcpy(data->data, buf + sent_len, pre_send_len);

#ifdef _http2_debug_
      printf(
          "  Send DATA Frame as Length: %d/%d/%d on stream %u "
          "@TransHttp1SendHttp2Content LEN: %d without PADDING\n",
          pre_send_len, http2_stream_inst->GetPeerWindowSize(), len, stream_ind,
          response_len - sizeof(HTTP2_Frame));
#endif /* _http2_debug_ */
    }
    ret = HttpSend(response_buf, response_len);

    if (response_buf)
      free(response_buf);

    if (ret != -1) {
#ifdef _http2_debug_
      printf("    Remote WIN[%d]: %d\n", stream_ind,
             http2_stream_inst ? http2_stream_inst->GetPeerWindowSize()
                               : m_peer_control_window_size);
#endif /* _http2_debug_ */

      sent_len += pre_send_len;
      http2_stream_inst->DecreasePeerWindowSize(pre_send_len);
      m_peer_control_window_size -= pre_send_len;
    } else
      break;
    if (sent_len == len)
      break;

    if (http2_stream_inst->GetPeerWindowSize() == 0) {
#ifdef _WITH_ASYNC_
      AsyncProcessing();
#else
      ProtRecv();
#endif /* _WITH_ASYNC_ */
    }
  }
  return ret;
}

int CHttp2::SendHttp2EmptyContent(uint_32 stream_ind, uint_8 flags) {
  http2_stream* http2_stream_inst = get_stream_instance(stream_ind);

  if (!http2_stream_inst)
    return -1;
  int curr_padding_len =
      (HTTP2_DATA_PADDING_LEN + sizeof(HTTP2_Frame_Data1)) > m_max_frame_size
          ? (m_max_frame_size - sizeof(HTTP2_Frame_Data1))
          : HTTP2_DATA_PADDING_LEN;
  int response_len =
      sizeof(HTTP2_Frame) + sizeof(HTTP2_Frame_Data1) + curr_padding_len;
  char* response_buf = (char*)malloc(response_len);
  memset(response_buf, 0, response_len);
  HTTP2_Frame* http2_frm_data = (HTTP2_Frame*)response_buf;
  http2_frm_data->length.len24 = htonl(response_len - sizeof(HTTP2_Frame)) >> 8;
  ;
  http2_frm_data->type = HTTP2_FRAME_TYPE_DATA;
  http2_frm_data->flags = flags;
  http2_frm_data->r = HTTP2_FRAME_R_UNSET;

  http2_frm_data->identifier = htonl(stream_ind) >> 1;

  HTTP2_Frame_Data1* padding_data =
      (HTTP2_Frame_Data1*)(response_buf + sizeof(HTTP2_Frame));
  padding_data->pad_length = curr_padding_len;
  int ret = HttpSend(response_buf, response_len);

#ifdef _http2_debug_
  printf("  Send DATA Frame on stream %u @SendHttp2EmptyContent LEN: %d\n",
         stream_ind, response_len - sizeof(HTTP2_Frame));
#endif /* _http2_debug_ */

  if ((http2_frm_data->flags & HTTP2_FRAME_FLAG_END_STREAM) ==
      HTTP2_FRAME_FLAG_END_STREAM) {
    if (http2_stream_inst &&
        http2_stream_inst->GetStreamState() != stream_half_closed_remote)
      http2_stream_inst->SetStreamState(stream_half_closed_local);
    else
      http2_stream_inst->SetStreamState(stream_closed);
  }

  if (response_buf)
    free(response_buf);
  return ret;
}

void CHttp2::SendHttp2PushPromiseResponse() {
  if (m_enable_push && m_pushed_request == TRUE && m_pushed_response == FALSE) {
    m_pushed_response = TRUE;
    for (map<uint_32, http2_stream*>::iterator it = m_stream_list.begin();
         it != m_stream_list.end(); ++it) {
      if (it->first % 2 == 0)  // Streams initiated by the server MUST use
                               // even-numbered stream identifiers
      {
        it->second->SendPushPromiseResponse();
      }
    }
  }
}

void CHttp2::PushPromise(uint_32 stream_ind, const char* path) {
  if (!m_enable_push)
    return;

  http2_stream* http2_stream_inst = get_stream_instance(stream_ind);

  if (!http2_stream_inst)
    return;

  HTTP2_Frame_Push_Promise_Without_Pad push_promise_frm;
  uint_32 promised_stream_ind = 0;
  for (uint_32 i = stream_ind / 2 * 2 + 2; i < (0xFFFFFFFFU >> 1); i += 2) {
    map<uint_32, http2_stream*>::iterator it = m_stream_list.find(i);
    if (it == m_stream_list.end() || it->second == NULL) {
      promised_stream_ind = i;
      break;
    }
  }

  std::pair<map<uint_32, http2_stream*>::iterator, bool>
      new_promised_stream_ret =
          m_stream_list.insert(map<uint_32, http2_stream*>::value_type(
              promised_stream_ind,
              create_stream_instance(promised_stream_ind)));

  if (!new_promised_stream_ret.second ||
      !new_promised_stream_ret.first->second) {
    send_goaway(stream_ind, HTTP2_PROTOCOL_ERROR);
    return;
  }
  new_promised_stream_ret.first->second->SetStreamState(stream_reserved_local);

  string str_http1_pseudo_header;

  str_http1_pseudo_header = ":authority: ";
  str_http1_pseudo_header += http2_stream_inst->m_authority;
  str_http1_pseudo_header += "\r\n";

  str_http1_pseudo_header += ":method: GET\r\n";

  str_http1_pseudo_header += ":path: ";
  str_http1_pseudo_header += path;
  str_http1_pseudo_header += "\r\n";

  str_http1_pseudo_header += ":scheme: https\r\n";

  if (strcmp(http2_stream_inst->GetHttp1()->GetRequestField("Accept"), "") !=
      0) {
    str_http1_pseudo_header += "accept: ";
    str_http1_pseudo_header +=
        http2_stream_inst->GetHttp1()->GetRequestField("Accept");
    str_http1_pseudo_header += "\r\n";
  }

  if (strcmp(http2_stream_inst->GetHttp1()->GetRequestField("Accept-Encoding"),
             "") != 0) {
    str_http1_pseudo_header += "accept-encoding: ";
    str_http1_pseudo_header +=
        http2_stream_inst->GetHttp1()->GetRequestField("Accept-Encoding");
    str_http1_pseudo_header += "\r\n";
  }

  if (strcmp(http2_stream_inst->GetHttp1()->GetRequestField("Accept-Language"),
             "") != 0) {
    str_http1_pseudo_header += "accept-language: ";
    str_http1_pseudo_header +=
        http2_stream_inst->GetHttp1()->GetRequestField("Accept-Language");
    str_http1_pseudo_header += "\r\n";
  }

  if (strcmp(http2_stream_inst->GetHttp1()->GetRequestField("User-Agent"),
             "") != 0) {
    str_http1_pseudo_header += "user-agent: ";
    str_http1_pseudo_header +=
        http2_stream_inst->GetHttp1()->GetRequestField("User-Agent");
    str_http1_pseudo_header += "\r\n";
  }
  TransHttp1SendHttp2Header(stream_ind, str_http1_pseudo_header.c_str(),
                            str_http1_pseudo_header.length(),
                            HTTP2_FRAME_TYPE_PUSH_PROMISE, promised_stream_ind);
#ifdef _http2_debug_
  printf("  Send PUSH_PROMISE, promised stream(%d): %s\n", promised_stream_ind,
         path);
#endif /* _http2_debug_ */

  new_promised_stream_ret.first->second->BuildPushPromiseResponse(
      http2_stream_inst, path);
}

void CHttp2::init_header_table() {
  m_header_static_table[1] = make_pair(string(":authority"), string(""));
  m_header_static_table[2] = make_pair(string(":method"), string("GET"));
  m_header_static_table[3] = make_pair(string(":method"), string("POST"));
  m_header_static_table[4] = make_pair(string(":path"), string("/"));
  m_header_static_table[5] = make_pair(string(":path"), string("/index.html"));
  m_header_static_table[6] = make_pair(string(":scheme"), string("http"));
  m_header_static_table[7] = make_pair(string(":scheme"), string("https"));
  m_header_static_table[8] = make_pair(string(":status"), string("200"));
  m_header_static_table[9] = make_pair(string(":status"), string("204"));
  m_header_static_table[10] = make_pair(string(":status"), string("206"));
  m_header_static_table[11] = make_pair(string(":status"), string("304"));
  m_header_static_table[12] = make_pair(string(":status"), string("400"));
  m_header_static_table[13] = make_pair(string(":status"), string("404"));
  m_header_static_table[14] = make_pair(string(":status"), string("500"));
  m_header_static_table[15] = make_pair(string("accept-charset"), string(""));
  m_header_static_table[16] =
      make_pair(string("accept-encoding"), string("gzip, deflate"));
  m_header_static_table[17] = make_pair(string("accept-language"), string(""));
  m_header_static_table[18] = make_pair(string("accept-ranges"), string(""));
  m_header_static_table[19] = make_pair(string("accept"), string(""));
  m_header_static_table[20] =
      make_pair(string("access-control-allow-origin"), string(""));
  m_header_static_table[21] = make_pair(string("age"), string(""));
  m_header_static_table[22] = make_pair(string("allow"), string(""));
  m_header_static_table[23] = make_pair(string("authorization"), string(""));
  m_header_static_table[24] = make_pair(string("cache-control"), string(""));
  m_header_static_table[25] =
      make_pair(string("content-disposition"), string(""));
  m_header_static_table[26] = make_pair(string("content-encoding"), string(""));
  m_header_static_table[27] = make_pair(string("content-language"), string(""));
  m_header_static_table[28] = make_pair(string("content-length"), string(""));
  m_header_static_table[29] = make_pair(string("content-location"), string(""));
  m_header_static_table[30] = make_pair(string("content-range"), string(""));
  m_header_static_table[31] = make_pair(string("content-type"), string(""));
  m_header_static_table[32] = make_pair(string("cookie"), string(""));
  m_header_static_table[33] = make_pair(string("date"), string(""));
  m_header_static_table[34] = make_pair(string("etag"), string(""));
  m_header_static_table[35] = make_pair(string("expect"), string(""));
  m_header_static_table[36] = make_pair(string("expires"), string(""));
  m_header_static_table[37] = make_pair(string("from"), string(""));
  m_header_static_table[38] = make_pair(string("host"), string(""));
  m_header_static_table[39] = make_pair(string("if-match"), string(""));
  m_header_static_table[40] =
      make_pair(string("if-modified-since"), string(""));
  m_header_static_table[41] = make_pair(string("if-none-match"), string(""));
  m_header_static_table[42] = make_pair(string("if-range"), string(""));
  m_header_static_table[43] =
      make_pair(string("if-unmodified-since"), string(""));
  m_header_static_table[44] = make_pair(string("last-modified"), string(""));
  m_header_static_table[45] = make_pair(string("link"), string(""));
  m_header_static_table[46] = make_pair(string("location"), string(""));
  m_header_static_table[47] = make_pair(string("max-forwards"), string(""));
  m_header_static_table[48] =
      make_pair(string("proxy-authenticate"), string(""));
  m_header_static_table[49] =
      make_pair(string("proxy-authorization"), string(""));
  m_header_static_table[50] = make_pair(string("range"), string(""));
  m_header_static_table[51] = make_pair(string("referer"), string(""));
  m_header_static_table[52] = make_pair(string("refresh"), string(""));
  m_header_static_table[53] = make_pair(string("retry-after"), string(""));
  m_header_static_table[54] = make_pair(string("server"), string(""));
  m_header_static_table[55] = make_pair(string("set-cookie"), string(""));
  m_header_static_table[56] =
      make_pair(string("strict-transport-security"), string(""));
  m_header_static_table[57] =
      make_pair(string("transfer-encoding"), string(""));
  m_header_static_table[58] = make_pair(string("user-agent"), string(""));
  m_header_static_table[59] = make_pair(string("vary"), string(""));
  m_header_static_table[60] = make_pair(string("via"), string(""));
  m_header_static_table[61] = make_pair(string("www-authenticate"), string(""));
}
