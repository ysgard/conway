# Assumes libtcod is available on the system
# See https://github.com/libtcod/libtcod on how to acquire it
.POSIX:
.SUFFIXES:
CC = gcc
CFLAGS = -W -O
LDLIBS = -ltcod
all: conway

conway: conway.o
	$(CC) -o conway conway.o $(LDLIBS)

conway.o: conway.c
	$(CC) -c conway.c

clean:
	rm -f *.o
	rm -f conway
