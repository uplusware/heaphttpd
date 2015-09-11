CC = gcc

HTTPAPI = libsample.so
HTTPAPIOBJS = sample.o

MYSQLDEMOAPI = libmysqldemo.so
MYSQLDEMOAPIOBJS = mysqldemo.o stgengine.o storage.o

FLAGS = -O2 -fPIC -w

INCDIR = -I../src/ -I/usr/include/mysql/ 
LDDIRS = -L../src/ -L/usr/lib64/mysql/ -L.

all: apisample apimysqldemo

install: all
	cp libsample.so /var/niuhttpd/api/
	cp libmysqldemo.so /var/niuhttpd/api/
    
apisample: $(HTTPAPI)
apimysqldemo: $(MYSQLDEMOAPI)

$(HTTPAPI): $(HTTPAPIOBJS)
	$(CC) -shared -W -fPIC -o $@ $(HTTPAPIOBJS) ${LDDIRS} -lstdc++ -lssl -lcrypto -lniuhttp 

$(MYSQLDEMOAPI): $(MYSQLDEMOAPIOBJS)
	$(CC) -shared -W -fPIC -o $@ $(MYSQLDEMOAPIOBJS) ${LDDIRS} -lstdc++ -lssl -lcrypto -lniuhttp -lmysqlclient_r
	
%.o : %.cpp
	$(CC) $(INCDIR) $(FLAGS) -c $< -o $@

clean:
	-rm -f $(HTTPAPI) $(HTTPAPIOBJS) $(MYSQLDEMOAPI) $(MYSQLDEMOAPIOBJS)  *.elf *.gdb *.o *.a *.so

