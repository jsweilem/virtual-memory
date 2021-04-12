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
int *frames;
int frame_counter = 0;
int page_faults;
int disk_reads;
int disk_writes;
struct disk *disk;

// return the first avaialble frame to insert data into PT
int check_frame_availibity(struct page_table *pt)
{
    for (int i = 0; i < pt->nframes; i++)
    {
        if (frames[i] == -1)
        {
            // if available frame is found, set bit to used and return frame number
            return i;
        }
    }
    return -1;
}

void replace_page(struct page_table *pt, int page)
{
    // initialize bits and frame
    int bits, frame;
    int fetch_bits;
    int fetch_frame;
    // get page table entry that currently exists in our frame array
    page_table_get_entry(pt, frames[frame_counter], &frame, &bits);

    if (frame_counter == frame)
    { // check if frame counter matches frame to be replaced, otherwise no replace is needed
        if (bits == (PROT_READ | PROT_WRITE))
        { // if write permissions exist, write back to disk before resetting
            disk_write(disk, frames[frame_counter], &pt->physmem[frame * BLOCK_SIZE]);
            disk_writes++;
            page_table_set_entry(pt, frames[frame_counter], frame, 0);
        }
        else
        { // if no write permissions exist, just reset the page
            page_table_set_entry(pt, frames[frame_counter], frame, 0);
        }

        // read the new page from the disk and set it in the page table
        disk_read(disk, page, &pt->physmem[frame * BLOCK_SIZE]);
        disk_reads++;

        if (pt->nframes > 1)
        {
            if (!strcmp(alg, "custom"))
            {

                // bring in next page, if there is only 1 frame, do not attempt to bring in surrounding pages
                if (page + 1 < pt->npages)
                {
                    int stop_custom = 0;
                    // check if page + 1 is in physical memory already
                    for (int i = 0; i < pt->nframes; i++)
                    {
                        if (frames[i] == page + 1)
                        {
                            stop_custom = 1;
                        }
                    }
                    if (!stop_custom)
                    {
                        fetch_frame = frame;
                        fetch_bits = bits;
                        fetch_frame++;
                        page_table_get_entry(pt, frames[frame_counter + 1], &fetch_frame, &fetch_bits);
                        if (fetch_bits & PROT_WRITE)
                        {
                            //disk_write(disk, page + 1, &pt->physmem[(frame)*BLOCK_SIZE]);
                            disk_write(disk, frames[frame_counter + 1], &pt->physmem[(frame_counter + 1) * BLOCK_SIZE]);
                            page_table_set_entry(pt, frames[frame_counter + 1], 0, 0);
                        }
                        disk_read(disk, page + 1, &pt->physmem[(frame)*BLOCK_SIZE]);
                        disk_reads++;

                        frames[frame_counter + 1] = page + 1;

                        page_table_set_entry(pt, page + 1, (frame_counter + 1) % pt->nframes, PROT_READ);
                    }
                }
            }
        }
        page_table_set_entry(pt, page, frame, PROT_READ);

        // update frame table with replaced page
        frames[frame_counter] = page;
    }
}

void page_fault_handler(struct page_table *pt, int page)
{
    // initialize bits and frame
    int bits, frame;
    page_table_get_entry(pt, page, &frame, &bits);

    // count number of page faults
    page_faults++;
    // if there is no PT mapping
    if (bits == 0)
    {
        // Find available frame
        if ((frame = check_frame_availibity(pt)) < 0)
        {
            //printf("DEBUG:alg is %s \n ", alg);

            // int frame_to_replace
            if (!strcmp(alg, "fifo") || !strcmp(alg, "custom"))
            {
                // fifo
                replace_page(pt, page);
                frame_counter++;
                frame_counter %= pt->nframes;
            }
            else if (!strcmp(alg, "rand"))
            {
                frame_counter = rand() % pt->nframes;
                replace_page(pt, page);
            }
        }
        else
        { // If a free frame is found insert into page table

            disk_read(disk, page, &pt->physmem[frame * BLOCK_SIZE]);
            disk_reads++;

            page_table_set_entry(pt, page, frame, PROT_READ);

            // update frame table with page for replacement policies
            frames[frame] = page;
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
        printf("couldn't create frame state list\n");
    }
    for (int i = 0; i < nframes; i++)
    {
        frames[i] = -1;
    }

    disk = disk_open("myvirtualdisk", npages);

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

    // char *physmem = page_table_get_physmem(pt);

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

    printf("Summary: Page Faults %d, Disk Reads %d, Disk Writes %d \n", page_faults, disk_reads, disk_writes);
    page_table_delete(pt);
    disk_close(disk);

    return 0;
}