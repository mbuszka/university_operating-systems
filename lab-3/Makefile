# Maciej Buszka

DIR 		= $(strip $(shell dirname $(realpath $(lastword $(MAKEFILE_LIST)))))
CC 			= clang
CFLAGS  = -g -Wall -Wextra -std=gnu99
LDFLAGS = -L$(DIR) -lmalloc -Wl,-rpath=$(DIR)

all : libmalloc.so tester

tester :

arena.o : arena.c arena.h
	$(CC) $(CFLAGS) -fPIC -c arena.c

malloc.o : malloc.c malloc.h
	$(CC) $(CFLAGS) -fPIC -c malloc.c

libmalloc.so : malloc.o arena.o
	$(CC) -shared -o libmalloc.so malloc.o arena.o

clean :
	rm libmalloc.so *.o tester
