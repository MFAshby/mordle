CC=clang
OPTS=-g -Werror -Wall
PQ_CFLAGS=`pkg-config libpq --cflags`
PQ_LIBS=`pkg-config libpq --libs`
MUSTACH_CFLAGS=`pkg-config libmustach --cflags`
MUSTACH_LIBS=`pkg-config libmustach --libs`
MONGOOSE_CFLAGS=
MONGOOSE_LIBS=-lmongoose
SODIUM_CFLAGS=`pkb-config libsodium --cflags`
SODIUM_LIBS=`pkg-config libsodium --libs`
SYSTEMD_CFLAGS=`pkg-config libsystemd --cflags`
SYSTEMD_LIBS=`pkg-config libsystemd --libs`
NAME=mordle
## Set to `valgrind` to execute tests with valgrind
TEST_VALGRIND=

## Dependencies: libpq, libsodium, mongoose, mustach

## Default target check the tests compile but doesn't run them
all: bin/$(NAME) bin/storage_test bin/index_test
	true

install: all
	install -o root -m 755 bin/mordle /usr/local/bin/mordle
	install -o root -g mordle -m 755 public/* /var/lib/mordle/public
	install -o root -m 755 mordle.service /etc/systemd/system/mordle.service

uninstall:
	rm /usr/local/bin/mordle
	rm -rf /var/lib/mordle/public
	rm /etc/systemd/system/mordle.service

bin/$(NAME): obj/main.o obj/slog.o obj/game.o obj/storage.o obj/index.o obj/user.o
	$(CC) $(OPTS) -o bin/$(NAME) obj/main.o obj/slog.o obj/game.o obj/storage.o obj/index.o obj/user.o \
		 $(PQ_LIBS) \
		 $(MUSTACH_LIBS) \
		 $(MONGOOSE_LIBS) \
		 $(SODIUM_LIBS) \
		 $(SYSTEMD_LIBS) \
		 -lpthread

obj/main.o: src/main.c gen/index.html.h
	$(CC) $(OPTS) -o obj/main.o -c src/main.c \
		-I include \
		-I vendor/slog \
		$(PQ_CFLAGS) \
		$(MUSTACH_CFLAGS) \
		$(MONGOOSE_CFLAGS) \
		$(SYSTEMD_CFLAGS)

gen/index.html.h: template/index.html.tpl
	xxd -i template/index.html.tpl > gen/index.html.h

obj/index.o: src/index.c gen/index.html.h
	$(CC) $(OPTS) -o obj/index.o -c src/index.c \
		-I include -I gen -I vendor/slog

obj/game.o: src/game.c
	$(CC) $(OPTS) -o obj/game.o -c src/game.c \
		-I include

obj/user.o: src/user.c
	$(CC) $(OPTS) -o obj/user.o -c src/user.c \
		-I vendor/slog \
		-I include
		
obj/storage.o: src/storage.c
	$(CC) $(OPTS) -o obj/storage.o -c src/storage.c \
		-I include -I vendor/slog

bin/storage_test: src/storage_test.c vendor/munit/munit.c obj/storage.o obj/slog.o
	$(CC) -I include -I vendor/slog -I vendor/munit \
		-g \
		obj/storage.o obj/slog.o vendor/munit/munit.c src/storage_test.c \
		-o bin/storage_test -lpthread $(PQ_LIBS) $(SODIUM_LIBS)

bin/index_test: src/index_test.c vendor/munit/munit.c obj/index.o obj/slog.o obj/storage.o obj/game.o
	$(CC) -I include \
		-I vendor/slog \
		-I vendor/munit \
		-g \
		obj/index.o obj/slog.o obj/storage.o obj/game.o vendor/munit/munit.c src/index_test.c \
		-o bin/index_test \
		-lpthread \
		$(PQ_LIBS) \
		$(MUSTACH_LIBS) \
		$(SODIUM_LIBS)

test: bin/storage_test bin/index_test
	$(TEST_VALGRIND) bin/storage_test \
	&& $(TEST_VALGRIND) bin/index_test

obj/slog.o: vendor/slog/slog.c
	$(CC) $(OPTS) -o obj/slog.o -c vendor/slog/slog.c -I vendor/slog

clean:
	rm -rf obj/* bin/* gen/*

.PHONY: clean test
