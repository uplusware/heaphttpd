CC = gcc
HTTPSRV = niuhttpd
HTTPSRVOBJS = mainsrv.o service.o session.o pool.o base.o

NIUTOOL = niutool
NIUTOOLOBJS = niutool.o util/DES.o

AUTHLIB = libniuauth.so
AUTHLIBOBJS = niuauth.o

WSLIB = libniuwebsocket.so
WSLIBOBJS = websocket.o

HTTPLIB = libniuhttp.so
HTTPLIBOBJS =   httpsessionvar.o httpservervar.o cookie.o   \
                http.o httpcomm.o cache.o htdoc.o \
                fstring.o webdoc.o webcgi.o niuapi.o wwwauth.o fastcgi.o	\
                util/digcalc.o	            \
                util/sha1.o                 \
                util/md5.o	                \
                util/DES.o	                \
                util/trace.o	            \
                util/sysdep.o               \
                util/uuid.o                 \
                tinyxml/tinyxmlparser.o	    \
                tinyxml/tinyxmlerror.o      \
                tinyxml/tinyxml.o           \
                tinyxml/tinystr.o

ifdef CYGWIN
MDEF = CYGWIN
else
MDEF = _LINUX_OS_
endif
INCDIR =
LDDIRS = -L.
LDLIST_HTTP = -lstdc++ -lssl -lcrypto -lniuauth -lniuwebsocket
LDLIST_SVR = -lstdc++ -lrt -ldl -lssl -lcrypto  -lpthread -lniuhttp -lniuauth -lniuwebsocket
LDLIST_TOOL = -lstdc++
LDLIST_AUTH = -lstdc++
LDLIST_WS = -lstdc++ -lssl -lcrypto

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
	$(CC) -D$(MDEF) $(INCDIR) $(FLAGS) -c $< -o $@

clean:
	-rm -f  $(HTTPSRV) $(HTTPLIB) $(WSLIB) $(AUTHLIB) $(NIUTOOL) $(WSLIBOBJS) $(HTTPSRVOBJS) $(NIUTOOLOBJS) $(HTTPLIBOBJS) $(AUTHLIBOBJS) *.elf *.gdb *.o *.a *.so
