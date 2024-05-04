CC=gcc
CFLAGS=-Wall -g

all: userspace

userspace: userspace.o
	$(CC) $(CFLAGS) -o userspace userspace.o

userspace.o: userspace.c
	$(CC) $(CFLAGS) -c userspace.c

clean:
	rm -f *.o userspace
