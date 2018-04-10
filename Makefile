# Makefile --------------------------------------------------------------------
C=/afs/nd.edu/user14/csesoft/new/bin/gcc
CFLAGS=-Wall -std=c99 -g
LD=/afs/nd.edu/user14/csesoft/new/bin/gcc
LDFLAGS=
# -----------------------------------------------------------------------------

all: virtmem

virtmem: main.o page_table.o disk.o program.o
	$(LD) $^ $(LDFLAGS) -o virtmem

main.o: main.c
	$(C) $(CFLAGS) -c main.c -o main.o

page_table.o: page_table.c
	$(C) $(CFLAGS) -c page_table.c -o page_table.o

disk.o: disk.c
	$(C) $(CFLAGS) -c disk.c -o disk.o

program.o: program.c
	$(C) $(CFLAGS) -c program.c -o program.o


clean:
	rm -f *.o virtmem

