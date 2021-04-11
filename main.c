/*
Main program for the virtual memory project.
Make all of your modifications to this file.
You may add or rearrange any code or data as you need.
The header files page_table.h and disk.h explain
how to use the page table and disk interfaces.
*/

/*
Operating Systems - Project 4
Group Members - Tarik Brown, Joseph Sweilem, Daraius Balsara, Bradley Budden
*/

#include "page_table.h"
#include "disk.h"
#include "program.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

struct page_table
{
    int fd;
    char *virtmem;
    int npages;
    char *physmem;
    int nframes;
    int *page_mapping;
    int *page_bits;
    page_fault_handler_t handler;
};
// Global algorithim variable to indicate which replacement
// policy to be used in page_fault_handler
char *alg;
int *lru_alg_array;
int *fifo_alg_array; // make way to find earliest frame used
int *frames;

// return the first avaialble frame to insert data into PT
int check_frame_availibity(struct page_table *pt)
{
    for (int i = 0; i < pt->nframes; i++)
    {
        if (frames[i] == 0)
        {
            // if available frame is found, set bit to used and return frame number
            frames[i] = 1;
            return i;
        }
    }
    return -1;
}

void page_fault_handler(struct page_table *pt, int page)
{
    // iterate page fault

    // initialize bits and frame

    int bits, frame;

    page_table_get_entry(pt, page, frame, bits);

    if (bits == 0)
    {
        // Find available frame
        if ((frame = check_frame_availibity(pt)) < 0)
        {
            if (strcmp(alg, "fifo"))
            {
                // fifo
            }
            else if (strcmp(alg, "random"))
            {
                // random
            }
            else
            {
                // custom
            }
        }
        // If a free frame is found insert into PT
        else
        {
            // write to disk
        }
    }
    else
    {
        page_table_set_entry(pt, page, frame, (PROT_READ | PROT_WRITE));
    }
}

int main(int argc, char *argv[])
{

    // 1. Allocate page table
    // 2. Allocate frame table
    // 3. Call respective progam (alpha, beta etc.)
    // 4. Algorithm variable is globally called and handled as needed
    // 4a. LFU: frequency counter array is initialized
    // 4b. FIFO: history array is initialized
    // 5. Program is finished
    if (argc != 5)
    {
        printf("use: virtmem <npages> <nframes> <rand|fifo|custom> <alpha|beta|gamma|delta>\n");
        return 1;
    }

    int npages = atoi(argv[1]);
    int nframes = atoi(argv[2]);
    alg = argv[3];
    const char *program = argv[4];

    // Pre-allocations of data structures needed for page_fault_handler logic
    frames = (int *)malloc(nframes * sizeof(int));
    if (!frames)
    {
        fprint("couldn't create frame state list\n");
    }
    for (int i = 0; i < nframes; i++)
    {
        frames[i] = 0;
    }

    if (!strcmp(alg, "lfu"))
    {
        // allocates array for use in LRU replacement policy
        lru_alg_array = (int *)malloc(nframes * sizeof(int));
        if (!lru_alg_array)
        {
            fprint("couldn't create lru array\n");
        }
    }
    else if (!strcmp(alg, "fifo"))
    {
        // allocates array for use in FIFO replacement policy
        fifo_alg_array = (int *)malloc(nframes * sizeof(int));
        if (!fifo_alg_array)
        {
            fprint("couldn't create fifo array\n");
        }
    }

    struct disk *disk = disk_open("myvirtualdisk", npages);

    if (!disk)
    {
        fprintf(stderr, "couldn't create virtual disk: %s\n", strerror(errno));
        return 1;
    }

    struct page_table *pt = page_table_create(npages, nframes, page_fault_handler);

    if (!pt)
    {
        fprintf(stderr, "couldn't create page table: %s\n", strerror(errno));
        return 1;
    }

    char *virtmem = page_table_get_virtmem(pt);

    char *physmem = page_table_get_physmem(pt);

    if (!strcmp(program, "alpha"))
    {
        alpha_program(virtmem, npages * PAGE_SIZE);
    }
    else if (!strcmp(program, "beta"))
    {
        beta_program(virtmem, npages * PAGE_SIZE);
    }
    else if (!strcmp(program, "gamma"))
    {
        gamma_program(virtmem, npages * PAGE_SIZE);
    }
    else if (!strcmp(program, "delta"))
    {
        delta_program(virtmem, npages * PAGE_SIZE);
    }
    else
    {
        fprintf(stderr, "unknown program: %s\n", argv[3]);
        return 1;
    }

    page_table_delete(pt);
    disk_close(disk);

    return 0;
}