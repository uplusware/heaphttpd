CC = gcc

EXT1 = libextsample1.so
EXT2 = libextsample2.so
EXT3 = libextsample3.so

EXTOBJ1 = extsample1.o
EXTOBJ2 = extsample2.o
EXTOBJ3 = extsample3.o

FLAGS = -O2 -fPIC -w

INCDIR = -I../src/
LDDIRS = -L../src/

all: ext

install: all
	cp libextsample1.so /var/niuhttpd/ext/
	cp libextsample2.so /var/niuhttpd/ext/
	cp libextsample3.so /var/niuhttpd/ext/
    cp -f extension.xml /etc/niuhttpd/extension.xml
    
ext: $(EXT1) $(EXT2) $(EXT3)

$(EXT1): $(EXTOBJ1)
	$(CC) -shared -W -fPIC -o $@ $(EXTOBJ1) ${LDDIRS} -lstdc++ -lssl -lcrypto -lniuhttp

$(EXT2): $(EXTOBJ2)
	$(CC) -shared -W -fPIC -o $@ $(EXTOBJ2) ${LDDIRS} -lstdc++ -lssl -lcrypto -lniuhttp

$(EXT3): $(EXTOBJ3)
	$(CC) -shared -W -fPIC -o $@ $(EXTOBJ3) ${LDDIRS} -lstdc++ -lssl -lcrypto -lniuhttp

%.o : %.cpp
	$(CC) $(INCDIR) $(FLAGS) -c $< -o $@

clean:
	-rm -f $(HTTPWS) $(HTTPWSOBJS) *.elf *.gdb *.o *.a *.so

