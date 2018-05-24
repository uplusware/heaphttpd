CC = gcc
HTTPSRV = heaphttpd
HTTPSRVOBJS = mainsrv.o service.o session.o backend_session.o session_group.o pool.o base.o

NIUTOOL = heaptool
NIUTOOLOBJS = heaptool.o util/DES.o

AUTHLIB = libheapauth.so
AUTHLIBOBJS = heapauth.o

WSLIB = libheapwebsocket.so
WSLIBOBJS = websocket.o

HTTPLIB = libheaphttp.so
HTTPLIBOBJS =   httpsessionvar.o httpservervar.o cookie.o   \
                http.o httpcomm.o cache.o htdoc.o http2.o http2comm.o hpack.o http2stream.o tunneling.o http_client.o  \
                fstring.o webdoc.o webcgi.o heapapi.o wwwauth.o cgi.o fastcgi.o scgi.o serviceobj.o	httprequest.o httpresponse.o httputility.o\
                util/trace.o	            \
                util/digcalc.o	            \
                util/sha1.o                 \
                util/md5.o	                \
                util/DES.o	                \
                util/sysdep.o               \
                util/uuid.o                 \
                util/huffman.o              \
                tinyxml/tinyxmlparser.o	    \
                tinyxml/tinyxmlerror.o      \
                tinyxml/tinyxml.o           \
                tinyxml/tinystr.o


ifdef CYGWIN
MDEF = CYGWIN
else
MDEF = _LINUX_OS_
endif

OPENSSL_DIR=openssl

INCDIR = -I../$(OPENSSL_DIR)/include
LDDIRS = -L.
    
ifdef memcached
    LDLIST_HTTP = -lstdc++ -lcrypt -lmemcached -lheapauth -lheapwebsocket
    MEMCACHED_DEF = _WITH_MEMCACHED_
else
    LDLIST_HTTP = -lstdc++ -lcrypt -lheapauth -lheapwebsocket
    MEMCACHED_DEF = _WITHOUT_MEMCACHED_
endif

ifdef ASYNC
    ASYNC_DEF = _WITH_ASYNC_
else
    ASYNC_DEF = _WITHOUT_ASYNC_
endif

LDLIST_SVR = ../$(OPENSSL_DIR)/libssl.a ../$(OPENSSL_DIR)/libcrypto.a -lstdc++ -lrt -ldl -lpthread -lheaphttp -lheapauth -lheapwebsocket
LDLIST_TOOL = -lstdc++
LDLIST_AUTH = -lstdc++
LDLIST_WS = -lstdc++

FLAGS = -O2 -fPIC -w

all: libws libauth libhttp websrv pgmtool

websrv: $(HTTPSRV)
pgmtool: $(NIUTOOL)
libhttp: $(HTTPLIB)
libauth: $(AUTHLIB)
libws: $(WSLIB)


$(HTTPSRV): $(HTTPSRVOBJS)
	$(CC) -o $@ $(HTTPSRVOBJS) $(LDDIRS) $(LDLIST_SVR) 

$(NIUTOOL): $(NIUTOOLOBJS)
	$(CC) -o $@ $(NIUTOOLOBJS) $(LDDIRS) $(LDLIST_TOOL) 
	
$(HTTPLIB): $(HTTPLIBOBJS)
	$(CC) -shared -W -fPIC -o $@ $(HTTPLIBOBJS) $(LDDIRS) $(LDLIST_HTTP) 

$(AUTHLIB): $(AUTHLIBOBJS)
	$(CC) -shared -W -fPIC -o $@ $(AUTHLIBOBJS) $(LDDIRS) $(LDLIST_AUTH)

$(WSLIB): $(WSLIBOBJS)
	$(CC) -shared -W -fPIC -o $@ $(WSLIBOBJS) $(LDDIRS) $(LDLIST_WS)
    
%.o : %.cpp
	$(CC) -D$(MDEF) -D$(MEMCACHED_DEF) -D$(ASYNC_DEF) $(INCDIR) $(FLAGS) -c $< -o $@

clean:
	-rm -f  $(HTTPSRV) $(HTTPLIB) $(WSLIB) $(AUTHLIB) $(NIUTOOL) $(WSLIBOBJS) $(HTTPSRVOBJS) $(NIUTOOLOBJS) $(HTTPLIBOBJS) $(AUTHLIBOBJS) *.elf *.gdb *.o *.a *.so
