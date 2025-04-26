CC = gcc
CFLAGS = -Wall -Wextra -fPIC
TARGET = libmysyslog-text.so

all: $(TARGET)

$(TARGET): libmysyslog-text.o
	$(CC) -shared -o $(TARGET) libmysyslog-text.o

libmysyslog-text.o: libmysyslog-text.c
	$(CC) $(CFLAGS) -I../libmysyslog -c libmysyslog-text.c

clean:
	rm -f $(TARGET) *.o

.PHONY: all clean
