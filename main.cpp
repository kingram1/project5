/*
Main program for the virtual memory project.
Make all of your modifications to this file.
You may add or rearrange any code or data as you need.
The header files page_table.h and disk.h explain
how to use the page table and disk interfaces.
*/

#include "page_table.h"
#include "disk.h"
#include "program.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <iostream>
#include <deque>

using namespace std;

/* GLOBALS ------------------------------------------------------------------ */

int npages;
int nframes;
int policy;

deque<int> frame_table;
struct disk *disk;

char *physmem;

/* FUNCTIONS ---------------------------------------------------------------- */

void page_fault_handler( struct page_table *pt, int page )
{
    printf("\nPage fault on page #%d\n",page);

    int bits = 0;
    int frame = 0;
    
    page_table_get_entry(pt, page, &frame, &bits);

    // Check bits
    if (bits & PROT_READ)
    {
	// Add PROT_WRITE bit
	
	// Determine if page is in physical memory
	if (frame >= 0 && frame < nframes)
	{
	    page_table_set_entry(pt, page, frame, PROT_READ | PROT_WRITE);
	    printf("Update bits\n");
	}
	else
	{
	    // Conflict Miss
	    printf("BOOM: FRAME NUMBER= %d\n", frame);
	}
    }
    else
    {
	// Add PROT_READ bit
	int size = frame_table.size();
	int evict;
	if (size == page_table_get_nframes(pt))
	{
	    // Determine eviction policy
	    if (policy == 0)
	    {
		// Random eviction policy
		evict = rand() % size;
		evict = frame_table[evict];
		frame_table.erase(frame_table.begin()+evict);
	    }
	    else
	    {
		// Pull from priority queue
		evict = frame_table.front(); // Gets page of frame to evict
		frame_table.pop_front();
	    }

	    // Check bits of frame to evict
	    page_table_get_entry(pt, evict, &frame, &bits);
	    if (bits & PROT_WRITE)
	    {
		printf("Write frame %d to memory\n", frame);
		disk_write(disk, evict, &physmem[(frame)*BLOCK_SIZE]);
	    }

	    printf("Set page table entry\n");
	    frame_table.push_back(page);

	    printf("Read from memory\n");
	    disk_read(disk, page, &physmem[(frame)*BLOCK_SIZE]);
	    
	    page_table_set_entry(pt, page, frame, PROT_READ);
	}
	else
	{
	    // Map to empty frame
	    frame = nframes - frame_table.size() - 1;
	    frame_table.push_back(page);

	    // Read from disk
	    printf("Read from memory\n");
	    disk_read(disk, page, &physmem[(frame)*BLOCK_SIZE]);

	    printf("Set page table entry\n");
	    page_table_set_entry(pt, page, frame, PROT_READ);
	}
    }
    printf("PAGE %d --> FRAME %d\n", page, frame);
}

void usage(int status)
{
    printf("USAGE: ./virtmem [PAGES] [FRAMES] [rand|fifo|lru] [sort|scan|focus]\n");
    printf("   [PAGES]   - number of pages\n");
    printf("   [FRAMES]  - number of frames\n");
    exit(status);
}

/* MAIN --------------------------------------------------------------------- */

int main( int argc, char *argv[] )
{
    if(argc!=5)
    {
	// wrong number of arguments
	usage(1);
    }

    npages = atoi(argv[1]);
    nframes = atoi(argv[2]);
    const char *program = argv[4];

    disk = disk_open("myvirtualdisk", npages); // nblocks = npages
    if(!disk)
    {
	fprintf(stderr,"couldn't create virtual disk: %s\n",strerror(errno));
	return 1;
    }


    struct page_table *pt = page_table_create( npages, nframes, page_fault_handler );
    if(!pt)
    {
	fprintf(stderr,"couldn't create page table: %s\n",strerror(errno));
	return 1;
    }

    // Determine the policy
    if (strcmp(argv[3], "rand") == 0)
    {
	policy = 0;
    }
    else if (strcmp(argv[3], "fifo") == 0)
    {
	policy = 1;
    }
    else if (strcmp(argv[3], "lru") == 0)
    {
	policy = 2;
    }
    else
    {
	fprintf(stderr, "ERROR: Invalid eviction policy\n");
	usage(1);
    }

    char *virtmem = page_table_get_virtmem(pt);

    physmem = page_table_get_physmem(pt);

    if(!strcmp(program,"sort"))
    {
	sort_program(virtmem,npages*PAGE_SIZE);
    }
    else if(!strcmp(program,"scan")) 
    {
	scan_program(virtmem,npages*PAGE_SIZE);
    }
    else if(!strcmp(program,"focus")) 
    {
	focus_program(virtmem,npages*PAGE_SIZE);
    }
    else 
    {
	fprintf(stderr,"unknown program: %s\n",argv[3]);
	return 1;
    }

    page_table_delete(pt);
    disk_close(disk);

    return 0;
}

