#include "my_malloc.h"

#include <assert.h>
#include <pthread.h>
#include <unistd.h>

pthread_mutex_t lock;
unsigned long data_segment_size = 0;
size_t blockSize = sizeof(block_tag);

void printList(void) {
    block_tag *cur = head;
    printf("printing the free list. blocksize is %lu\n", blockSize);
    printf("head : %p, tail : %p\n", head, tail);
    while (cur) {
        printf("[add:%p size:%lu]-->\n", cur, cur->size);
        cur = cur->next;
    }
    printf("\n");
}

// void *ff_malloc(size_t size) {
//     block_tag *blockFound = ff_findBlock(size);
//     if (blockFound != NULL) {  // valid block found
//         // but need to see if splitting is needed
//         if ((blockFound->size - size) <= sizeof(block_tag)) {
//             if (blockFound != head && blockFound != tail) {
//                 blockFound->prev->next = blockFound->next;
//                 blockFound->next->prev = blockFound->prev;
//             } else if (blockFound == head && blockFound == tail) {
//                 head = NULL;
//                 tail = NULL;
//             } else if (blockFound == head) {  // head
//                 head = blockFound->next;
//                 // blockFound->next->prev = NULL;
//                 blockFound->next->prev = NULL;
//             } else if (blockFound == tail) {  // tail
//                 tail = blockFound->prev;
//                 blockFound->prev->next = NULL;
//             }
//             blockFound->prev = NULL;
//             blockFound->next = NULL;
//             return (void *)blockFound + blockSize;
//         } else {
//             // printf("******%lu, %lu*******", (blockFound->size - size), sizeof(block_tag));
//             // split(blockFound, size);

//             block_tag *pointer = (block_tag *)((char *)blockFound + blockSize + blockFound->size - size - blockSize);
//             blockFound->size = blockFound->size - size - blockSize;
//             pointer->next = NULL;
//             pointer->prev = NULL;
//             pointer->size = size;

//             return (void *)((char *)pointer + blockSize);
//         }
//         //        printf("After mallocing for size %lu, (reused!!), the list is:\n", size);
//         //        printList();
//     } else {  // no valid block found

//         block_tag *newBlock = block_new(size);
//         if (newBlock == NULL) {  // sbrk failed
//             return NULL;
//         }

//         return (void *)newBlock + blockSize;
//     }
// }

void ts_free_lock(void *ptr) {
    pthread_mutex_lock(&lock);
    if (ptr == NULL) {
        pthread_mutex_unlock(&lock);
        return;
    }
    block_tag *block_freed = (block_tag *)((char *)ptr - sizeof(block_tag));  //?

    block_tag *curr = head;

    while (block_freed > curr && curr != NULL) {
        curr = curr->next;
    }
    if (curr != NULL) {
        // if curr not head
        if (curr->prev != NULL) {
            block_freed->next = curr;
            block_freed->prev = curr->prev;
            block_freed->prev->next = block_freed;
            curr->prev = block_freed;

        }
        // if curr is head
        else {
            head = block_freed;
            block_freed->next = curr;
            block_freed->prev = NULL;
            curr->prev = head;
        }
        // merge
        merge(block_freed);
    } else {
        if (head == NULL) {
            head = block_freed;
            tail = block_freed;
            block_freed->next = NULL;
            block_freed->prev = NULL;
        } else {
            tail->next = block_freed;
            block_freed->prev = tail;
            tail = block_freed;
            block_freed->next = NULL;
            if ((char *)block_freed->prev + blockSize + block_freed->prev->size == (char *)block_freed) {
                block_freed->prev->size += (block_freed->size + blockSize);
                tail = block_freed->prev;
                block_freed->prev->next = NULL;
                block_freed->prev = NULL;
                block_freed->next = NULL;
            }
        }
    }
    pthread_mutex_unlock(&lock);
    return;
}

// Merging the adjacent free regions into a single free region of memory

block_tag *block_new(size_t size) {
    block_tag *newBlock = sbrk(size + blockSize);  // return the current location of the program break
    data_segment_size += size + blockSize;
    if (newBlock == (void *)-1)
        return NULL;
    // sbrk(size+sizeof(block_tag));
    newBlock->size = size;
    newBlock->prev = NULL;
    newBlock->next = NULL;

    return newBlock;
}

