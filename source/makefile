all: debug

test_gdb: debug
	gdb -ex run --args ./zeta --test

test: debug
	./zeta --test

#gcc -fsanitize=address -fsanitize=undefined -fno-sanitize-recover -fstack-protector
debug: *.c
	gcc -std=c11 -O0 -g -lmcheck -ftrapv -o zeta vm.c parser.c interp.c main.c

release: *.c
	gcc -std=c11 -O4 -o zeta vm.c parser.c interp.c main.c

clean:
	rm -f *.o

