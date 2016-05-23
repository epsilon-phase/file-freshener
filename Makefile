
CC=gcc
LIBS=-lsqlite3
CFLAGS= -g


freshenfiles:src/main.c src/freshen.h
	$(CC) $(CFLAGS) src/main.c -o freshenfiles $(LIBS)
