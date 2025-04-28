CC	=	gcc

CFLAGS	=	-Wall -Wextra \
		-I../libmysyslog \
		-I../libmysyslog-json \
		-I/usr/include/json-c

LDFLAGS	=	-L../libmysyslog-json	-lmysyslog-json \
		-L../libmysyslog	-lmysyslog \
		-ljson-c

TARGET	=	myRPC-server

all: $(TARGET)

$(TARGET): myRPC-server.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

clean:
	rm -f $(TARGET)

.PHONY: all clean deb

deb: all
	mkdir -p pkg/DEBIAN pkg/usr/bin
	printf "Package: myrpc-server\n\
Version: 1.0\n\
Section: net\n\
Priority: optional\n\
Architecture: amd64\n\
Depends: libc6 (>= 2.27), libmysyslog-json (>= 1.0), libmysyslog (>= 1.0), libjson-c5\n\
Maintainer: Karay <Alecorna05@icloud.com>\n\
Description: myRPC server — демон для приёма JSON-RPC команд и возврата результатов\n" > pkg/DEBIAN/control
	cp myRPC-server pkg/usr/bin/
	chmod 755 pkg/usr/bin/myRPC-server
	dpkg-deb --build pkg myrpc-server_1.0_amd64.deb
