/*
	Copyright (c) openheap, uplusware
	uplusware@gmail.com
*/

#include "niuapi.h"

bool __inline__ _lendian_()
{
	unsigned short s = 0xFF00;
	unsigned char* c = (unsigned char*)&s;
	if(*c == 0x00)
		return true;
	else
		return false;
}

void __inline__ _strtrim_(string &src)
{
	string::size_type ops1, ops2, ops3, ops4;
	bool bExit = false;
	while(!bExit)
	{
		bExit = true;
		ops1 = src.find_first_not_of(" ");
		if((ops1 != string::npos)&&(ops1 != 0))
		{
			src.erase(0, ops1);
			bExit = false;
		}
		else if(ops1 == string::npos)
		{
			src.erase(0, src.length());
		}
		ops2 = src.find_first_not_of("\t");
		if((ops2 != string::npos)&&(ops2 != 0))
		{
			src.erase(0, ops2);
		}
		else if(ops2 == string::npos)
		{
			src.erase(0, src.length());
		}
		ops3 = src.find_first_not_of("\r");
		if((ops3 != string::npos)&&(ops3 != 0))
		{
			src.erase(0, ops3);
		}
		else if(ops3 == string::npos)
		{
			src.erase(0, src.length());
		}
			
		ops4 = src.find_first_not_of("\n");
		if((ops4 != string::npos)&&(ops4 != 0))
		{
			src.erase(0, ops4);
		}
		else if(ops4 == string::npos)
		{
			src.erase(0, src.length());
		}
	}
	bExit = false;
	while(!bExit)
	{
		bExit = true;
		ops1 = src.find_last_not_of(" ");
		if((ops1 != string::npos)&&(ops1 != src.length() - 1 ))
		{
			src.erase(ops1 + 1, src.length() - ops1 - 1);
			bExit = false;
		}
		
		ops2 = src.find_last_not_of("\t");
		if((ops2 != string::npos)&&(ops2 != src.length() - 1 ))
		{
			src.erase(ops2 + 1, src.length() - ops2 - 1);
			bExit = false;
		}
		ops3 = src.find_last_not_of("\r");
		if((ops3 != string::npos)&&(ops3 != src.length() - 1 ))
		{
			src.erase(ops3 + 1, src.length() - ops3 - 1);
			bExit = false;
		}
			
		ops4 = src.find_last_not_of("\n");
		if((ops4 != string::npos)&&(ops4 != src.length() - 1 ))
		{
			src.erase(ops4 + 1, src.length() - ops4 - 1);
			bExit = false;
		}
	}
}

static const char*  hex[] = {
        "00","01","02","03","04","05","06","07","08","09","0A","0B","0C","0D","0E","0F",
        "10","11","12","13","14","15","16","17","18","19","1A","1B","1C","1D","1E","1F",
        "20","21","22","23","24","25","26","27","28","29","2A","2B","2C","2D","2E","2F",
        "30","31","32","33","34","35","36","37","38","39","3A","3B","3C","3D","3E","3F",
        "40","41","42","43","44","45","46","47","48","49","4A","4B","4C","4D","4E","4F",
        "50","51","52","53","54","55","56","57","58","59","5A","5B","5C","5D","5E","5F",
        "60","61","62","63","64","65","66","67","68","69","6A","6B","6C","6D","6E","6F",
        "70","71","72","73","74","75","76","77","78","79","7A","7B","7C","7D","7E","7F",
        "80","81","82","83","84","85","86","87","88","89","8A","8B","8C","8D","8E","8F",
        "90","91","92","93","94","95","96","97","98","99","9A","9B","9C","9D","9E","9F",
        "A0","A1","A2","A3","A4","A5","A6","A7","A8","A9","AA","AB","AC","AD","AE","AF",
        "B0","B1","B2","B3","B4","B5","B6","B7","B8","B9","BA","BB","BC","BD","BE","BF",
        "C0","C1","C2","C3","C4","C5","C6","C7","C8","C9","CA","CB","CC","CD","CE","CF",
        "D0","D1","D2","D3","D4","D5","D6","D7","D8","D9","DA","DB","DC","DD","DE","DF",
        "E0","E1","E2","E3","E4","E5","E6","E7","E8","E9","EA","EB","EC","ED","EE","EF",
        "F0","F1","F2","F3","F4","F5","F6","F7","F8","F9","FA","FB","FC","FD","FE","FF"
    };
static unsigned char val[] = {
        0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,
        0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,
        0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,
        0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,
        0x3F,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,
        0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,
        0x3F,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,
        0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,
        0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,
        0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,
        0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,
        0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,
        0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,
        0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,
        0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,
        0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F
    };

