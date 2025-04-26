CC = gcc
CFLAGS = -Wall -Wextra -fPIC
TARGET = libmysyslog.so

all: $(TARGET)

$(TARGET): libmysyslog.o
	$(CC) -shared -o $(TARGET) libmysyslog.o

libmysyslog.o: libmysyslog.c
	$(CC) $(CFLAGS) -c libmysyslog.c

clean:
	rm -f $(TARGET) *.o

.PHONY: all clean
