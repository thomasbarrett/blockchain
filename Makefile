CC=clang
CFLAGS=-fsanitize=address -Iinclude -I/usr/local/include -L/usr/local/lib

all: bin bin/main

bin:
	mkdir bin

bin/main: src/main.c src/settings.c src/network.c src/message.c src/peer_list.c src/message_history.c src/guid.c
	$(CC) $^ -o $@ $(CFLAGS) -luv -lsodium

clean:
	rm -r bin