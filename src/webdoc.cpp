/*
	Copyright (c) openheap, uplusware
	uplusware@gmail.com
*/
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include "webdoc.h"
#include "util/md5.h"

typedef struct content_range
{
    long long beg;
    long long end;
    long long len;
} HTTP_CONTENT_RANGE;

bool get_file_etag(const char* path, unsigned char etag[33], char* filebuf = NULL)
{
    char * pbuf = filebuf;
    ifstream ifsResource(path, ios_base::binary);
    string strline;
    if(!ifsResource.is_open())
    {
        return false;
    }
    else
    {
        unsigned char HA[16];
        MD5_CTX_OBJ Md5Ctx;
        Md5Ctx.MD5Init();
        char rbuf[1449];
        while(1)
        {
            if(ifsResource.eof())
            {
                break;
            }
            if(ifsResource.read(rbuf, 1448) < 0)
            {
                break;
            }
            int rlen = ifsResource.gcount();
            if(filebuf != NULL)
            {
                memcpy(pbuf, rbuf, rlen);
                pbuf += rlen;
            }
            Md5Ctx.MD5Update((unsigned char*) rbuf, rlen);
        }
        Md5Ctx.MD5Final(HA);
        sprintf((char*)etag, "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x", 
            HA[0], HA[1], HA[2], HA[3], HA[4], HA[5], HA[6], HA[7], 
            HA[8], HA[9], HA[10], HA[11], HA[12], HA[13], HA[14], HA[15]);
        
    }
    if(ifsResource.is_open())
        ifsResource.close();
    return true;
}

bool get_file_length_lastmidifytime(const char* path, long long & length, time_t & lastmodifytime)
{
    
    struct stat statbuf;
    if( stat(path, &statbuf) < 0)
        return false;
    
    lastmodifytime = statbuf.st_mtime;
    length = statbuf.st_size;
    return true;
}

