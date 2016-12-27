# Maciej Buszka

DIR 		= $(strip $(shell dirname $(realpath $(lastword $(MAKEFILE_LIST)))))
CC 			= clang
CFLAGS  = -g -Wall -Wextra -std=gnu99 -D_GNU_SOURCE
LDFLAGS = -L$(DIR) -lmalloc -Wl,-rpath=$(DIR) -pthread

all : libmalloc.so tester test

test :

tester :

arena.o : arena.c arena.h
	$(CC) $(CFLAGS) -fPIC -c arena.c

malloc.o : malloc.c malloc.h
	$(CC) $(CFLAGS) -fPIC -c malloc.c

libmalloc.so : malloc.o arena.o
	$(CC) -g -shared -o libmalloc.so malloc.o arena.o -pthread

clean :
	rm libmalloc.so *.o tester
