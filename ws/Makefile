CC = gcc

HTTPWS = libecho.so
HTTPWSOBJS = echo.o

FLAGS = -O2 -fPIC

INCDIR = -I../src/
LDDIRS = -L../src/

all: ws

install: all
	cp libecho.so /var/heaphttpd/ws/
    
ws: $(HTTPWS)

$(HTTPWS): $(HTTPWSOBJS)
	$(CC) -shared -W -fPIC -o $@ $(HTTPWSOBJS) ${LDDIRS} -lstdc++ -lssl -lcrypto -lheapwebsocket
	
%.o : %.cpp
	$(CC) $(INCDIR) $(FLAGS) -c $< -o $@

clean:
	-rm -f $(HTTPWS) $(HTTPWSOBJS) *.elf *.gdb *.o *.a *.so

