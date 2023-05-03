/*
        Copyright (c) openheap, uplusware
        uplusware@gmail.com
*/

#ifndef _WEB_SOCKET_H_
#define _WEB_SOCKET_H_

/*
 WebSocket Base Framing Protocol
      0               1               2               3
      0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7
     +-+-+-+-+-------+-+-------------+-------------------------------+
     |F|R|R|R| opcode|M| Payload len |    Extended payload length    |
     |I|S|S|S|  (4)  |A|     (7)     |             (16/64)           |
     |N|V|V|V|       |S|             |   (if payload len==126/127)   |
     | |1|2|3|       |K|             |                               |
     +-+-+-+-+-------+-+-------------+ - - - - - - - - - - - - - - - +
     |     Extended payload length continued, if payload len == 127  |
     + - - - - - - - - - - - - - - - +-------------------------------+
     |                               |Masking-key, if MASK set to 1  |
     +-------------------------------+-------------------------------+
     | Masking-key (continued)       |          Payload Data         |
     +-------------------------------- - - - - - - - - - - - - - - - +
     :                     Payload Data continued ...                :
     + - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - +
     |                     Payload Data continued ...                |
     +---------------------------------------------------------------+

*/

#include <openssl/err.h>
#include <openssl/ssl.h>
#include <unistd.h>

#define OPCODE_CONTINUE 0x00
#define OPCODE_TEXT 0x01
#define OPCODE_BINARY 0x02

#define OPCODE_CLOSE 0x08
#define OPCODE_PING 0x09
#define OPCODE_PONG 0x0A

#pragma pack(push)
#pragma pack(1)

typedef struct {
  union {
    struct {
      unsigned char payload_len_7bit : 7;
      unsigned char mask : 1;
      unsigned char opcode : 4;
      unsigned char rsv3 : 1;
      unsigned char rsv2 : 1;
      unsigned char rsv1 : 1;
      unsigned char fin : 1;
    } hstruct;
    unsigned short huint16;
  } h;
} WebSocket_Base_Header;

#pragma pack(pop)

typedef struct {
  WebSocket_Base_Header base_hdr;
  unsigned long long payload_len;
  unsigned char masking_key[4];
  unsigned char* payload_buf;
} WebSocket_Base_Framing;

typedef struct {
  char* buf;
  unsigned long long len;
  int opcode;
  bool mask;
} WebSocket_Buffer;

int WebSocket_Buffer_Alloc(WebSocket_Buffer* data,
                           int len = 0,
                           bool mask = false,
                           int opcode = OPCODE_TEXT);
int WebSocket_Buffer_Free(WebSocket_Buffer* data);

class WebSocket {
 public:
  WebSocket(int sockfd, SSL* ssl) {
    m_sockfd = sockfd;
    m_ssl = ssl;
  }

  virtual ~WebSocket() {
    if (m_sockfd >= 0)
      close(m_sockfd);

    m_sockfd = -1;
  }

  int Send(WebSocket_Buffer* data);
  int Recv(WebSocket_Buffer* data);
  int Close();
  int Ping();
  int Pong();

 private:
  int m_sockfd;
  SSL* m_ssl;
};

#endif /* _WEB_SOCKET_H_ */