CC = gcc

EXT1 = libextsample1.so
EXT2 = libextsample2.so
EXT3 = libextsample3.so

HTTP_BALANCER = libhttpbalancer.so

EXTOBJ1 = extsample1.o
EXTOBJ2 = extsample2.o
EXTOBJ3 = extsample3.o

HTTP_BALANCER_OBJ = httpbalancer.o  \
                ../src/tinyxml/tinyxmlparser.o	    \
                ../src/tinyxml/tinyxmlerror.o      \
                ../src/tinyxml/tinyxml.o           \
                ../src/tinyxml/tinystr.o

FLAGS = -O2 -fPIC -w

INCDIR = -I../src/
LDDIRS = -L../src/

all: ext

install: all
	cp libextsample1.so /var/niuhttpd/ext/
	cp libextsample2.so /var/niuhttpd/ext/
	cp libextsample3.so /var/niuhttpd/ext/

bal_install: bal
	cp libhttpbalancer.so /var/niuhttpd/ext/
	test -x /etc/niuhttpd/extension/ || mkdir /etc/niuhttpd/extension/
	cp httpbalancer.xml /etc/niuhttpd/extension/httpbalancer.xml
    
ext: $(EXT1) $(EXT2) $(EXT3)

bal: $(HTTP_BALANCER)

$(EXT1): $(EXTOBJ1)
	$(CC) -shared -W -fPIC -o $@ $(EXTOBJ1) ${LDDIRS} -lstdc++ -lssl -lcrypto -lniuhttp

$(EXT2): $(EXTOBJ2)
	$(CC) -shared -W -fPIC -o $@ $(EXTOBJ2) ${LDDIRS} -lstdc++ -lssl -lcrypto -lniuhttp

$(EXT3): $(EXTOBJ3)
	$(CC) -shared -W -fPIC -o $@ $(EXTOBJ3) ${LDDIRS} -lstdc++ -lssl -lcrypto -lniuhttp

$(HTTP_BALANCER): $(HTTP_BALANCER_OBJ)
	$(CC) -shared -W -fPIC -o $@ $(HTTP_BALANCER_OBJ) ${LDDIRS} -lstdc++ -lssl -lcrypto -lniuhttp
    
%.o : %.cpp
	$(CC) $(INCDIR) $(FLAGS) -c $< -o $@

clean:
	-rm -f $(HTTPWS) $(HTTPWSOBJS) *.elf *.gdb *.o *.a *.so

