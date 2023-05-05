/*
        Copyright (c) openheap, uplusware
        uplusware@gmail.com
*/

#include "http2_client.h"

http2_client::http2_client(memory_cache* cache,
                           const char* http_url,
                           http_tunneling* tunneling)
    : http_client(cache, http_url, tunneling) {
  m_http1_1_client = new http1_1_client(cache, http_url, tunneling);

  m_frame_hdr = new HTTP2_Frame;
  m_frame_hdr_valid_len = 0;

  m_payload = NULL;
  m_payload_valid_len = 0;

  m_http2_state = http2Header;
}
http2_client::~http2_client() {
  if (m_http1_1_client)
    delete m_http1_1_client;
  if (m_frame_hdr)
    delete m_frame_hdr;
}

bool http2_client::processing(const char* buf, int buf_len) {
  char* new_payload1 = new char[m_payload_valid_len + buf_len];
  memcpy(new_payload1, m_payload, m_payload_valid_len);
  memcpy(new_payload1 + m_payload_valid_len, buf, buf_len);
  if (m_payload)
    delete[] m_payload;
  m_payload = new_payload1;
  m_payload_valid_len += buf_len;

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
      m_payload_valid_len -= copy_len;
    }
  }

  else if (m_http2_state == http2Payload) {
    uint_32 payload_len = m_frame_hdr->length.len24;
    payload_len = ntohl(payload_len << 8) & 0x00FFFFFFU;
    uint_32 stream_ind = m_frame_hdr->identifier;

    stream_ind = ntohl(stream_ind << 1) & 0x7FFFFFFFU;

#ifdef _http2_debug_
    printf(">>>> FRAME(%03d): length(%05d) type(%s)\n", stream_ind, payload_len,
           frame_names[m_frame_hdr->type]);
#endif /* _http2_debug_ */

    if (m_payload_valid_len >= payload_len) {
      m_http2_state = http2Header;
    }
  }
  return m_http1_1_client->processing(buf, buf_len);
}
