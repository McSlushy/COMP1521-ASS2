
// COMP1521 18s1 Assignment 2
// Implementation of heap management system
// Completed by Johannes So (z5164638) 13/5/2018

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "myHeap.h"

// minimum total space for heap
#define MIN_HEAP  4096
// minimum amount of space for a free Chunk (includes\\\ Header)
#define MIN_CHUNK 32

#define ALLOC     0x55555555
#define FREE      0xAAAAAAAA

typedef unsigned int uint;                                                  // counters, bit-strings, ...

typedef void *Addr;                                                         // addresses

typedef struct {                                                            // headers for Chunks
    uint  status;                                                           // status (ALLOC or FREE)
    uint  size;                                                             // #bytes, including header
} Header;

static Addr  heapMem;                                                       // space allocated for Heap
static int   heapSize;                                                      // number of bytes in heapMem
static Addr *freeList;                                                      // array of pointers to free chunks
static int   freeElems;                                                     // number of elements in freeList[]
static int   nFree;                                                         // number of free chunks

static void sortFreeList();
static int findSmallestChunk(int size);
static void organiseNULL(int index);
static int findAddressInFreeList(void *address);
static void adjacentLeft(int index, void *address);
static void adjacentRight(int index, void *address);

// initialise heap
int initHeap(int size) {
    if (size < MIN_HEAP) size = MIN_HEAP;                                   // set size to minimum heap size if less than it
    int remainder = size % 4;
    if (remainder != 0) size = size + 4 - remainder;                        // round up to nearest multiple of 4 if not divisible by 4
    
    heapMem = (char *)malloc(size*sizeof(char));                            // set heapMem to first byte of malloc'd region
    if (heapMem == NULL) return -1;
    memset((char *)heapMem,'\0',size);                                      // zeroes out entire region
    heapSize = size;
    
    freeList = malloc((size/MIN_CHUNK)*sizeof(Addr));                       // allocate freeList array
    if (freeList == NULL) return -1;
    freeElems = size/MIN_CHUNK;
    for (int i = 0; i < freeElems; i++) {                                   // initialise freeList array to NULL addresses
        freeList[i] = NULL;
    }
    
    Header *newHeader = (Header *)heapMem;                                  // initialise region to be a single large free-space chunk
    newHeader->status = FREE;
    newHeader->size = size;
    freeList[0] = heapMem;                                                  // set first item in freeList array to the single free-space chunk
    nFree = 1;
    
    return 0;
}

// clean heap
void freeHeap() {
    free(heapMem);
    free(freeList);
}

// allocate a chunk of memory
void *myMalloc(int size) {
    if (size < 1) return NULL;                                              // cannot malloc using zero or negative values
    int remainder = size % 4;
    if (remainder != 0) size = size + 4 - remainder;                        // round up to nearest multiple of 4 if not divisible by 4
    int index = findSmallestChunk(size);                                    // search for smallest usable free-space chunk if any
    if (index == -1) return NULL;                                           // cannot malloc if only inadequately sized chunks available
    
    Addr curr = heapMem;
    Header *temp = (Header *)freeList[index];
    if (temp->size <= (size + 8)) {                                         // allocate entire chunk if there would be no excess free space left over
        uint oldSize = temp->size;                                          // get size of the chunk to be malloced
        uint offset = (uint) heapOffset(freeList[index]);                   // get address offset of chunk from heapMem
        curr = (Addr) ((char *)curr + offset);                              // add offset to get address of chunk
        Header *newHeader = (Header *)curr;
        newHeader->status = ALLOC;
        newHeader->size = oldSize;
        freeList[index] = NULL;
        nFree--;
        organiseNULL(index);                                                // move new NULL entry to back of array to retain sorted ascending address order
    } else {                                                                // split large chunk into a allocated chunk for the request the rest as free space
        uint freeSize = temp->size - (size + 8);                            // calculate free space excess from split
        uint offset = (uint) heapOffset(freeList[index]);                   // get address offset of malloced lower chunk from heapMem
        curr = (Addr) ((char *)curr + offset);                              // add offset to get address of malloced lower chunk
        Header *newHeader = (Header *)curr;
        newHeader->status = ALLOC;
        newHeader->size = size + 8;
        
        Addr curr2 = (Addr) ((char *)curr + (size + 8));                    // add size of lower chunk to get address of upper chunk
        Header *newHeader2 = (Header *)curr2;
        newHeader2->status = FREE;
        newHeader2->size = freeSize;
        freeList[index] = curr2;                                            // replace old free space chunk with the new one
        sortFreeList();                                                     // sort the freeList array to ensure ascending address order
    }
    
    return curr + 8;
}

