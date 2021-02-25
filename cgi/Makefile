CC = gcc
CGIPGM = sample.cgi
CGIPGMOBJS = sample.o ../src/heapapi.o

MDEF = _LINUX_OS_
INCDIR = -I../src
LDDIRS = -L.
LDLIST_CGIPGM = -lstdc++

FLAGS = -O2 -fPIC

all: pgm

install: all
	cp sample.cgi /var/heaphttpd/cgi-bin
    
pgm: $(CGIPGM)
	
$(CGIPGM): $(CGIPGMOBJS)
	$(CC) -o $@ $(CGIPGMOBJS) $(LDDIRS) $(LDLIST_CGIPGM) 

%.o : %.cpp
	$(CC) -D$(MDEF) $(INCDIR) $(FLAGS) -c $< -o $@

clean:
	-rm -f  $(CGIPGM) $(HTTPLIB) $(CGIPGMOBJS) $(HTTPLIBOBJS) *.elf *.gdb *.o *.a *.so

