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
    string strResp;
    fbufseg segAttach;
    if(m_session->parse_multipart_value("ATTACHFILEBODY", segAttach) == 0)
	{
        string filename, filetype;
        if(m_session->parse_multipart_filename("ATTACHFILEBODY", filename) != -1 &&
            m_session->parse_multipart_type("ATTACHFILEBODY", filetype) !=  -1)
        {
            char* buf = (char*)m_session->GetFormData()->c_buffer() + segAttach.m_byte_beg;
            int len = segAttach.m_byte_end - segAttach.m_byte_beg + 1;
            string strFilePath;
            strFilePath = "/var/niuhttpd/html/upload/";
            strFilePath += filename;
            ofstream* attachfd =  new ofstream(strFilePath.c_str(), ios_base::binary|ios::out|ios::trunc);		
            if(buf && len > 0 && attachfd && attachfd->is_open())
            {             
                char szLen[64];
                sprintf(szLen, "%d", len);
                do {
                    attachfd->write(buf + attachfd->tellp(), len - attachfd->tellp());
                } while(attachfd->tellp() != len);
                attachfd->close();
                
                header.SetStatusCode(SC200);
                header.SetField("Content-Type", "text/html");
                
                string strEscapedFilename, strEscapedFiletype;
                escapeHTML(filename.c_str(), strEscapedFilename);
                escapeHTML(filetype.c_str(), strEscapedFiletype);
                strResp = "<html></head><title>API Sample</title></head><body><h1>niuhttpd web server/0.3</h1>Upload Sample: <b>";
                strResp += strEscapedFilename;
                strResp += "</b> [";
                strResp += strEscapedFiletype;
                strResp += "] (";
                strResp += szLen;
                strResp += "bytes) ";
                strResp += "has been uploaded to /var/niuhttpd/html/upload/";
                strResp += strEscapedFilename;
                if(strncasecmp(filetype.c_str(), "image/", sizeof("image/") - 1) == 0)
                {
                    strResp += "<hr><img src=\"/upload/";
                    strResp += filename;
                    strResp += "\" />";
                }
                strResp += "</body></html>";
                header.SetField("Content-Length", strResp.length());
            
            }
            else
            {
                header.SetStatusCode(SC500);
                header.SetField("Content-Type", "text/html");
                
                strResp = header.GetDefaultHTML();
                header.SetField("Content-Length", strResp.length());
        
            }
            if(attachfd)
                delete attachfd;
        
        }
        else
        {
            header.SetStatusCode(SC200);
            header.SetField("Content-Type", "text/html");
            
            strResp = "<html></head><title>API Sample</title></head><body><h1>niuhttpd web server/0.3</h1>Upload failed.</body></html>";
            header.SetField("Content-Length", strResp.length());
        }
    }
    else
    {
        header.SetStatusCode(SC200);
        header.SetField("Content-Type", "text/html");
        
        strResp = "<html></head><title>API Sample</title></head><body><h1>niuhttpd web server/0.3</h1>Upload failed.</body></html>";
        header.SetField("Content-Length", strResp.length());
    }
    m_session->SendHeader(header.Text(), header.Length());
	m_session->SendContent(strResp.c_str(), strResp.length());
}

void* api_upload_response(CHttp* session, const char* html_path)
{
	ApiUpload *pDoc = new ApiUpload(session, html_path);
	pDoc->Response();
	delete pDoc;
}
