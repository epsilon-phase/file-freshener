
CC=gcc
LIBS=-lsqlite3
CFLAGS= -g


freshenfiles:src/main.c src/freshen.h src/cp.c src/cp.h
	$(CC) $(CFLAGS) src/cp.c src/main.c -o freshenfiles $(LIBS)
