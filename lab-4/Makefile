CC = clang
CPPFLAGS = -Iinclude
CFLAGS = -g -O2 -Wall -Wextra $(shell pkg-config --cflags fuse)
LDLIBS = $(shell pkg-config --libs fuse)

fat16: fat16.o blkio.o

blkio.o: blkio.c blkio.h
fat16.o: fat16.c fat16.h blkio.h

clean:
	rm -f *~ *.o fat16

# vim: ts=8 sw=8 noet
