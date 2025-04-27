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

.PHONY: all clean