void doc::Response()
{
	string strExtName;
	string strResourceFullPath;
	string strResource;
	
	strResourceFullPath = m_session->GetResource();
    
    strResourceFullPath = m_work_path.c_str();
    strResourceFullPath += "/html/";
    strResourceFullPath += m_session->GetResource();
    

    struct stat info;
    stat(strResourceFullPath.c_str(), &info);
    if(S_ISDIR(info.st_mode))
    {
        CHttpResponseHdr header;
        header.SetStatusCode(SC404);
        header.SetField("Content-Type", "text/html");
        header.SetField("Content-Length", header.GetDefaultHTMLLength());
        
        m_session->HttpSend(header.Text(), header.Length());
        m_session->HttpSend(header.GetDefaultHTML(), header.GetDefaultHTMLLength());
        return;
    }
    
    long long nResourceLength, nRangeLength = 0;
    unsigned char sz_etag[33];
    time_t lastmodifytime;
    if(get_file_length_lastmidifytime(strResourceFullPath.c_str(), nResourceLength, lastmodifytime) == false)
    {
        /* No such file */
        CHttpResponseHdr header;
        header.SetStatusCode(SC404);
        header.SetField("Content-Type", "text/html");
        header.SetField("Content-Length", header.GetDefaultHTMLLength());
        
        m_session->HttpSend(header.Text(), header.Length());
        m_session->HttpSend(header.GetDefaultHTML(), header.GetDefaultHTMLLength());
        return;
    }
    else
    {
        char* file_cache_buf = NULL;
        char sz_quota_etag_quota[FILE_ETAG_LEN + 2 + 1];
        string str_date;
        OutHTTPDateString(lastmodifytime, str_date);
        
        if(strcmp(m_session->GetRequestField("Cache-Control"), "no-cache") != 0
            && strcmp(m_session->GetRequestField("Pragma"), "no-cache") != 0
            && strcmp(m_session->GetRequestField("If-Modified-Since"), "") != 0 
            && strcmp(m_session->GetRequestField("If-Modified-Since"), str_date.c_str()) == 0)
        {
            /* No such file */
            CHttpResponseHdr header;
            header.SetStatusCode(SC304);
            unsigned int zero = 0;
            header.SetField("Content-Length", zero);    
            m_session->HttpSend(header.Text(), header.Length());
            return;
        }
        else
        {
            if(nResourceLength <= FILE_MAX_SIZE)
                file_cache_buf = (char*)malloc(nResourceLength + 1);
                
            if(get_file_etag(strResourceFullPath.c_str(), sz_etag, file_cache_buf) ==  false)
            {
                if(file_cache_buf)
                    free(file_cache_buf);
                CHttpResponseHdr header;
                header.SetStatusCode(SC404);
                header.SetField("Content-Type", "text/html");
                header.SetField("Content-Length", header.GetDefaultHTMLLength());
                
                m_session->HttpSend(header.Text(), header.Length());
                m_session->HttpSend(header.GetDefaultHTML(), header.GetDefaultHTMLLength());
                return;
            }
            
            sprintf(sz_quota_etag_quota, "\"%s\"", (char*)sz_etag);
        }
        
        if(strcmp(m_session->GetRequestField("Cache-Control"), "no-cache") != 0
            && strcmp(m_session->GetRequestField("Pragma"), "no-cache") != 0
            && strcmp(m_session->GetRequestField("If-None-Match"), "") != 0 
            && strcmp(m_session->GetRequestField("If-None-Match"), sz_quota_etag_quota) == 0)
        {
            CHttpResponseHdr header;
            header.SetStatusCode(SC304);
            unsigned int zero = 0;
            header.SetField("Content-Length", zero);    
            m_session->HttpSend(header.Text(), header.Length()); 
            return;                     
        }
        
        /* Parse ranges */
        string strContentRangeResponse;
        vector<HTTP_CONTENT_RANGE> content_ranges;
        const char* szHttpRequestRanges = m_session->GetRequestField("Range");
        int x = 0;
        bool flag = false;
        string strRange;
        while(1)
        {
            if(!flag)
            {
                if(szHttpRequestRanges[x] == '=')
                {
                    flag = true;
                }
                x++;
                continue;
            }
            if(szHttpRequestRanges[x] == ',' || szHttpRequestRanges[x] == '\0')
            {
                HTTP_CONTENT_RANGE ra;
                ra.beg = 0;
                ra.end = nResourceLength -1;
                
                string strbeg, strend;
                strcut(strRange.c_str(), NULL, "-", strbeg);
                strcut(strRange.c_str(), "-", NULL, strend);
                strtrim(strbeg);
                strtrim(strend);
                if(strbeg != "")
                {
                   ra.beg = atoll(strbeg.c_str());
                }
                if(strend != "")
                {
                   ra.end = atoll(strend.c_str());
                }
                if(ra.beg <= ra.end)
                {
                    ra.len = ra.end - ra.beg + 1;
                    
                    char szTmp[1024];
                    sprintf(szTmp, "%lld-%lld/%lld,", ra.beg, ra.end, ra.len);
                    nRangeLength += ra.len;
                    
                    if(strContentRangeResponse == "")
                        strContentRangeResponse = "bytes ";
                    strContentRangeResponse += szTmp;
                    content_ranges.push_back(ra);
                }
                
                strRange = "";
            }
            else
                strRange += (szHttpRequestRanges[x]);
            
            if(szHttpRequestRanges[x] == '\0')
                break;
            x++;
        }
        
        lowercase(m_session->GetResource(), strResource);				
        get_extend_name(strResource.c_str(), strExtName);
        
        /* Has cache buff */
        if(file_cache_buf)
        {
            CHttpResponseHdr header;
            if(strcmp(szHttpRequestRanges, "") != 0 && strContentRangeResponse != "")
                header.SetStatusCode(SC206);
            else
                header.SetStatusCode(SC200);
            header.SetField("ETag", sz_quota_etag_quota);
            header.SetField("Content-Length", nRangeLength);
            OutHTTPDateString(time(NULL) + 900, str_date);
            header.SetField("Expires", str_date.c_str());
            header.SetField("Cache-Control", "max-age=900");
            OutHTTPDateString(lastmodifytime, str_date);
            header.SetField("Last-Modified", str_date.c_str());
            
            if(m_session->m_cache->m_type_table.find(strExtName) == m_session->m_cache->m_type_table.end())
            {
                header.SetField("Content-Type", "application/*");
            }
            else
            {
                header.SetField("Content-Type", m_session->m_cache->m_type_table[strExtName].c_str());
            }
            if(strcmp(szHttpRequestRanges, "") != 0 && strContentRangeResponse != "")
            {
                header.SetField("Content-Range", strContentRangeResponse.c_str());
            }
            
            m_session->HttpSend(header.Text(), header.Length());
            
            if(strcmp(szHttpRequestRanges, "") == 0)
            {
                if(m_session->GetMethod() != hmHead)
                {
                    m_session->HttpSend(file_cache_buf, nResourceLength);
                }
            }
            else
            {
                for(int v = 0; v < content_ranges.size(); v++)
                {
                    if(m_session->GetMethod() != hmHead)
                    {
                        int rest_length = nResourceLength - content_ranges[v].beg;
                        if(rest_length <= 0)
                            continue;
                        m_session->HttpSend(file_cache_buf + content_ranges[v].beg, 
                            rest_length > content_ranges[v].len ? content_ranges[v].len : rest_length);
                    }
                }
            }
            free(file_cache_buf);
        }
        else
        {
            ifstream ifsResource(strResourceFullPath.c_str(), ios_base::binary);
            string strline;
            if(!ifsResource.is_open())
            {
                CHttpResponseHdr header;
                header.SetStatusCode(SC404);
                header.SetField("Content-Type", "text/html");
                header.SetField("Content-Length", header.GetDefaultHTMLLength());
                m_session->HttpSend(header.Text(), header.Length());
                m_session->HttpSend(header.GetDefaultHTML(), header.GetDefaultHTMLLength());
            }
            else
            {
                CHttpResponseHdr header;
                if(strcmp(szHttpRequestRanges, "") != 0 && strContentRangeResponse != "")
                    header.SetStatusCode(SC206);
                else
                    header.SetStatusCode(SC200);
                header.SetField("ETag", sz_quota_etag_quota);
                header.SetField("Content-Length", nRangeLength);
                OutHTTPDateString(time(NULL) + 900, str_date);
                header.SetField("Expires", str_date.c_str());
                header.SetField("Cache-Control", "max-age=900");
                OutHTTPDateString(lastmodifytime, str_date);
                header.SetField("Last-Modified", str_date.c_str());
                
                if(m_session->m_cache->m_type_table.find(strExtName) == m_session->m_cache->m_type_table.end())
                {
                    header.SetField("Content-Type", "application/*");
                }
                else
                {
                    header.SetField("Content-Type", m_session->m_cache->m_type_table[strExtName].c_str());
                }
                if(strcmp(szHttpRequestRanges, "") != 0 && strContentRangeResponse != "")
                {
                    header.SetField("Content-Range", strContentRangeResponse.c_str());
                }
                
                m_session->HttpSend(header.Text(), header.Length());
                
                if(strcmp(szHttpRequestRanges, "") == 0)
                {
                    if(m_session->GetMethod() != hmHead)
                    {
                        char rbuf[1449];
                        while(1)
                        {
                            if(ifsResource.eof())
                                break;
                            
                            if(ifsResource.read( rbuf, 1448) < 0)
                                break;
                            
                            int rlen = ifsResource.gcount();
                            m_session->HttpSend(rbuf, rlen);
                        }
                    }
                }
                else
                {
                    for(int v = 0; v < content_ranges.size(); v++)
                    {
                        if(m_session->GetMethod() != hmHead)
                        {
                            ifsResource.seekg(content_ranges[v].beg, ios::beg);
                            long long len_sum = 0;
                            char rbuf[1449];
                            while(1)
                            {
                                if(ifsResource.eof())
                                    break;
                                
                                if(ifsResource.read( rbuf, (content_ranges[v].len - len_sum)  > 1448 ? 1448 : (content_ranges[v].len -len_sum) ) < 0)
                                    break;
                                
                                int rlen = ifsResource.gcount();
                                len_sum += rlen;
                                m_session->HttpSend(rbuf, rlen);
                                if(len_sum == content_ranges[v].len)
                                    break;
                            }
                        }
                    }
                }
            }
            if(ifsResource.is_open())
                ifsResource.close();
        }
    }
}
