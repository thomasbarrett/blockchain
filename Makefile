CC=clang

all: src/main.c src/guid.c | include/guid.h
	$(CC) -fsanitize=address -o main src/main.c src/guid.c -Iinclude -I/usr/local/include -L/usr/local/lib -luv -lsodium