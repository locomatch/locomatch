TARGET = libsockets.so
LIBS = -lcommons
CC = gcc
CFLAGS = -g -Wall

.PHONY: default all clean

default: $(TARGET) 
all: clean default install

OBJECTS = $(patsubst %.c, %.o, $(wildcard *.c))
HEADERS = $(wildcard *.h)
H_SRCS = $(shell find . -iname "*.h" | tr '\n' ' ')

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

.PRECIOUS: $(TARGET) $(OBJECTS)

$(TARGET): $(OBJECTS)
	$(CC) -shared $(OBJECTS) $(CFLAGS) $(LIBS) -o $@

install: 
	cp -u libsockets.so /usr/lib
	rm -rf /usr/include/linuse/sockets/
	mkdir -p /usr/include/linuse/sockets
	cp --parents $(H_SRCS) /usr/include/linuse/sockets

clean:
	rm -f *.o
	rm -f $(TARGET)
	rm -f *.log