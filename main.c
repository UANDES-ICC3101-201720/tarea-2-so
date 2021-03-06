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

int page_faults, disk_reads, disk_writes; // Statistics
int *frame_table; // To keep track of the state of each frame
int *fifo_queue; // Helper array for fifo method, queue implemented as circular array
int queue_front; // Index of element in the front
int queue_count; // Number of elements in queue
int queue_len;   // Length of queue (= nframes)
struct disk *disk; // Globalized for easier use
char *physmem;     // Globalized for easier use
const char *method;
int last_custom;

void queue_append(int frame){
    fifo_queue[(queue_front + queue_count) % queue_len] = frame;
    queue_count++;
}

int queue_remove(){
    int frame = fifo_queue[queue_front];
    queue_front = (queue_front + 1) % queue_len;
    return frame;
}


/* Handle page faults with my custom method  */
void custom_method(struct page_table *pt, int page) {
    /* I'll use an algorithm inspired in the replacement policy called 'Second Chance'[1], but not quite it (at all).
     * This algorithm extends the FIFO method but gives a second chance to a page if it's in memory and written to by
     * putting it in the end of the queue.
     * In other words, we prefer to replace unmodified pages.
     * I call it "FIFO on Es"
     * [1]: http://www.mathcs.emory.edu/~cheung/Courses/355/Syllabus/9-virtual-mem/SC-replace.html */

    int frame = queue_remove();
    if (frame != last_custom){
//        printf("frame: %d\n", queue_front);
        queue_append(frame);
    }
    last_custom = frame;
    queue_front = (queue_front + 1) % queue_len;

}
/* Handle page faults with random method */
void rand_method(struct page_table *pt, int page) {
    int frame, prot_bits;
    /* Randomly choose a frame and get its page */
    int victim_page = frame_table[lrand48() % page_table_get_nframes(pt)];
    page_table_get_entry(pt, victim_page, &frame, &prot_bits);
    if (prot_bits & PROT_WRITE){ // If it was written, update disk with new value
        disk_write(disk, victim_page, &physmem[frame * PAGE_SIZE]);
        disk_writes++;
    }
    disk_read(disk, page, &physmem[frame * PAGE_SIZE]); // TODO: right?
    disk_reads++;
    page_table_set_entry(pt, page, frame, PROT_READ);
    page_table_set_entry(pt, victim_page, frame, 0);
    frame_table[frame] = page;
}

/* Handle page faults with FIFO method */
void fifo_method(struct page_table *pt, int page) {
    int frame, prot_bits;
    /* Since no frames available, do swapping with 'first in' */
    int front_page = frame_table[queue_remove()];
    /* Check if the page is dirty (W bit) and needs to be updated in disk */
    page_table_get_entry(pt, front_page, &frame, &prot_bits);
    if (prot_bits & PROT_WRITE){ // If it was written, update disk with new value
                disk_write(disk, front_page, &physmem[frame * PAGE_SIZE]);
                disk_writes++;
    }
    disk_read(disk, page, &physmem[frame * PAGE_SIZE]); // TODO: right?
    disk_reads++;
    page_table_set_entry(pt, page, frame, PROT_READ);
    page_table_set_entry(pt, front_page, frame, 0);
    frame_table[frame] = page; // important!

}

int available_frame(struct page_table *pt) {
    for (int i = 0; i < page_table_get_nframes(pt); ++i) {
        if (frame_table[i] == -1){ // frame available
            return i;
        }
    }
    return -1; // No frame available
}

void page_fault_handler( struct page_table *pt, int page )
{
    page_faults++;

    /* First obtain frame number and access permissions to check if page is in memory (R) */
    int frame, prot_bits;
    page_table_get_entry(pt, page, &frame, &prot_bits);
    if (prot_bits & PROT_READ){
        /* Page is in memory
         * Set W to be able to write in it*/
        page_table_set_entry(pt, page, frame, PROT_READ|PROT_WRITE);
        /***** CUSTOM METHOD *****/
        if(!strcmp(method,"custom")) custom_method(pt, page);
    } else {
        /* Page need to be loaded to memory
         * First: Check if there are frames available */
        frame = available_frame(pt);
        if (frame >= 0){
            frame_table[frame] = page;
            if(!strcmp(method,"fifo") || !strcmp(method,"custom")) queue_append(frame); // Only used in FIFO and custom method
            page_table_set_entry(pt, page, frame, PROT_READ);
            disk_read(disk, page, &physmem[frame * PAGE_SIZE]); // TODO: right?
            disk_reads++;

        } else { // No frame available
            /* PAGE FAULT HANDLER ALGORITHM */
            if(!strcmp(method,"fifo")) {
                fifo_method(pt, page);
            } else if(!strcmp(method,"rand")) {
                rand_method(pt, page);
            } else if(!strcmp(method,"custom")) {
                fifo_method(pt, page);
            }
        }

    }

}


int main( int argc, char *argv[] )
{
	if(argc!=5) {
		/* Add 'random' replacement algorithm if the size of your group is 3 */
		printf("use: virtmem <npages> <nframes> <rand|custom|fifo> <sort|scan|focus>\n");
		return 1;
	}

	int npages = atoi(argv[1]);
	int nframes = atoi(argv[2]);
	const char *program = argv[4];
    method = argv[3];

    last_custom = -1;
    page_faults = 0;
    disk_writes = 0;
    disk_reads = 0;
    frame_table = malloc(nframes * sizeof(int));
    for (int i = 0; i < nframes; ++i) {
        frame_table[i] = -1; // Initialize frame_table with -1 (frame is free)
    }
    /* If FIFO or custom will be used, initialize its helpers */
    if(!strcmp(method,"fifo") || !strcmp(method,"custom")) {
        fifo_queue = malloc(nframes * sizeof(int)); // At max we will nframes in queue
        queue_front = -1; queue_count = 0; queue_len = nframes;
    }

	disk = disk_open("myvirtualdisk",npages);
	if(!disk) {
		fprintf(stderr,"couldn't create virtual disk: %s\n",strerror(errno));
		return 1;
	}

	struct page_table *pt = page_table_create( npages, nframes, page_fault_handler );
	if(!pt) {
		fprintf(stderr,"couldn't create page table: %s\n",strerror(errno));
		return 1;
	}

	char *virtmem = page_table_get_virtmem(pt);

	physmem = page_table_get_physmem(pt);

	if(!strcmp(program,"sort")) {
		sort_program(virtmem,npages*PAGE_SIZE);

	} else if(!strcmp(program,"scan")) {
		scan_program(virtmem,npages*PAGE_SIZE);

	} else if(!strcmp(program,"focus")) {
		focus_program(virtmem,npages*PAGE_SIZE);

	} else {
		fprintf(stderr,"unknown program: %s\n",argv[3]);

	}

	printf("# page faults: %d\n# disk reads: %d\n# disk writes: %d\n", page_faults, disk_reads, disk_writes);

	page_table_delete(pt);
	disk_close(disk);
	free(frame_table);
    if(!strcmp(method,"fifo") || !strcmp(method,"custom")) free(fifo_queue);

	return 0;
}
