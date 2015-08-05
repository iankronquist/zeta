all: release

release: *.c
	gcc -std=c11 -O4 -o zeta vm.c parser.c interp.c main.c

clean:
	rm -f *.o

