/*
        Copyright (c) openheap, uplusware
        uplusware@gmail.com
*/
#include "upload.h"
#include <stdio.h>
#include "version.h"

// API upload
void ApiUpload::Response() {
  CHttpResponseHdr header;
  header.SetStatusCode(SC200);
  header.SetField("Content-Type", "text/html");

  string strResp;
  strResp =
      "<html><head><title>API Sample</title></head><body><h1>" VERSION_STRING
      "</h1><hr>";

  string filename;
  string filetype;
  const char* valbuf;
  int vallen;

  if (m_request->get_multipart_formdata("ATTACHFILEDESC", filename, filetype,
                                        valbuf, vallen) == 0) {
    char* desc = (char*)malloc(vallen + 1);
    memcpy(desc, valbuf, vallen);
    desc[vallen] = '\0';
    strResp += desc;
    strResp += "<hr>";
    free(desc);
  }

  if (m_request->get_multipart_formdata("ATTACHFILEBODY1", filename, filetype,
                                        valbuf, vallen) == 0) {
    string strFilePath;
    strFilePath = "/var/heaphttpd/html/upload/";
    strFilePath += filename;

    ofstream* attachfd = new ofstream(strFilePath.c_str(),
                                      ios_base::binary | ios::out | ios::trunc);
    if (valbuf && vallen > 0 && attachfd && attachfd->is_open()) {
      char szLen[64];
      sprintf(szLen, "%d", vallen);
      do {
        attachfd->write(valbuf + attachfd->tellp(), vallen - attachfd->tellp());
      } while (attachfd->tellp() != vallen);
      attachfd->close();

      string strEscapedFilename, strEscapedFiletype;
      http_utility::escape_HTML(filename.c_str(), strEscapedFilename);
      http_utility::escape_HTML(filetype.c_str(), strEscapedFiletype);

      string strURIFilename;
      http_utility::encode_URI(filename.c_str(), strURIFilename);
      strResp += "<p><b>Path:</b> /var/heaphttpd/html/upload/";
      strResp += strEscapedFilename;
      strResp += "<br><b>Type:</b> ";
      strResp += strEscapedFiletype;
      strResp += "<br><b>Size:</b> ";
      strResp += szLen;
      strResp += "bytes";
      if (strncasecmp(filetype.c_str(), "image/", sizeof("image/") - 1) == 0) {
        strResp += "<hr><img src=\"/upload/";
        strResp += strURIFilename;
        strResp += "\" />";
      }
    }
    if (attachfd)
      delete attachfd;
  }

  if (m_request->get_multipart_formdata("ATTACHFILEBODY2", filename, filetype,
                                        valbuf, vallen) == 0) {
    string strFilePath = "/var/heaphttpd/html/upload/";
    strFilePath += filename;
    ofstream* attachfd = new ofstream(strFilePath.c_str(),
                                      ios_base::binary | ios::out | ios::trunc);
    if (valbuf && vallen > 0 && attachfd && attachfd->is_open()) {
      char szLen[64];
      sprintf(szLen, "%d", vallen);
      do {
        attachfd->write(valbuf + attachfd->tellp(), vallen - attachfd->tellp());
      } while (attachfd->tellp() != vallen);
      attachfd->close();

      string strEscapedFilename, strEscapedFiletype;
      http_utility::escape_HTML(filename.c_str(), strEscapedFilename);
      http_utility::escape_HTML(filetype.c_str(), strEscapedFiletype);

      string strURIFilename;
      http_utility::encode_URI(filename.c_str(), strURIFilename);

      strResp += "<p><b>Path:</b> /var/heaphttpd/html/upload/";
      strResp += strEscapedFilename;
      strResp += "<br><b>Type:</b> ";
      strResp += strEscapedFiletype;
      strResp += "<br><b>Size:</b> ";
      strResp += szLen;
      strResp += "bytes";
      if (strncasecmp(filetype.c_str(), "image/", sizeof("image/") - 1) == 0) {
        strResp += "<hr><img src=\"/upload/";
        strResp += strURIFilename;
        strResp += "\" />";
      }
    }
    if (attachfd)
      delete attachfd;
  }

  if (m_request->get_multipart_formdata("ATTACHFILEBODY3", filename, filetype,
                                        valbuf, vallen) == 0) {
    string strFilePath = "/var/heaphttpd/html/upload/";
    strFilePath += filename;
    ofstream* attachfd = new ofstream(strFilePath.c_str(),
                                      ios_base::binary | ios::out | ios::trunc);
    if (valbuf && vallen > 0 && attachfd && attachfd->is_open()) {
      char szLen[64];
      sprintf(szLen, "%d", vallen);
      do {
        attachfd->write(valbuf + attachfd->tellp(), vallen - attachfd->tellp());
      } while (attachfd->tellp() != vallen);
      attachfd->close();

      string strEscapedFilename, strEscapedFiletype;
      http_utility::escape_HTML(filename.c_str(), strEscapedFilename);
      http_utility::escape_HTML(filetype.c_str(), strEscapedFiletype);

      string strURIFilename;
      http_utility::encode_URI(filename.c_str(), strURIFilename);

      strResp += "<p><b>Path:</b> /var/heaphttpd/html/upload/";
      strResp += strEscapedFilename;
      strResp += "<br><b>Type:</b> ";
      strResp += strEscapedFiletype;
      strResp += "<br><b>Size:</b> ";
      strResp += szLen;
      strResp += "bytes";
      if (strncasecmp(filetype.c_str(), "image/", sizeof("image/") - 1) == 0) {
        strResp += "<hr><img src=\"/upload/";
        strResp += strURIFilename;
        strResp += "\" />";
      }
    }
    if (attachfd)
      delete attachfd;
  }
  strResp += "</body></html>";
  header.SetField("Content-Length", strResp.length());

  m_response->send_header(header.Text(), header.Length());
  m_response->send_content(strResp.c_str(), strResp.length());
}

void* api_upload_response(http_request* request, http_response* response) {
  ApiUpload* api_inst = new ApiUpload(request, response);
  api_inst->Response();
  delete api_inst;
}
