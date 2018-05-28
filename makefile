CFLAGS=-std=c99

p2: p2.o
	gcc -o p2 p2.o $(CFLAGS)
