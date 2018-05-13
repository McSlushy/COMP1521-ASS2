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

typedef unsigned int uint;                                      // counters, bit-strings, ...

typedef void *Addr;                                             // addresses

typedef struct {                                                // headers for Chunks
    uint  status;                                               // status (ALLOC or FREE)
    uint  size;                                                 // #bytes, including header
} Header;

static Addr  heapMem;                                           // space allocated for Heap
static int   heapSize;                                          // number of bytes in heapMem
static Addr *freeList;                                          // array of pointers to free chunks
static int   freeElems;                                         // number of elements in freeList[]
static int   nFree;                                             // number of free chunks

static void sortFreeList();
static int findSmallestChunk(int size);

// initialise heap
int initHeap(int size) {
    if (size < MIN_HEAP) size = MIN_HEAP;                       // set size to minimum heap size if less than it
    int remainder = size % 4;
    if (remainder != 0) size = size + 4 - remainder;            // round up to nearest multiple of 4 if not divisible by 4
    
    (char *)heapMem = (char *)malloc(size*sizeof(char));        // set heapMem to first byte of malloc'd region
    if (heapMem == NULL) return -1;
    memset((char *)heapMem,'\0',size);                          // zeroes out entire region
    heapSize = size;
    
    freeList = malloc((size/MIN_CHUNK)*sizeof(Addr));           // allocate freeList array
    if (freeList == NULL) return -1;
    freeElems = size/MIN_CHUNK;
    for (int i = 0; i < freeElems; i++) {                       // initialise freeList array to NULL
        freeList[i] = NULL;
    }
    
    struct Header heap = {FREE,size};                           // initialise region to be a single large free-space chunk
    (Header *)freeList[0] = heap;                               // set first item in freeList array to the single free-space chunk
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
    if (size < 1) return NULL;
    int remainder = size % 4;
    if (remainder != 0) size = size + 4 - remainder;
    int index = findSmallestChunk(size);
    if (index == -1) return NULL;
    
    if (freeList[index]->size <= (size + 8 + MIN_CHUNK) {       // allocate entire chunk if there would be no excess free space left over
        struct Header newAlloc = {ALLOC,freeList[index]->size};
        (Header *)freeList[index] = newAlloc;
    } else {
        int freeSize = freeList[index]->size - (size + 8 + MIN_CHUNK);
        struct Header newAlloc = {ALLOC,(size + 8 + MIN_CHUNK)};
        (Header *)freeList[index] = newAlloc;                   // allocate lower chunk for request
        
    }
}

// free a chunk of memory
void myFree(void *block) {
    
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
    int address1, address2;

    for (int i = 0; i < nFree; i++) {
        for (int j = 0; j < (nFree-i); j++) {
            if (freeList[j] != NULL && freeList[j+1] != NULL) {
                address1 = &freeList[j];                        // get address of header
                address2 = &freeList[j+1];                      // get address of header
                if (address1 > address2) {
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
    
    for (int i = 0; i < nFree; i++) {
        if (freeList[i]->size >= (size + 8)) {
            if (index == -1) index = i;
            else if (freeList[index]->size > freeList[i]->size) index = i;
        }
    }
    
    return index;
}
