
mini-c: compile.cpp compile.h main.c
	g++ compile.cpp main.c -o mini-c

test: mini-c test.c
	./mini-c test.c

clean:
	rm -rf mini-c test.c.s
