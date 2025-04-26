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

.PHONY: all clean
