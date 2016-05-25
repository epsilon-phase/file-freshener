
CC=gcc
LIBS=-lsqlite3
CFLAGS+= -g

all: file-freshener

file-freshener:src/main.c src/freshen.h src/cp.c src/cp.h Makefile
	$(CC) $(CFLAGS) src/cp.c src/main.c -o file-freshener $(LIBS)

clean: file-freshener
	rm file-freshener
