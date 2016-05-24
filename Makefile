
CC=gcc
LIBS=-lsqlite3
CFLAGS+= -g


file-freshener:src/main.c src/freshen.h src/cp.c src/cp.h
	$(CC) $(CFLAGS) src/cp.c src/main.c -o file-freshener $(LIBS)