void NIU_URLFORMAT_ENCODE(const unsigned char* src, string & dst)
{
	dst = "";
	int slen = strlen((char*)src);
	int x = 0;
	while(x < slen)
	{
		if(src[x] == ' ')
		{
			dst += ('+');
		}
		else if((src[x] >= 'A') && (src[x] <= 'Z'))
		{
			dst += (src[x]);
		}
		else if((src[x] >= 'a') && (src[x] <= 'z'))
		{
			dst += (src[x]);
		}
		else if((src[x] >= '0') && (src[x] <= '9'))
		{
			dst += (src[x]);
		}
		else if((src[x] == '-') || (src[x] == '_') ||
		(src[x] == '.') || (src[x] == '!') ||
		(src[x] == '~') || (src[x] == '*') ||
		(src[x] == '\\') || (src[x] == '(') || (src[x] == ')'))
		{
			dst += (src[x]);
		}
		else if(src[x] <= 0x7F)
		{
			dst += ('%');
			dst += (hex[src[x]][0]);
			dst += (hex[src[x]][1]);
		}
		else
		{
			dst += ('%');
			dst += ('u');
			if(_lendian_())
			{
				dst += (hex[src[x+1]][0]);
				dst += (hex[src[x+1]][1]);
				dst += (hex[src[x]][0]);
				dst += (hex[src[x]][1]);
			}
			else
			{
				dst += (hex[src[x]][0]);
				dst += (hex[src[x]][1]);
				dst += (hex[src[x+1]][0]);
				dst += (hex[src[x+1]][1]);
			}
			x++;
		}
		x++;
	}
}

void NIU_URLFORMAT_DECODE(const unsigned char* src, string & dst)
{
	dst = "";
	int slen = strlen((char*)src);
	int x = 0;
	while( x < slen)
	{
		if(src[x] == '+')
		{
			dst += (' ');
		}
		else if((src[x] >= 'A') && (src[x] <= 'Z'))
		{
			dst += (src[x]);
		}
		else if((src[x] >= 'a') && (src[x] <= 'z'))
		{
			dst += (src[x]);
		}
		else if((src[x] >= '0') && (src[x] <= '9'))
		{
			dst += (src[x]);
		}
		else if((src[x] == '-') || (src[x] == '_') ||(src[x] == '.')||(src[x] == '!') ||
		(src[x] == '~') || (src[x] == '*') ||(src[x] == '\\') || (src[x] == '(') || (src[x] == ')'))
		{
			dst += (src[x]);
		}
		else if(src[x] == '%')
		{
			unsigned char cint = 0;
			if(src[x + 1] != 'u')
			{
				cint = (cint << 4) | val[src[x + 1]];
                cint = (cint << 4) | val[src[x + 2]];
                dst += (cint);
                x += 2;
			}
			else
			{
				if(_lendian_())
				{
					cint = (cint << 4) | val[src[x + 4]];
	                cint = (cint << 4) | val[src[x + 5]];
	                dst += (cint);
	                
					cint = (cint << 4) | val[src[x + 2]];
	                cint = (cint << 4) | val[src[x + 3]];

	                dst += (cint);
                }
                else
                {
					cint = (cint << 4) | val[src[x + 2]];
	                cint = (cint << 4) | val[src[x + 3]];

	                dst += (cint);

	                cint = (cint << 4) | val[src[x + 4]];
	                cint = (cint << 4) | val[src[x + 5]];
	                dst += (cint);

                }
                x += 5;

			}
			
		}
		else
		{
			dst += (src[x]);
		}
		x++;
	}
}

void NIU_POST_GET_VARS(const char* data, map<string, string> & varmap)
{
    char where = 'K'; /* K is for key, V is for value*/
    varmap.clear();
    string key, value, value2;
    int x = 0;
    while(1)
    {
        if(data[x] == '&' || data[x] == '\0')
        {
            _strtrim_(key);
            if(key != "")
            {
                if(value != "")
                {
                    NIU_URLFORMAT_DECODE((const unsigned char*)value.c_str(), value2);
                }
                varmap[key] = value2;
                key = "";
                value = "";
            }
            where = 'K';
            
        }
        else if(data[x] == '=')
        {
            where = 'V';
        }
        else
        {
            if( where == 'K' )
                key += (data[x]);
            else if( where == 'V' )
                value += (data[x]);
        }
        
        if(data[x] == '\0')
            break;
        x++;
    }
}

void NIU_COOKIE_VARS(const char* data, map<string, string> & varmap)
{
    char where = 'K'; /* K is for key, V is for value*/
    varmap.clear();
    string key, value;
    int x = 0;
    while(1)
    {
        if(data[x] == ';' || data[x] == '\0')
        {
             _strtrim_(key);
            if(key != "")
            {
                varmap[key] = value;
                key = "";
                value = "";
            }
            where = 'K';
            
        }
        else if(data[x] == '=')
        {
            where = 'V';
        }
        else
        {
            if( where == 'K' )
                key += (data[x]);
            else if( where == 'V' )
                value += (data[x]);
        }
        
        if(data[x] == '\0')
            break;
        x++;
    }
}
