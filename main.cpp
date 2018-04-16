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
#include <algorithm>

using namespace std;

/* GLOBALS ------------------------------------------------------------------ */

int npages;
int nframes;
int policy;
int nfaults;
int nreads;
int nwrites;

deque<int> frame_table;
struct disk *disk;

char *physmem;

vector<int> page_evicts;

/* FUNCTIONS ---------------------------------------------------------------- */

bool compare_evicts(int p1, int p2)
{
    // Determines which of page1 or page2 has most evicts
    return page_evicts[p1] < page_evicts[p2];
}

void page_fault_handler( struct page_table *pt, int page )
{
    // printf("\nPage fault on page #%d\n",page);
    nfaults++;

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
	    // printf("Update bits\n");
	}
	else
	{
	    // Conflict Miss
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
		int temp;
		temp = rand() % nframes;
		evict = frame_table[temp];
		frame_table.erase(frame_table.begin()+temp);
	    }
	    else if (policy == 1)
	    {
		// Pull from priority queue
		evict = frame_table.front(); // Gets page of frame to evict
		frame_table.pop_front();
	    }
	    else
	    {
		auto it = min_element(begin(frame_table), end(frame_table), compare_evicts);
		int temp = distance(begin(frame_table), it);
		evict = frame_table[temp];
		frame_table.erase(frame_table.begin()+temp);
		page_evicts[evict]++;
	    }

	    // Check bits of frame to evict
	    page_table_get_entry(pt, evict, &frame, &bits);
	    page_table_set_entry(pt, evict, frame, 0);
	    if (bits & PROT_WRITE)
	    {
		// printf("Write frame %d to memory\n", frame);
		disk_write(disk, evict, &physmem[(frame)*BLOCK_SIZE]);
		nwrites++;
	    }

	    // printf("Set page table entry\n");
	    frame_table.push_back(page);

	    // printf("Read from memory\n");
	    disk_read(disk, page, &physmem[(frame)*BLOCK_SIZE]);
	    nreads++;

	    page_table_set_entry(pt, page, frame, PROT_READ);
	}
	else
	{
	    // Map to empty frame
	    frame = nframes - frame_table.size() - 1;
	    frame_table.push_back(page);

	    // Read from disk
	    // printf("Read from memory\n");
	    disk_read(disk, page, &physmem[(frame)*BLOCK_SIZE]);
	    nreads++;

	    // printf("Set page table entry\n");
	    page_table_set_entry(pt, page, frame, PROT_READ);
	}
    }
    // printf("PAGE %d --> FRAME %d\n", page, frame);
}

void usage(int status)
{
    printf("USAGE: ./virtmem [PAGES] [FRAMES] [rand|fifo|custom] [sort|scan|focus]\n");
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

    // check npages
    for (unsigned int i=0; i < strlen(argv[1]); i++) {
        if (argv[1][i] < 48 || argv[1][i] > 57) {
            usage(1);
        }
    }
    npages = atoi(argv[1]);

    // check nframes
    for (unsigned int i=0; i < strlen(argv[2]); i++) {
        if (argv[2][i] < 48 || argv[2][i] > 57) {
            usage(1);
        }
    }
    nframes = atoi(argv[2]);

    // check program arg
    const char *program;
    if (strcmp(argv[4], "sort") == 0 || strcmp(argv[4], "scan") == 0 || strcmp(argv[4], "focus") == 0) {
        program = argv[4];
    } else {
        usage(1);
    }

    nfaults = 0;
    nreads = 0;
    nwrites = 0;

    page_evicts.resize(npages, 0);

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
    else if (strcmp(argv[3], "custom") == 0)
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

    printf("Page Faults = %d\nDisk Reads = %d\nDisk Writes = %d\n", nfaults, nreads, nwrites);

    return 0;
}

