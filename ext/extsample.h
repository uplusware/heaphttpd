/*
	Copyright (c) openheap, uplusware
	uplusware@gmail.com
*/

#ifndef _EXTSAMPLE_H_
#define _EXTSAMPLE_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "http.h"
#include "htdoc.h"
#include "webdoc.h"
#include "niuapi.h"

extern "C" void* ext_request(CHttp* m_session, const char * action);
extern "C" void* ext_response(CHttp* m_session, const char * action, Htdoc* doc);
extern "C" void* ext_finish(CHttp* m_session, const char * action, Htdoc* doc);

#endif /* _EXTSAMPLE_H_ */
