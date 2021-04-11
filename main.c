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
// Global algorithim variable to be called by p
char *alg;
int *frame_counter;
int *fifo_history; // make way to find earliest frame used

void page_fault_handler(struct page_table *pt, int page)
{
    if (!strcmp(alg, "lfu"))
    {
        lfu(pt, page);
        printf("DEBUG: %d", pt->nframes);
        page_table_set_entry(pt, page, page, PROT_READ | PROT_WRITE);
    }
    else
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

    // Pre-allocations
    if (!strcmp(alg, "lfu"))
    {
        // allocates frequency array
        frame_counter = (int *)malloc(nframes * sizeof(int));
    }
    else if (!strcmp(alg, "fifo"))
    {
        // allocates history array
        fifo_history = (int *)malloc(nframes * sizeof(int));
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