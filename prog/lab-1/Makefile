DIR = $(strip $(shell dirname $(realpath $(lastword $(MAKEFILE_LIST)))))
OPTS = -Wall -Wextra -std=gnu99
LINK = -L$(DIR) -lstrfun
E5OPTS = $(OPTS) $(LINK) -Wl,-rpath=$(DIR) 
all : ex1 ex2 ex3 ex4 ex5-load-time ex5-run-time ex6 libstrfun.so

ex1 : ex1.c utils.h
	gcc $(OPTS) -o ex1 ex1.c

ex2 : ex2.c utils.h
	gcc $(OPTS) -o ex2 ex2.c

ex3 : ex3.c utils.h
	gcc $(OPTS) -o ex3 ex3.c

ex4 : ex4.c utils.h
	gcc $(OPTS) -o ex4 ex4.c

ex5-load-time : ex5-load-time.c strfun.h libstrfun.so
	gcc $(E5OPTS) -o ex5-load-time  ex5-load-time.c

ex5-run-time : ex5-run-time.c strfun.h libstrfun.so
	gcc $(OPTS) $(OPTS) -ldl -o ex5-run-time ex5-run-time.c

ex6 : ex6.c utils.h
	gcc $(OPTS) -o ex6 ex6.c

strfun.o : strfun.h strfun.c
	gcc -c $(OPTS) -fpic strfun.c

libstrfun.so : strfun.o
	gcc -shared -o libstrfun.so strfun.o

clean :
	rm ex1 ex2 ex3 ex4 ex5-run-time ex5-load-time ex6 strfun.o libstrfun.so

