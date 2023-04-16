#ifndef MY_MALLOC_H
#define MY_MALLOC_H

#include <stdio.h>
#include <stdlib.h>

typedef struct block block_tag;

struct block {  // only record free blocks in linked list
    size_t size;
    struct block* next;
    struct block* prev;
};

// First Fit malloc/free
//  void *ff_malloc(size_t size);
//  void ff_free(void *ptr);

// Best Fit malloc/free
void* ts_malloc_lock(size_t size);
void ts_free_lock(void* ptr);

void* ts_malloc_nolock(size_t size);
void ts_free_nolock(void* ptr);

unsigned long get_data_segment_size();             // in bytes
unsigned long get_data_segment_free_space_size();  // in byte

block_tag* block_new(size_t size);
void merge(block_tag* block);
block_tag* ff_findBlock(size_t size);
block_tag* bf_findBlock(size_t size);
//void split(block_tag * ptr, size_t size);
void printList(void);

block_tag* head = NULL;   // head of linked list
block_tag* tail = NULL;   // tail of linked list
block_tag* start = NULL;  // start address

__thread block_tag* head_nolock = NULL;
__thread block_tag* tail_nolock = NULL;
block_tag* block_new_nolock(size_t size);
void merge_nolock(block_tag* block);
block_tag* bf_findBlock_nolock(size_t size);
// void split_nolock(block_tag * ptr, size_t size);
#endif