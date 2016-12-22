/*
	Copyright (c) openheap, uplusware
	uplusware@gmail.com
*/

#ifndef _FASTCGI_H_
#define _FASTCGI_H_

#include "cgi.h"

#define FCGI_VERSION_1 1

//type
#define FCGI_BEGIN_REQUEST 1
#define FCGI_ABORT_REQUEST 2
#define FCGI_END_REQUEST 3
#define FCGI_PARAMS 4
#define FCGI_STDIN 5
#define FCGI_STDOUT 6
#define FCGI_STDERR 7
#define FCGI_DATA 8
#define FCGI_GET_VALUES 9
#define FCGI_GET_VALUES_RESULT 10
#define FCGI_UNKNOWN_TYPE 11

#define FCGI_MAX_CONNS  "FCGI_MAX_CONNS"
#define FCGI_MAX_REQS   "FCGI_MAX_REQS"
#define FCGI_MPXS_CONNS "FCGI_MPXS_CONNS"

//protocolStatus
#define FCGI_REQUEST_COMPLETE 0
#define FCGI_CANT_MPX_CONN 1
#define FCGI_OVERLOADED 2
#define FCGI_UNKNOWN_ROLE 3
#define FCGI_UNKNOWN_STATUS	0xFF

//Response content
typedef struct {
     unsigned char appStatusB3;
     unsigned char appStatusB2;
     unsigned char appStatusB1;
     unsigned char appStatusB0;
     unsigned char protocolStatus;
     unsigned char reserved[3];
} FCGI_EndRequestBody;

//Request content
//Role
#define FCGI_RESPONDER 1
#define FCGI_AUTHORIZER 2
#define FCGI_FILTER 3

typedef struct {
     unsigned char roleB1;
     unsigned char roleB0;
     unsigned char flags;
     unsigned char reserved[5];
} FCGI_BeginRequestBody;

typedef struct {
     unsigned char version;
     unsigned char type;
     unsigned char requestIdB1;
     unsigned char requestIdB0;
     unsigned char contentLengthB1;
     unsigned char contentLengthB0;
     unsigned char paddingLength;
     unsigned char reserved;
} FCGI_Header;

typedef struct {
     FCGI_Header header;
     unsigned char* contentData;
	 unsigned long long contentLength;
     unsigned char* paddingData;
	 unsigned long long paddingLength;
} FCGI_Record;


typedef struct {
	unsigned char nameLengthB0; /* nameLengthB0 >> 7 == 0 */
	unsigned char valueLengthB0; /* valueLengthB0 >> 7 == 0 */
	/*unsigned char nameData[nameLength];
	unsigned char valueData[valueLength]; */
} FCGI_NameValuePair11;

typedef struct {
	unsigned char nameLengthB0; /* nameLengthB0 >> 7 == 0 */
	unsigned char valueLengthB3; /* valueLengthB3 >> 7 == 1 */
	unsigned char valueLengthB2;
	unsigned char valueLengthB1;
	unsigned char valueLengthB0;
	/*unsigned char nameData[nameLength];
	unsigned char valueData[valueLength((B3 & 0x7f) << 24) + (B2 << 16) + (B1 << 8) + B0];*/
} FCGI_NameValuePair14;
typedef struct {
	unsigned char nameLengthB3; /* nameLengthB3 >> 7 == 1 */
	unsigned char nameLengthB2;
	unsigned char nameLengthB1;
	unsigned char nameLengthB0;
	unsigned char valueLengthB0; /* valueLengthB0 >> 7 == 0 */
	/*
	unsigned char nameData[nameLength((B3 & 0x7f) << 24) + (B2 << 16) + (B1 << 8) + B0];
	unsigned char valueData[valueLength]; */
} FCGI_NameValuePair41;
typedef struct {
	unsigned char nameLengthB3; /* nameLengthB3 >> 7 == 1 */
	unsigned char nameLengthB2;
	unsigned char nameLengthB1;
	unsigned char nameLengthB0;
	unsigned char valueLengthB3; /* valueLengthB3 >> 7 == 1 */
	unsigned char valueLengthB2;
	unsigned char valueLengthB1;
	unsigned char valueLengthB0;
	/*
	unsigned char nameData[nameLength((B3 & 0x7f) << 24) + (B2 << 16) + (B1 << 8) + B0];
	unsigned char valueData[valueLength((B3 & 0x7f) << 24) + (B2 << 16) + (B1 << 8) + B0];*/
} FCGI_NameValuePair44;

class FastCGI : public cgi_base
{
public:
	FastCGI(const char* ipaddr, unsigned short port);
	FastCGI(const char* sock_file);
	virtual ~FastCGI();
	
	int BeginRequest(unsigned short request_id);
	int SendParams(map<string, string> &params_map);
	int SendParams(const char* name, unsigned int name_len, const char* value, unsigned int value_len);
	int SendEmptyParams();
	int Send_STDIN(const char* inbuf, unsigned long inbuf_len);
	int SendEmpty_STDIN();
	int RecvAppData(vector<char>& binaryResponse, string& errout, unsigned int& appstatus, unsigned char& protocolstatus,
        BOOL& continue_recv);
	int AbortRequest();
	int EndRequest(unsigned int app_status, unsigned char protocol_status);
	int GetAppValue(const char* name, string& value);
private:
	unsigned char m_RequestIDB0;
	unsigned char m_RequestIDB1;
};

#endif /* _FASTCGI_H_ */

