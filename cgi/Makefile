CC = gcc
CGIPGM = sample.cgi
CGIPGMOBJS = sample.o ../src/niuapi.o

MDEF = _LINUX_OS_
INCDIR = -I../src
LDDIRS = -L.
LDLIST_CGIPGM = -lstdc++

FLAGS = -O2 -fPIC -w

all: pgm

install: all
	cp sample.cgi /var/niuhttpd/cgi-bin
    
pgm: $(CGIPGM)
	
$(CGIPGM): $(CGIPGMOBJS)
	$(CC) -o $@ $(CGIPGMOBJS) $(LDDIRS) $(LDLIST_CGIPGM) 

%.o : %.cpp
	$(CC) -D$(MDEF) $(INCDIR) $(FLAGS) -c $< -o $@

clean:
	-rm -f  $(CGIPGM) $(HTTPLIB) $(CGIPGMOBJS) $(HTTPLIBOBJS) *.elf *.gdb *.o *.a *.so

