TARGET = sac_cli
LIBS = -lcommons -lsockets -lpthread -lfuse
CC = gcc
CFLAGS = -g -Wall -D_FILE_OFFSET_BITS=64 -DFUSE_USE_VERSION=27

.PHONY: default all clean

default: $(TARGET)
all: clean default 

OBJECTS = $(patsubst %.c, %.o, $(wildcard *.c))
HEADERS = $(wildcard *.h)

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

.PRECIOUS: $(TARGET) $(OBJECTS)

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) $(CFLAGS) $(LIBS) -o $@

clean:
	-rm -f *.o
	-rm -f $(TARGET)
	-rm -f *.log