CC = gcc

HTTPAPI = libsample.so
HTTPAPIOBJS = sample.o

PERFAPI = libperf.so
PERFAPIOBJS = perf.o

UPLOAD = libupload.so
UPLOADOBJS = upload.o

ifdef MONGODB
    DB_DEF = _MONGODB_
else
ifdef MYSQLDB
    DB_DEF = _MARIADB_
    DB_INCDIR = -I/usr/include/mariadb/
else
ifdef UPLOAD
    DB_DEF = _UPLOAD_
else
	DB_DEF = _SAMPLE_
endif
endif
endif

MYSQLDEMOAPI = libmysqldemo.so
MYSQLDEMOAPIOBJS = mysqldemo.o stgengine.o storage.o

MONGODEMOAPI = libmongodemo.so
MONGODEMOAPIOBJS = mongodemo.o stgengine.o storage.o

FLAGS = -O2 -fPIC -w

INCDIR = -I../src/ $(DB_INCDIR)
LDDIRS = -L../src/ -L/usr/lib/x86_64-linux-gnu/ -L.

ifdef MONGODB
all: apimongodemo
install: all
	cp libmongodemo.so /var/heaphttpd/api/
else
ifdef MYSQLDB
all: apimysqldemo
install: all
	cp libmysqldemo.so /var/heaphttpd/api/
else
ifdef PERF
all: apiperf
install: all
	cp libperf.so /var/heaphttpd/api/
else
ifdef UPLOADSAMPLE
all: apiupload
install: all
	cp libupload.so /var/heaphttpd/api/
	test -x /var/heaphttpd/html/upload || mkdir /var/heaphttpd/html/upload
else
all: apisample
install: all
	cp libsample.so /var/heaphttpd/api/
endif
endif
endif
endif

    
apisample: $(HTTPAPI)
apimysqldemo: $(MYSQLDEMOAPI)
apimongodemo: $(MONGODEMOAPI)
apiperf: $(PERFAPI)
apiupload: $(UPLOAD)

$(UPLOAD): $(UPLOADOBJS)
	$(CC) -shared -W -fPIC -o $@ $(UPLOADOBJS) ${LDDIRS} -lstdc++ -lheaphttp 
    
$(PERFAPI): $(PERFAPIOBJS)
	$(CC) -shared -W -fPIC -o $@ $(PERFAPIOBJS) ${LDDIRS} -lstdc++ -lheaphttp 
    
$(HTTPAPI): $(HTTPAPIOBJS)
	$(CC) -shared -W -fPIC -o $@ $(HTTPAPIOBJS) ${LDDIRS} -lstdc++ -lheaphttp 

$(MYSQLDEMOAPI): $(MYSQLDEMOAPIOBJS)
	$(CC) -shared -W -fPIC -o $@ $(MYSQLDEMOAPIOBJS) ${LDDIRS} -lstdc++ -lheaphttp -lmariadb

$(MONGODEMOAPI): $(MONGODEMOAPIOBJS)
	$(CC) -shared -W -fPIC -o $@ $(MONGODEMOAPIOBJS) ${LDDIRS} -lstdc++ -lheaphttp `pkg-config --libs-only-l libmongoc-1.0`

ifdef MONGODB	
%.o : %.cpp
	$(CC) $(INCDIR) `pkg-config --cflags-only-I libmongoc-1.0` -D$(DB_DEF) $(FLAGS) -c $< -o $@ 
else
%.o : %.cpp
	$(CC) $(INCDIR) -D$(DB_DEF) $(FLAGS) -c $< -o $@
endif
clean:
	-rm -f $(HTTPAPI) $(UPLOADOBJS) $(HTTPAPIOBJS) $(MYSQLDEMOAPI) $(MYSQLDEMOAPIOBJS) $(MONGODEMOAPI) $(MONGODEMOAPIOBJS)  *.elf *.gdb *.o *.a *.so