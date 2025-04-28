CC	= gcc
CFLAGS	=	-Wall -Wextra \
	-I../libmysyslog \
	-I../libmysyslog-json \
	-I/usr/include/json-c

LDFLAGS	=	-L../libmysyslog-json	-lmysyslog-json \
		-L../libmysyslog	-lmysyslog \
		-ljson-c

TARGET	= myRPC-client

all: $(TARGET)

$(TARGET): myRPC-client.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

myRPC-client.o: myRPC-client.c
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f $(TARGET) *.o

.PHONY: all clean deb 

deb: all
	mkdir -p pkg/DEBIAN pkg/usr/bin
	printf "Package: myrpc-client\n\
Version: 1.0\n\
Section: net\n\
Priority: optional\n\
Architecture: amd64\n\
Depends: libc6 (>= 2.27), libmysyslog-json (>= 1.0), libmysyslog (>= 1.0), libjson-c5\n\
Maintainer: Karay <Alecorna05@icloud.com>\n\
Description: myRPC client — консольная утилита для JSON-RPC вызова команд\n" \
	> pkg/DEBIAN/control
	cp myRPC-client pkg/usr/bin/
	chmod 755 pkg/usr/bin/myRPC-client
	dpkg-deb --build pkg myrpc-client_1.0_amd64.deb
