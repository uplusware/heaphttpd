/*
	Copyright (c) openheap, uplusware
	uplusware@gmail.com
*/
#include <stdio.h>
#include "upload.h"

//API upload
void ApiUpload::Response()
{
    CHttpResponseHdr header;
    header.SetStatusCode(SC200);
    header.SetField("Content-Type", "text/html");
    
    string strResp;
    strResp = "<html></head><title>API Sample</title></head><body><h1>niuhttpd web server/0.3</h1>";
    
    string filename;
    string filetype;
    const char* valbuf;
    int vallen;
    
    if(m_session->parse_multipart_formdata("ATTACHFILEDESC", filename, filetype, valbuf, vallen) == 0)
	{
        char* desc = (char*)malloc(vallen + 1);
        memcpy(desc, valbuf, vallen);
        desc[vallen] = '\0';
        strResp += desc;
        strResp += "<hr>";
        free(desc);
    }
    
    if(m_session->parse_multipart_formdata("ATTACHFILEBODY1", filename, filetype, valbuf, vallen) == 0)
	{
        string strFilePath;
        strFilePath = "/var/niuhttpd/html/upload/";
        strFilePath += filename;
        
        ofstream* attachfd =  new ofstream(strFilePath.c_str(), ios_base::binary|ios::out|ios::trunc);		
        if(valbuf && vallen > 0 && attachfd && attachfd->is_open())
        {             
            char szLen[64];
            sprintf(szLen, "%d", vallen);
            do {
                attachfd->write(valbuf + attachfd->tellp(), vallen - attachfd->tellp());
            } while(attachfd->tellp() != vallen);
            attachfd->close();

            string strEscapedFilename, strEscapedFiletype;
            escapeHTML(filename.c_str(), strEscapedFilename);
            escapeHTML(filetype.c_str(), strEscapedFiletype);
            strResp += "<p><b>Path:</b> /var/niuhttpd/html/upload/";
            strResp += strEscapedFilename;
            strResp += "<br><b>Type:</b> ";
            strResp += strEscapedFiletype;
            strResp += "<br><b>Size:</b> ";
            strResp += szLen;
            strResp += "bytes";
            if(strncasecmp(filetype.c_str(), "image/", sizeof("image/") - 1) == 0)
            {
                strResp += "<hr><img src=\"/upload/";
                strResp += filename;
                strResp += "\" />";
            }
        }
        if(attachfd)
            delete attachfd;
    }
    
    if(m_session->parse_multipart_formdata("ATTACHFILEBODY2", filename, filetype, valbuf, vallen) == 0)
	{
        string strFilePath = "/var/niuhttpd/html/upload/";
        strFilePath += filename;
        ofstream* attachfd =  new ofstream(strFilePath.c_str(), ios_base::binary|ios::out|ios::trunc);		
        if(valbuf && vallen > 0 && attachfd && attachfd->is_open())
        {             
            char szLen[64];
            sprintf(szLen, "%d", vallen);
            do {
                attachfd->write(valbuf + attachfd->tellp(), vallen - attachfd->tellp());
            } while(attachfd->tellp() != vallen);
            attachfd->close();

            string strEscapedFilename, strEscapedFiletype;
            escapeHTML(filename.c_str(), strEscapedFilename);
            escapeHTML(filetype.c_str(), strEscapedFiletype);
            strResp += "<p><b>Path:</b> /var/niuhttpd/html/upload/";
            strResp += strEscapedFilename;
            strResp += "<br><b>Type:</b> ";
            strResp += strEscapedFiletype;
            strResp += "<br><b>Size:</b> ";
            strResp += szLen;
            strResp += "bytes";
            if(strncasecmp(filetype.c_str(), "image/", sizeof("image/") - 1) == 0)
            {
                strResp += "<hr><img src=\"/upload/";
                strResp += filename;
                strResp += "\" />";
            }
        }
        if(attachfd)
            delete attachfd;
    }
    
    if(m_session->parse_multipart_formdata("ATTACHFILEBODY3", filename, filetype, valbuf, vallen) == 0)
	{
        string strFilePath = "/var/niuhttpd/html/upload/";
        strFilePath += filename;
        ofstream* attachfd =  new ofstream(strFilePath.c_str(), ios_base::binary|ios::out|ios::trunc);		
        if(valbuf && vallen > 0 && attachfd && attachfd->is_open())
        {             
            char szLen[64];
            sprintf(szLen, "%d", vallen);
            do {
                attachfd->write(valbuf + attachfd->tellp(), vallen - attachfd->tellp());
            } while(attachfd->tellp() != vallen);
            attachfd->close();

            string strEscapedFilename, strEscapedFiletype;
            escapeHTML(filename.c_str(), strEscapedFilename);
            escapeHTML(filetype.c_str(), strEscapedFiletype);
            strResp += "<p><b>Path:</b> /var/niuhttpd/html/upload/";
            strResp += strEscapedFilename;
            strResp += "<br><b>Type:</b> ";
            strResp += strEscapedFiletype;
            strResp += "<br><b>Size:</b> ";
            strResp += szLen;
            strResp += "bytes";
            if(strncasecmp(filetype.c_str(), "image/", sizeof("image/") - 1) == 0)
            {
                strResp += "<hr><img src=\"/upload/";
                strResp += filename;
                strResp += "\" />";
            }
        }
        if(attachfd)
            delete attachfd;
    }
    strResp += "</body></html>";
    header.SetField("Content-Length", strResp.length());
            
    m_session->SendHeader(header.Text(), header.Length());
	m_session->SendContent(strResp.c_str(), strResp.length());
}

void* api_upload_response(CHttp* session, const char* html_path)
{
	ApiUpload *pDoc = new ApiUpload(session, html_path);
	pDoc->Response();
	delete pDoc;
}
