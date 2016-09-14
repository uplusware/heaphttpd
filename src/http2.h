/*
	Copyright (c) openheap, uplusware
	uplusware@gmail.com
*/

#ifndef _HTTP2_H_
#define _HTTP2_H_
#include <pthread.h>
#include "http.h"
#include "http2comm.h"
#include "hpack.h"
#include <queue>
#include <utility>
#include <map>
using namespace std;

/*
          +-------+-----------------------------+---------------+
          | Index | Header Name                 | Header Value  |
          +-------+-----------------------------+---------------+
          | 1     | :authority                  |               |
          | 2     | :method                     | GET           |
          | 3     | :method                     | POST          |
          | 4     | :path                       | /             |
          | 5     | :path                       | /index.html   |
          | 6     | :scheme                     | http          |
          | 7     | :scheme                     | https         |
          | 8     | :status                     | 200           |
          | 9     | :status                     | 204           |
          | 10    | :status                     | 206           |
          | 11    | :status                     | 304           |
          | 12    | :status                     | 400           |
          | 13    | :status                     | 404           |
          | 14    | :status                     | 500           |
          | 15    | accept-charset              |               |
          | 16    | accept-encoding             | gzip, deflate |
          | 17    | accept-language             |               |
          | 18    | accept-ranges               |               |
          | 19    | accept                      |               |
          | 20    | access-control-allow-origin |               |
          | 21    | age                         |               |
          | 22    | allow                       |               |
          | 23    | authorization               |               |
          | 24    | cache-control               |               |
          | 25    | content-disposition         |               |
          | 26    | content-encoding            |               |
          | 27    | content-language            |               |
          | 28    | content-length              |               |
          | 29    | content-location            |               |
          | 30    | content-range               |               |
          | 31    | content-type                |               |
          | 32    | cookie                      |               |
          | 33    | date                        |               |
          | 34    | etag                        |               |
          | 35    | expect                      |               |
          | 36    | expires                     |               |
          | 37    | from                        |               |
          | 38    | host                        |               |
          | 39    | if-match                    |               |
          | 40    | if-modified-since           |               |
          | 41    | if-none-match               |               |
          | 42    | if-range                    |               |
          | 43    | if-unmodified-since         |               |
          | 44    | last-modified               |               |
          | 45    | link                        |               |
          | 46    | location                    |               |
          | 47    | max-forwards                |               |
          | 48    | proxy-authenticate          |               |
          | 49    | proxy-authorization         |               |
          | 50    | range                       |               |
          | 51    | referer                     |               |
          | 52    | refresh                     |               |
          | 53    | retry-after                 |               |
          | 54    | server                      |               |
          | 55    | set-cookie                  |               |
          | 56    | strict-transport-security   |               |
          | 57    | transfer-encoding           |               |
          | 58    | user-agent                  |               |
          | 59    | vary                        |               |
          | 60    | via                         |               |
          | 61    | www-authenticate            |               |
          +-------+-----------------------------+---------------+
*/

#define HTTP2_PREFACE_LEN 24

enum http2_msg_type
{
    http2_msg_quit = 0,
    http2_msg_data
};

typedef struct
{
    http2_msg_type msg_type;
    HTTP2_Frame* frame;
    char* payload;
} http2_msg;

class CHttp;
class CHttp2
{
public:
	CHttp2(CHttp* pHttp1);
	virtual ~CHttp2();
	
    CHttp* GetHttp1() { return m_pHttp1; }
	int ProtRecv();
    Http_Connection Processing();
    
    void PushMsg(http2_msg_type type, HTTP2_Frame* frame, char* payload);
    void PopMsg(http2_msg* msg);
    
    void ParseHeaders(uint_32 stream_ind, hpack* hdr);
    
    int ParseHTTP1Header(const char* buf, int len);
    int ParseHTTP1Content(const char* buf, uint_32 len);
    
    int SendContentEOF();
    
    volatile int m_thread_running;
private:
	CHttp* m_pHttp1;
	char m_preface[HTTP2_PREFACE_LEN + 1];
	void init_header_table();
	pthread_t m_send_pthread_id;
    pthread_mutex_t m_frame_queue_mutex;
    queue<http2_msg> m_frame_queue;
	map<int, pair<string, string> > m_header_static_table;
    map<int, pair<string, string> > m_header_dynamic_table;
    
    string m_path;
    string m_method;
    string m_authority;
    string m_scheme;
    
    string m_status;
    
    uint_32 m_curr_stream_ind;
};

#endif /* _HTTP2_H_ */