void merge(block_tag *block) {
    if (block == NULL) {
        return;
    }

    if (block->prev && block->next && ((char *)block == (char *)block->prev + blockSize + block->prev->size) && ((char *)block->next == (char *)block + block->size + blockSize)) {
        block_tag *newBlock = block->prev;
        newBlock->size = block->size + block->prev->size + block->next->size + 2 * blockSize;

        if (block->next->next == NULL) {
            tail = newBlock;
            newBlock->next = NULL;
        } else {
            newBlock->next = block->next->next;
            block->next->next->prev = newBlock;
        }
        block->next->prev = NULL;
        block->next->next = NULL;
        block->prev = NULL;
        block->next = NULL;
    } else if (block->prev && (char *)block->prev + block->prev->size + blockSize == (char *)block) {
        block_tag *newBlock = block->prev;

        newBlock->size = block->size + block->prev->size + sizeof(block_tag);

        if (block->next == NULL) {
            tail = newBlock;
            newBlock->next = NULL;
        } else {
            newBlock->next = block->next;
            block->next->prev = newBlock;
        }
        block->next = NULL;
        block->prev = NULL;
    } else if ((block->next) && ((char *)block + block->size + sizeof(block_tag) == (char *)block->next)) {
        block_tag *newBlock = block;
        newBlock->size = block->size + block->next->size + sizeof(block_tag);
        if (block->next->next == NULL) {
            tail = block;
            block->next->prev = NULL;
            block->next->next = NULL;
            block->next = NULL;
        } else {
            block_tag *temp = block->next;
            block->next->next->prev = newBlock;
            newBlock->next = block->next->next;
            temp->prev = NULL;
            temp->next = NULL;
        }
    } else {
        return;
    }
}

block_tag *ff_findBlock(size_t size) {
    block_tag *ptr = head;
    while (ptr != NULL) {
        if (ptr->size >= size) {
            return ptr;
        } else {
            ptr = ptr->next;
        }
    }
    return NULL;  // Not found
}

block_tag *bf_findBlock(size_t size) {
    block_tag *ptr = head;
    size_t min = (size_t)-1;
    block_tag *curr = NULL;
    while (ptr != NULL) {
        if (ptr == ptr->next) {
            break;
        }
        if (ptr->size >= size && (ptr->size - size) < min) {
            if (ptr->size == size) {
                return ptr;
            }

            min = ptr->size - size;
            curr = ptr;
        } else {
            ptr = ptr->next;
        }
    }
    return curr;
}

void *ts_malloc_lock(size_t size) {
    pthread_mutex_lock(&lock);
    block_tag *blockFound = bf_findBlock(size);
    if (blockFound != NULL) {  // valid block found
        // but need to see if splitting is needed
        if ((blockFound->size - size) <= sizeof(block_tag)) {
            if (blockFound != head && blockFound != tail) {
                blockFound->prev->next = blockFound->next;
                blockFound->next->prev = blockFound->prev;
            } else if (blockFound == head && blockFound == tail) {
                head = NULL;
                tail = NULL;
            } else if (blockFound == head) {  // head
                head = blockFound->next;
                // blockFound->next->prev = NULL;
                blockFound->next->prev = NULL;
            } else if (blockFound == tail) {  // tail
                tail = blockFound->prev;
                blockFound->prev->next = NULL;
            }
            blockFound->prev = NULL;
            blockFound->next = NULL;

            pthread_mutex_unlock(&lock);
            return (void *)blockFound + blockSize;
        } else {
            block_tag *pointer = (block_tag *)((char *)blockFound + blockSize + blockFound->size - size - blockSize);
            blockFound->size = blockFound->size - size - blockSize;
            pointer->next = NULL;
            pointer->prev = NULL;
            pointer->size = size;

            pthread_mutex_unlock(&lock);
            return (void *)((char *)pointer + blockSize);
        }

    } else {
        block_tag *newBlock = block_new(size);
        if (newBlock == NULL) {  // sbrk failed

            pthread_mutex_unlock(&lock);
            return NULL;
        }

        pthread_mutex_unlock(&lock);
        return (void *)newBlock + blockSize;
    }
}

unsigned long get_data_segment_size() {
    return data_segment_size;
}
unsigned long get_data_segment_free_space_size() {
    block_tag *curr = head;
    unsigned long freesize = 0;
    while (curr) {
        freesize += curr->size + blockSize;
        curr = curr->next;
    }
    return freesize;
}

void *ts_malloc_nolock(size_t size) {
    block_tag *blockFound = bf_findBlock_nolock(size);
    if (blockFound != NULL) {  // valid block found
        // but need to see if splitting is needed
        if ((blockFound->size - size) <= sizeof(block_tag)) {
            if (blockFound != head_nolock && blockFound != tail_nolock) {
                blockFound->prev->next = blockFound->next;
                blockFound->next->prev = blockFound->prev;
            } else if (blockFound == head_nolock && blockFound == tail_nolock) {
                head_nolock = NULL;
                tail_nolock = NULL;
            } else if (blockFound == head_nolock) {  // head
                head_nolock = blockFound->next;
                // blockFound->next->prev = NULL;
                blockFound->next->prev = NULL;
            } else if (blockFound == tail_nolock) {  // tail
                tail_nolock = blockFound->prev;
                blockFound->prev->next = NULL;
            }
            blockFound->prev = NULL;
            blockFound->next = NULL;

            return (void *)blockFound + blockSize;
        } else {
            block_tag *pointer = (block_tag *)((char *)blockFound + blockSize + blockFound->size - size - blockSize);
            blockFound->size = blockFound->size - size - blockSize;
            pointer->next = NULL;
            pointer->prev = NULL;
            pointer->size = size;

            return (void *)((char *)pointer + blockSize);
        }

    } else {
        block_tag *newBlock = block_new_nolock(size);
        if (newBlock == NULL) {  // sbrk failed

            return NULL;
        }

        return (void *)newBlock + blockSize;
    }
}

