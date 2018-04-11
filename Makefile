CC=/afs/nd.edu/user14/csesoft/new/bin/gcc
CFLAGS=-std=c11 -Wall -Wpedantic -g -D_XOPEN_SOURCE=500
CPP=/afs/nd.edu/user14/csesoft/new/bin/g++
CPPFLAGS=-Wall -Wpedantic -g -std=c++11
LD=/afs/nd.edu/user14/csesoft/new/bin/g++
LDFLAGS=

virtmem: main.o page_table.o disk.o program.o
	$(LD) $^ $(LDFLAGS) -o $@

# Compile C files with C compiler instead of C++ compiler
page_table.o: page_table.c
disk.o: disk.c
program.o: program.c

page_table.o disk.o program.o:
	$(CC) $(CFLAGS) -c $^ -o $@


# EVIL IMPLICIT RULES MUST BE DESTROYED.
%.o: %.cpp
	$(CPP) $(CPPFLAGS) -c $^ -o $@

.PHONY: clean
clean:
	rm -f *.o virtmem myvirtualdisk

