CC?=gcc
CFLAGS=-std=c11
CFLAGS_debug=-O0 -g -ftrapv
CFLAGS_release=-O4

OS := $(shell uname)
ifeq ($(OS),Linux) 
  CFLAGS_debug+=-lmcheck
else ifeq ($(OS),FreeBSD)
  CC=gcc5
endif

GCC_MAJ := $(shell expr `$(CC) -dumpversion | cut -d. -f1`)
GCC_MIN := $(shell expr `$(CC) -dumpversion | cut -d. -f2`)
GCC_GE_4_9 := $(shell [ $(GCC_MAJ) -ge 5 -o \( $(GCC_MAJ) -eq 4 -a $(GCC_MIN) -ge 9 \) ] && echo yes)

ifneq ($(GCC_GE_4_9),yes)
GCC_VER := $(shell expr `$(CC) -dumpversion`)
$(error GCC version required is 4.9+ (your version $(GCC_VER)).)
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
	$(CC) $(CFLAGS) $(CFLAGS_debug) -o zeta vm.c parser.c interp.c main.c

release: *.c
	$(CC) $(CFLAGS) $(CFLAGS_release) -o zeta vm.c parser.c interp.c main.c

clean:
	rm -f *.o

# Tells make which targets are not files. 
.PHONY: test test_gdb debug release all clean