block_tag *bf_findBlock_nolock(size_t size) {
    block_tag *ptr = head_nolock;
    size_t min = (size_t)-1;
    block_tag *curr = NULL;
    while (ptr != NULL) {
        if (ptr == ptr->next) {
            break;
        }
        if (ptr->size >= size && (ptr->size - size) < min) {
            if (ptr->size == size) {
                return ptr;
            }

            min = ptr->size - size;
            curr = ptr;
        } else {
            ptr = ptr->next;
        }
    }
    return curr;
}

block_tag *block_new_nolock(size_t size) {
    pthread_mutex_lock(&lock);
    block_tag *newBlock = sbrk(size + blockSize);  // return the current location of the program break
    data_segment_size += size + blockSize;
    if (newBlock == (void *)-1) {
        pthread_mutex_unlock(&lock);
        return NULL;
    }

    // sbrk(size+sizeof(block_tag));
    newBlock->size = size;
    newBlock->prev = NULL;
    newBlock->next = NULL;

    pthread_mutex_unlock(&lock);
    return newBlock;
}

void ts_free_nolock(void *ptr) {
    if (ptr == NULL) {
        return;
    }
    block_tag *block_freed = (block_tag *)((char *)ptr - sizeof(block_tag));  //?

    block_tag *curr = head_nolock;

    while (block_freed > curr && curr != NULL) {
        curr = curr->next;
    }
    if (curr != NULL) {
        // if curr not head
        if (curr->prev != NULL) {
            block_freed->next = curr;
            block_freed->prev = curr->prev;
            block_freed->prev->next = block_freed;
            curr->prev = block_freed;

        }
        // if curr is head
        else {
            head_nolock = block_freed;
            block_freed->next = curr;
            block_freed->prev = NULL;
            curr->prev = head_nolock;
        }
        // merge
        merge_nolock(block_freed);
    } else {
        if (head_nolock == NULL) {
            head_nolock = block_freed;
            tail_nolock = block_freed;
            block_freed->next = NULL;
            block_freed->prev = NULL;
        } else {
            tail_nolock->next = block_freed;
            block_freed->prev = tail_nolock;
            tail_nolock = block_freed;
            block_freed->next = NULL;
            if ((char *)block_freed->prev + blockSize + block_freed->prev->size == (char *)block_freed) {
                block_freed->prev->size += (block_freed->size + blockSize);
                tail_nolock = block_freed->prev;
                block_freed->prev->next = NULL;
                block_freed->prev = NULL;
                block_freed->next = NULL;
            }
        }
    }
    return;
}

void merge_nolock(block_tag *block) {
    if (block == NULL) {
        return;
    }

    if (block->prev && block->next && ((char *)block == (char *)block->prev + blockSize + block->prev->size) && ((char *)block->next == (char *)block + block->size + blockSize)) {
        block_tag *newBlock = block->prev;
        newBlock->size = block->size + block->prev->size + block->next->size + 2 * blockSize;

        if (block->next->next == NULL) {
            tail_nolock = newBlock;
            newBlock->next = NULL;
        } else {
            newBlock->next = block->next->next;
            block->next->next->prev = newBlock;
        }
        block->next->prev = NULL;
        block->next->next = NULL;
        block->prev = NULL;
        block->next = NULL;
    } else if (block->prev && (char *)block->prev + block->prev->size + blockSize == (char *)block) {
        block_tag *newBlock = block->prev;

        newBlock->size = block->size + block->prev->size + sizeof(block_tag);

        if (block->next == NULL) {
            tail_nolock = newBlock;
            newBlock->next = NULL;
        } else {
            newBlock->next = block->next;
            block->next->prev = newBlock;
        }
        block->next = NULL;
        block->prev = NULL;
    } else if ((block->next) && ((char *)block + block->size + sizeof(block_tag) == (char *)block->next)) {
        block_tag *newBlock = block;
        newBlock->size = block->size + block->next->size + sizeof(block_tag);
        if (block->next->next == NULL) {
            tail_nolock = block;
            block->next->prev = NULL;
            block->next->next = NULL;
            block->next = NULL;
        } else {
            block_tag *temp = block->next;
            block->next->next->prev = newBlock;
            newBlock->next = block->next->next;
            temp->prev = NULL;
            temp->next = NULL;
        }
    } else {
        return;
    }
}