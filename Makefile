# Makefile --------------------------------------------------------------------
C=/afs/nd.edu/user14/csesoft/new/bin/g++
CFLAGS=-Wall -std=c++11 -g
LD=/afs/nd.edu/user14/csesoft/new/bin/g++
LDFLAGS=-static-libstdc++
# -----------------------------------------------------------------------------

all: virtmem

virtmem: main.o page_table.o disk.o program.o
	$(LD) $^ $(LDFLAGS) -o virtmem

main.o: main.cpp
	$(C) $(CFLAGS) -c main.cpp -o main.o

page_table.o: page_table.cpp
	$(C) $(CFLAGS) -c page_table.cpp -o page_table.cpp

disk.o: disk.cpp
	$(C) $(CFLAGS) -c disk.cpp -o disk.o

program.o: program.cpp
	$(C) $(CFLAGS) -c program.cpp -o program.o


clean:
	rm -f *.o virtmem

