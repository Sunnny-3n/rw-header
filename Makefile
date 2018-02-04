CC = cc
CFLAGS = -Wall -g -O0
LDFLAGS = -lpthread

OBJS = block.o csapp.o 

all: block

block: $(OBJS)

csapp.o: csapp/csapp.c
	$(CC) $(CFLAGS) -c csapp/csapp.c

block.o: block.c
	$(CC) $(CFLAGS) -c block.c

clean:
	rm -f *~ *.o block

