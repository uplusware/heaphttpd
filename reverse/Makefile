CC = gcc

HTTPREVERSE = libhttpreverse.so

HTTPREVERSEOBJ = httpreverse.o

FLAGS = -O2 -fPIC -w

INCDIR = -I../src/
LDDIRS = -L../src/

all: reverse

install: all
	test -x /var/heaphttpd/reverse || mkdir /var/heaphttpd/reverse
	cp libhttpreverse.so /var/heaphttpd/reverse/
    
reverse: $(HTTPREVERSE)

$(HTTPREVERSE): $(HTTPREVERSEOBJ)
	$(CC) -shared -W -fPIC -o $@ $(HTTPREVERSEOBJ) ${LDDIRS} -lstdc++
%.o : %.cpp
	$(CC) $(INCDIR) $(FLAGS) -c $< -o $@

clean:
	-rm -f $(HTTPWS) $(HTTPWSOBJS) *.elf *.gdb *.o *.a *.so

