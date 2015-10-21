CC=gcc
CFLAGS=-std=c99

OS := $(shell uname)
ifeq ($(OS),Linux) 
  CFLAGS+=-lmcheck
else ifeq ($(OS),FreeBSD)
  CC=gcc5
endif

all: debug

help:
	@echo "Zeta"
	@echo "make         - builds VM & language"
	@echo "make release - builds VM & language as optimized code."
	@echo "make clean   - deletes build"
	@echo "make help    - prints this help"

test_gdb: debug
	gdb -ex run --args ./zeta --test

test: debug
	./zeta --test

#gcc -fsanitize=address -fsanitize=undefined -fno-sanitize-recover -fstack-protector
debug: *.c
	 $(CC) $(CFLAGS) -O0 -g -ftrapv -o zeta vm.c parser.c interp.c main.c

release: *.c
	 $(CC) $(CFLAGS) -O4 -o zeta vm.c parser.c interp.c main.c

clean:
	rm -f *.o

# Tells make which targets are not files. 
.PHONY: test test_gdb debug release all clean

