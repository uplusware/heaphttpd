#ifndef _FORM_DATA_H_
#define _FORM_DATA_H_
#include "fstring.h"
class formparamter
{
public:
	fbufseg m_seg;
	
	fbufseg m_header;
	fbufseg	m_data;
};

class formdata
{
private:
	const char* m_buf;
	const char* m_boundary;
public:
	vector<formparamter> m_paramters;

	const char* c_buffer() { return m_buf; }
	
	formdata(const char* buf, int buflen, const char* boundary)
	{
		m_buf = buf;
		m_boundary = boundary;
		string sboundary = "--";
		sboundary += boundary;
		sboundary += "\r\n";
		
		string eboundary = "--";
		eboundary += boundary;
		eboundary += "--\r\n";
        
		formparamter parmeter;
		parmeter.m_seg.m_byte_beg = 0;
		parmeter.m_seg.m_byte_end = 0;
		parmeter.m_header.m_byte_beg = 0;
		parmeter.m_header.m_byte_end = 0;
		parmeter.m_data.m_byte_beg = 0;
		parmeter.m_data.m_byte_end = 0;
		BOOL isHeader = FALSE;
        int x = 0;
		while(x < buflen)
		{
			if((memcmp(buf + x, sboundary.c_str(), sboundary.length()) == 0) &&
                (memcmp(buf + x, eboundary.c_str(), eboundary.length()) != 0))
			{
				if(x == 0)
				{
					parmeter.m_seg.m_byte_beg = x + sboundary.length();
					parmeter.m_header.m_byte_beg = parmeter.m_seg.m_byte_beg;
				}
				else
				{
					parmeter.m_seg.m_byte_end = x - 1;
					parmeter.m_data.m_byte_end = parmeter.m_seg.m_byte_end - 2;
					
					m_paramters.push_back(parmeter);
					
					parmeter.m_seg.m_byte_beg = x + sboundary.length();
					parmeter.m_header.m_byte_beg = parmeter.m_seg.m_byte_beg;
				}
				x += sboundary.length();
				isHeader = TRUE;
			}
			else if(memcmp(buf + x, eboundary.c_str(), eboundary.length()) == 0)
			{
				parmeter.m_seg.m_byte_end = x - 1;
				parmeter.m_data.m_byte_end = parmeter.m_seg.m_byte_end - 2;	   
				m_paramters.push_back(parmeter);
				
				x += eboundary.length();
				break;
			}
			else
			{
				if(isHeader == TRUE)
				{
					if(strncmp(buf + x, "\r\n\r\n", 4) == 0)
					{
						parmeter.m_header.m_byte_end = x - 1;
						parmeter.m_data.m_byte_beg = x  + 4;
						isHeader = FALSE;
						x += 4;
					}
					else
					{
						x++;
					}
				}
				else
				{
					x++;
				}
			}
		}
	}

	virtual ~formdata()
	{

	}
};

#endif /* _FORM_DATA_H_ */