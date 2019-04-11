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

FLAGS = -O2 -fPIC

INCDIR = -I../src/
LDDIRS = -L../src/

all: ext

install: all
	cp libextsample1.so /var/heaphttpd/ext/
	cp libextsample2.so /var/heaphttpd/ext/
	cp libextsample3.so /var/heaphttpd/ext/

bal_install: bal
	cp libhttpbalancer.so /var/heaphttpd/ext/
	test -x /etc/heaphttpd/extension/ || mkdir /etc/heaphttpd/extension/
	cp httpbalancer.xml /etc/heaphttpd/extension/httpbalancer.xml
    
ext: $(EXT1) $(EXT2) $(EXT3)

bal: $(HTTP_BALANCER)

$(EXT1): $(EXTOBJ1)
	$(CC) -shared -W -fPIC -o $@ $(EXTOBJ1) ${LDDIRS} -lstdc++ -lheaphttp

$(EXT2): $(EXTOBJ2)
	$(CC) -shared -W -fPIC -o $@ $(EXTOBJ2) ${LDDIRS} -lstdc++ -lheaphttp

$(EXT3): $(EXTOBJ3)
	$(CC) -shared -W -fPIC -o $@ $(EXTOBJ3) ${LDDIRS} -lstdc++ -lheaphttp

$(HTTP_BALANCER): $(HTTP_BALANCER_OBJ)
	$(CC) -shared -W -fPIC -o $@ $(HTTP_BALANCER_OBJ) ${LDDIRS} -lstdc++ -lheaphttp
    
%.o : %.cpp
	$(CC) $(INCDIR) $(FLAGS) -c $< -o $@

clean:
	-rm -f $(HTTPWS) $(HTTPWSOBJS) *.elf *.gdb *.o *.a *.so

