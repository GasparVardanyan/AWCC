PREFIX = /usr/local

CFLAGS=-std=c18 -pedantic-errors -Werror=pedantic -O0 -g -DDEBUG -I./include -I./temporary/include -I./src -I./temporary/src -include stdio.h -DENABLE_LOGS
LIBS=

DEPS := $(wildcard include/*.h) $(wildcard temporary/include/*.h) $(wildcard src/*.h) $(wildcard temporary/src/*.h)
SRC := $(wildcard src/*.c) $(wildcard temporary/src/*.c)

OBJ=${SRC:%.c=%.o}

awcc_autoboost: $(OBJ)
	$(CC) -o $@ $^ $(LIBS)

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

clean:
	rm -f $(OBJ) awcc_autoboost

compile_commands.json: Makefile
	@command -v compiledb >/dev/null || { echo "compiledb not found! Install it with 'pipx install compiledb'"; exit 1; }
	@compiledb -n make

compiledb: compile_commands.json

run: awcc_autoboost compiledb # update compile_commands.json for neovim
	./awcc_autoboost

install: awcc_autoboost
	cp -fv awcc_autoboost /usr/local/bin
	mkdir -pv /etc/systemd/system
	cp -fv systemd/awcc_autoboost.service /etc/systemd/system/awcc_autoboost.service

.PHONY: awcc_autoboost clean install uninstall compiledb
