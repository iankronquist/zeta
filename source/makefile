all: debug

test: debug
	./zeta --test

debug: *.c
	gcc -std=c11 -O0 -g -o zeta vm.c parser.c interp.c main.c

release: *.c
	gcc -std=c11 -O4 -o zeta vm.c parser.c interp.c main.c

clean:
	rm -f *.o