// free a chunk of memory
void myFree(void *block) {
    block = (Addr) ((char *)block - 8);
    Header *temp = (Header *)block;
    if (block == NULL || temp->status != ALLOC) {
        fprintf(stderr,"Attempt to free unallocated chunk\n");              // return error if block is an allocated chunk or if the address is not the start of a data block
        exit(1);
    }
    
    temp->status = FREE;                                                    // release allocated chunk
    freeList[nFree] = block;                                                // place new free chunk into freeList array
    nFree++;
    sortFreeList();                                                         // sort the freeList array to ensure ascending address order
    int index = findAddressInFreeList(block);
    
    if (nFree != 1) {
        if (index == 0) {
            adjacentRight(index,block);
        } else if (index == (nFree - 1)) {
            adjacentLeft(index,block);
        } else {
            adjacentRight(index,block);
            adjacentLeft(index,block);
        }
    }
}

// convert pointer to offset in heapMem
int  heapOffset(void *p) {
    Addr heapTop = (Addr)((char *)heapMem + heapSize);
    if (p == NULL || p < heapMem || p >= heapTop)
        return -1;
    else
        return p - heapMem;
}

// dump contents of heap (for testing/debugging)
void dumpHeap() {
    Addr    curr;
    Header *chunk;
    Addr    endHeap = (Addr)((char *)heapMem + heapSize);
    int     onRow = 0;

    curr = heapMem;
    while (curr < endHeap) {
        char stat;
        chunk = (Header *)curr;
        switch (chunk->status) {
        case FREE:  stat = 'F'; break;
        case ALLOC: stat = 'A'; break;
        default:    fprintf(stderr,"Corrupted heap %08x\n",chunk->status); exit(1); break;
        }
        printf("+%05d (%c,%5d) ", heapOffset(curr), stat, chunk->size);
        onRow++;
        if (onRow%5 == 0) printf("\n");
        curr = (Addr)((char *)curr + chunk->size);
    }
    if (onRow > 0) printf("\n");
}

// uses a simple Bubble Sort to sort freeList in ascending address order
static void sortFreeList() {
    Addr tmp;

    for (int i = 0; i < nFree; i++) {
        for (int j = 0; j < (nFree-i); j++) {
            if (freeList[j] != NULL && freeList[j+1] != NULL) {
                if (freeList[j] > freeList[j+1]) {
                    tmp = freeList[j];
                    freeList[j] = freeList[j+1];
                    freeList[j+1] = tmp;
                }
            }
        }
    }
}

// returns the index of the samllest usable chunk in FreeList, if none can be found -1 is returned instead
static int findSmallestChunk(int size) {
    int index = -1;
    Header *temp, *check;
    
    for (int i = 0; i < nFree; i++) {
        temp = (Header *)freeList[i];
        if (temp->size >= (size + 8)) {
            if (index == -1 || check->size > temp->size) {
                index = i;
                check = (Header *)freeList[index];
            }
        }
    }
    
    return index;
}

// given index of a NULL address, this function moves it to the back of the freeList array
static void organiseNULL(int index) {
    for (int i = index; i < freeElems; i++) {
        if (freeList[i+1] == NULL) break;
        freeList[i] = freeList[i+1];
        freeList[i+1] = NULL;
    }
}

// given the start address of a free-space chunk, return its index in the freeList array
static int findAddressInFreeList(void *address) {
    for (int i = 0; i < nFree; i++) {
        Addr curr = heapMem;
        uint offset = (uint) heapOffset(freeList[i]);                       // get address offset of chunk from heapMem
        curr = (Addr) ((char *)curr + offset);                              // add offset to get address of chunk
        if (curr == address) return i;
    }
    
    return 0;
}

// check if the next free chunk on the left of the given free chunk is adjacent to it, and if so, merge them
static void adjacentLeft(int index, void *address) {
    Addr curr = heapMem;
    uint offset = (uint) heapOffset(freeList[index-1]);                     // get address offset of chunk from heapMem
    curr = (Addr) ((char *)curr + offset);                                  // add offset to get address of chunk
    uint difference = (uint) (address - curr);                              // get offset between the base addresses of the chunks
    
    Header *temp = (Header *)curr, *base = (Header *)address;
    if (temp->size == difference) {
        temp->size += base->size;                                           // merge free chunks together
        freeList[index] = NULL;                                             // remove reference to adjacent chunk
        nFree--;
        organiseNULL(index);                                                // move new NULL entry to back of array to retain sorted ascending address order
        address = curr;                                                     // set address to the address of the previous left adjacent chunk
    }
}

// check if the next free chunk on the right of the given free chunk is adjacent to it, and if so, merge them
static void adjacentRight(int index, void *address) {
    Addr curr = heapMem;
    uint offset = (uint) heapOffset(freeList[index+1]);                     // get address offset of chunk from heapMem
    curr = (Addr) ((char *)curr + offset);                                  // add offset to get address of chunk
    uint difference = (uint) (curr - address);                              // get offset between the base addresses of the chunks
    
    Header *temp = (Header *)curr, *base = (Header *)address;
    if (base->size == difference) {
        base->size += temp->size;                                           // merge free chunks together
        freeList[index+1] = NULL;                                           // remove reference to adjacent chunk
        nFree--;
        organiseNULL(index+1);                                              // move new NULL entry to back of array to retain sorted ascending address order
    }
}
