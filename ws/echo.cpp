/*
	Copyright (c) openheap, uplusware
	uplusware@gmail.com
*/

#include "echo.h"
#include "websocket.h"
#include <string>
using namespace std;

void* ws_echo_main(int sockfd, SSL* ssl)
{
    WebSocket web_socket(sockfd, ssl);
			
    WebSocket_Buffer wsBuffer;
    WebSocket_Buffer_Alloc(&wsBuffer);
    web_socket.Recv(&wsBuffer);
    string strRecv = wsBuffer.buf ? wsBuffer.buf : "";
    WebSocket_Buffer_Free(&wsBuffer);
    
    WebSocket_Buffer_Alloc(&wsBuffer, 1024, false, OPCODE_TEXT);
    if(strRecv == "")
    {
        strcpy(wsBuffer.buf, "Oops, received empty message via WebSocket.");
    }
    else
    {
        strcpy(wsBuffer.buf, "Received your message\"");
        strcat(wsBuffer.buf, strRecv.c_str());
        strcat(wsBuffer.buf, "\" via WebSocket.");
    }
    wsBuffer.len = strlen(wsBuffer.buf);
    wsBuffer.opcode = OPCODE_TEXT;
    wsBuffer.mask = false;
    web_socket.Send(&wsBuffer);
    WebSocket_Buffer_Free(&wsBuffer);

    return NULL;    
}
