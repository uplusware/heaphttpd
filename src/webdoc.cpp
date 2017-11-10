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

bool get_file_length_lastmidifytime(const char* path, long long & length, time_t & tLastModifyTime)
{
    
    struct stat statbuf;
    if( stat(path, &statbuf) < 0)
        return false;
    
    tLastModifyTime = statbuf.st_mtime;
    length = statbuf.st_size;
    return true;
}

void doc::Response()
{
	string strExtName;
	string strResourceFullPath;
	string strResource;
	
	long long nResourceLength = 0;
	long long nRangeLength = 0;
    unsigned char szETag[33];
    time_t tLastModifyTime;
    
    string strContentRangeResponse;
    vector<HTTP_CONTENT_RANGE> content_ranges;
    const char* szHttpRequestRanges = NULL;
    int x = 0;
    bool flag = false;
    string strRange;
    
    char* file_cache_buf = NULL;
    char* file_buf = NULL;
    char szQETagQ[FILE_ETAG_LEN + 2 + 1];
    string strDateTime;    
    
    strResourceFullPath = m_work_path.c_str();
    strResourceFullPath += "/html/";
    strResourceFullPath += m_session->GetResource();
    
    FILE_CACHE_DATA* file_cache_data = NULL;
    file_cache* file_cache_instance = NULL;
    
    bool file_cache_available = false;
    m_session->m_cache->rdlock_file_cache();
    if(!m_session->RequestNoCache() && m_session->m_cache->_find_file_(strResourceFullPath.c_str(), &file_cache_instance)
        && file_cache_instance)
    {
        file_cache_data = file_cache_instance->file_rdlock();
    }
    
    m_session->m_cache->unlock_file_cache();
    
    if(file_cache_instance && file_cache_data)
    {
        time_t cache_time_out = time(NULL) - file_cache_data->t_create;
        //10s timeout
        if(cache_time_out < 10 && cache_time_out > 0)
        {
            file_cache_available = true;
        }
        else
        {
            //90s timeout
            if(get_file_length_lastmidifytime(strResourceFullPath.c_str(), nResourceLength, tLastModifyTime) == true
                && tLastModifyTime == file_cache_data->t_modify && cache_time_out < 90 && cache_time_out > 0)
            {
                file_cache_available = true;
            }
            else
            {
                file_cache_instance->file_unlock();
                file_cache_data = NULL;
                file_cache_instance = NULL;
                file_cache_available = false;
            }
        }
    }
    
    if(!file_cache_available)
    {
        struct stat info;
        stat(strResourceFullPath.c_str(), &info);
        if(S_ISDIR(info.st_mode)) /* Forbidden to access directory directly */
        {
            CHttpResponseHdr header;
            header.SetStatusCode(SC404);
            header.SetField("Content-Type", "text/html");
            header.SetField("Content-Length", header.GetDefaultHTMLLength());
            
            m_session->SendHeader(header.Text(), header.Length());
            m_session->SendContent(header.GetDefaultHTML(), header.GetDefaultHTMLLength());
            goto END_STEP1;
        }
        
        if(get_file_length_lastmidifytime(strResourceFullPath.c_str(), nResourceLength, tLastModifyTime) == false)
        {
            /* No such file */
            CHttpResponseHdr header;
            header.SetStatusCode(SC404);
            header.SetField("Content-Type", "text/html");
            header.SetField("Content-Length", header.GetDefaultHTMLLength());
            
            m_session->SendHeader(header.Text(), header.Length());
            m_session->SendContent(header.GetDefaultHTML(), header.GetDefaultHTMLLength());
            goto END_STEP1;
        }
    }
    else
    {
        nResourceLength = file_cache_data->len;
        tLastModifyTime = file_cache_data->t_modify;
    }

    OutHTTPGMTDateString(tLastModifyTime, strDateTime);
    
    if(strcmp(m_session->GetRequestField("Cache-Control"), "no-cache") != 0
        && strcmp(m_session->GetRequestField("Pragma"), "no-cache") != 0
        && strcmp(m_session->GetRequestField("If-Modified-Since"), "") != 0 
        && strcmp(m_session->GetRequestField("If-Modified-Since"), strDateTime.c_str()) == 0)
    {
        /* No such file */
        CHttpResponseHdr header;
        header.SetStatusCode(SC304);
        unsigned int zero = 0;
        header.SetField("Content-Length", zero);    
        m_session->SendHeader(header.Text(), header.Length());
        goto END_STEP1;
    }
    else
    {
    
        if(file_cache_available)
        {
            file_cache_buf = file_cache_data->buf;
            memcpy(szETag, file_cache_data->etag, FILE_ETAG_LEN + 1);
        }
        else
        {
            if(nResourceLength <= CHttpBase::m_single_localfile_cache_size)
                file_buf = new char[nResourceLength];
                
            if(get_file_etag(strResourceFullPath.c_str(), szETag, file_buf) == false)
            {
                if(file_buf)
                    delete[] file_buf;
                    
                CHttpResponseHdr header;
                header.SetStatusCode(SC404);
                header.SetField("Content-Type", "text/html");
                header.SetField("Content-Length", header.GetDefaultHTMLLength());
                
                m_session->SendHeader(header.Text(), header.Length());
                m_session->SendContent(header.GetDefaultHTML(), header.GetDefaultHTMLLength());
                goto END_STEP1;
            }
            
            if(file_buf)
            {
                m_session->m_cache->wrlock_file_cache();
                bool push_rc = m_session->m_cache->_push_file_(strResourceFullPath.c_str(),
                    file_buf, nResourceLength, tLastModifyTime, szETag,
                    &file_cache_instance);
                m_session->m_cache->unlock_file_cache();
                
                if(push_rc)
                {   
                    file_cache_data = file_cache_instance->file_rdlock();
                    
                    file_cache_buf = file_cache_data->buf;
                    file_cache_available = true;
                }
                                 
                delete[] file_buf;
            }
        }
        
        sprintf(szQETagQ, "\"%s\"", (char*)szETag);
    }
    
    if(strcmp(m_session->GetRequestField("Cache-Control"), "no-cache") != 0
        && strcmp(m_session->GetRequestField("Pragma"), "no-cache") != 0
        && strcmp(m_session->GetRequestField("If-None-Match"), "") != 0 
        && strcmp(m_session->GetRequestField("If-None-Match"), szQETagQ) == 0)
    {
        CHttpResponseHdr header;
        header.SetStatusCode(SC304);
        unsigned int zero = 0;
        header.SetField("Content-Length", zero);    
        m_session->SendHeader(header.Text(), header.Length()); 
        goto END_STEP1;                     
    }
    
    /* Parse ranges */
    szHttpRequestRanges = m_session->GetRequestField("Range");
    x = 0;
    flag = false;
    while(szHttpRequestRanges != NULL)
    {
        if(strcmp(szHttpRequestRanges, "") == 0)
        {
            nRangeLength = nResourceLength;
            break;
        }
        if(!flag)
        {
            if(szHttpRequestRanges[x] == '=' || szHttpRequestRanges[x] != '\0')
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
        header.SetField("ETag", szQETagQ);
        header.SetField("Content-Length", nRangeLength);        
        if(strcasecmp(strExtName.c_str(), "css") == 0 || strcasecmp(strExtName.c_str(), "js") == 0)
        {
            header.SetField("Cache-Control", "public, max-age=31536000"); //365 days
            OutHTTPGMTDateString(time(NULL) + 31536000, strDateTime);
        }
        else if(strcasecmp(strExtName.c_str(), "jpg") == 0
            || strcasecmp(strExtName.c_str(), "jpeg") == 0
            || strcasecmp(strExtName.c_str(), "png") == 0
            || strcasecmp(strExtName.c_str(), "gif") == 0
            || strcasecmp(strExtName.c_str(), "bmp") == 0)
        {
            header.SetField("Cache-Control", "public, max-age=86400"); // 24 hours
            OutHTTPGMTDateString(time(NULL) + 86400, strDateTime);
        }
        else
        {
            OutHTTPGMTDateString(time(NULL) + 28800, strDateTime);
            header.SetField("Cache-Control", "public, max-age=28800"); // 8 hours
        }
        header.SetField("Expires", strDateTime.c_str());
        OutHTTPGMTDateString(tLastModifyTime, strDateTime);
        header.SetField("Last-Modified", strDateTime.c_str());
        
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
        
        m_session->SendHeader(header.Text(), header.Length());
        
        if(strcmp(szHttpRequestRanges, "") == 0)
        {
            if(m_session->GetMethod() != hmHead)
            {
                m_session->SendContent(file_cache_buf, nResourceLength);
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
                    if(m_session->GetMethod() != hmHead)
                        m_session->SendContent(file_cache_buf + content_ranges[v].beg, 
                            rest_length > content_ranges[v].len ? content_ranges[v].len : rest_length);
                }
            }
        }
        //free(file_cache_buf);
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
            m_session->SendHeader(header.Text(), header.Length());
            m_session->SendContent(header.GetDefaultHTML(), header.GetDefaultHTMLLength());
        }
        else
        {
            CHttpResponseHdr header;
            if(strcmp(szHttpRequestRanges, "") != 0 && strContentRangeResponse != "")
                header.SetStatusCode(SC206);
            else
                header.SetStatusCode(SC200);
            header.SetField("ETag", szQETagQ);
            header.SetField("Content-Length", nRangeLength);
            
            if(strcasecmp(strExtName.c_str(), "css") == 0 || strcasecmp(strExtName.c_str(), "js") == 0)
            {
                header.SetField("Cache-Control", "public, max-age=31536000"); //365 days
                OutHTTPGMTDateString(time(NULL) + 31536000, strDateTime);
            }
            else if(strcasecmp(strExtName.c_str(), "jpg") == 0
                || strcasecmp(strExtName.c_str(), "jpeg") == 0
                || strcasecmp(strExtName.c_str(), "png") == 0
                || strcasecmp(strExtName.c_str(), "gif") == 0
                || strcasecmp(strExtName.c_str(), "bmp") == 0)
            {
                header.SetField("Cache-Control", "public, max-age=86400"); //24 hours
                OutHTTPGMTDateString(time(NULL) + 86400, strDateTime);
            }
            else
            {
                OutHTTPGMTDateString(time(NULL) + 28800, strDateTime); // 8 hours
                header.SetField("Cache-Control", "public, max-age=28800");
            }
            header.SetField("Expires", strDateTime.c_str());
            OutHTTPGMTDateString(tLastModifyTime, strDateTime);
            header.SetField("Last-Modified", strDateTime.c_str());
            
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
            
            m_session->SendHeader(header.Text(), header.Length());
            
            if(strcmp(szHttpRequestRanges, "") == 0)
            {
                if(m_session->GetMethod() != hmHead)
                {
                    char rbuf[1449];
                    while(1)
                    {
                        if(ifsResource.eof())
                            break;
                        
                        if(ifsResource.read(rbuf, 1448) < 0)
                            break;
                        
                        int rlen = ifsResource.gcount();
                        if(m_session->GetMethod() != hmHead)
                            m_session->SendContent(rbuf, rlen);
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
                            int rest_len = content_ranges[v].len - len_sum;
                            if(ifsResource.read(rbuf, rest_len  > 1448 ? 1448 : rest_len) < 0)
                                break;
                            
                            int rlen = ifsResource.gcount();
                            len_sum += rlen;
                            if(m_session->GetMethod() != hmHead)
                                m_session->SendContent(rbuf, rlen);
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
END_STEP1:
    if(file_cache_instance)
        file_cache_instance->file_unlock();
    return;
}
