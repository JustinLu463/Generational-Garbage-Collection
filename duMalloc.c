#include "dumalloc.h"
#include <stdio.h>
#include <string.h>

unsigned char heaps[3][HEAP_SIZE];  // Three heaps: two young and one old generation
int currentHeap = 0;
int allocationStrategy;

typedef struct memoryBlockHeader {
    int free; // 0 - used, 1 = free
    int size; // Size of the reserved block
    int managedIndex; // The unchanging index in the managed array
    int survivalAmt; // The number of times the block has moved between young heaps
    struct memoryBlockHeader* next;  // The next block in the integrated free list
} memoryBlockHeader;

memoryBlockHeader* freeListHeads[3];  // Free list heads for all three heaps

#define MANAGED_LIST_SIZE (HEAP_SIZE / 8)
void* managedList[MANAGED_LIST_SIZE];
int managedListCounter;

void duInitMalloc(int strategy) {
    // Zero out the entire heap
    for (int i = 0; i < HEAP_SIZE; i++) {
        heaps[0][i] = 0;
        heaps[1][i] = 0;
        heaps[2][i] = 0;
    }

    // Set up the initial free block in all heaps
    for (int i = 0; i < 3; i++) {
        memoryBlockHeader* initialBlock = (memoryBlockHeader*) heaps[i];
        initialBlock->free = 1;
        initialBlock->size = HEAP_SIZE - sizeof(memoryBlockHeader);
        initialBlock->next = NULL;
        initialBlock->managedIndex = -1;
        initialBlock->survivalAmt = 0;

        // Set the head of the free list to point to the initial block
        freeListHeads[i] = initialBlock;
        printf("duInitMalloc: Initialized heap %d with free block at %p, size %d\n", i, (void*)initialBlock, initialBlock->size);
    }

    // Set the allocation strategy
    allocationStrategy = strategy;
}

void duManagedInitMalloc(int strategy) {
    duInitMalloc(strategy);

    for (int i = 0; i < MANAGED_LIST_SIZE; i++) {
        managedList[i] = NULL;
    }

    managedListCounter = 0;
}

void printMemoryBlocks(int heapIndex) {
    printf("Memory Block\n");
    unsigned char* current = heaps[heapIndex];
    while (current < heaps[heapIndex] + HEAP_SIZE) {
        memoryBlockHeader* block = (memoryBlockHeader*) current;
        printf("%s at %p, size %d, surv: %d\n", block->free ? "Free" : "Used", (void*)block, block->size, block->survivalAmt);
        current += sizeof(memoryBlockHeader) + block->size;
    }

    printf("\nGraphical Representation:\n");
    char memoryRepresentation[HEAP_SIZE / 8 + 1];
    memoryRepresentation[HEAP_SIZE / 8] = '\0';

    current = heaps[heapIndex];
    int index = 0;
    char freeMarker = 'a';
    char usedMarker = 'A';
    while (current < heaps[heapIndex] + HEAP_SIZE) {
        memoryBlockHeader* block = (memoryBlockHeader*) current;
        char marker = block->free ? freeMarker++ : usedMarker++;
        for (int i = 0; i < (sizeof(memoryBlockHeader) + block->size) / 8; i++) {
            memoryRepresentation[index++] = marker;
        }
        current += sizeof(memoryBlockHeader) + block->size;
    }
    printf("%s\n", memoryRepresentation);
}

void printFreeList(int heapIndex) {
    printf("Free List\n");
    for (memoryBlockHeader* block = freeListHeads[heapIndex]; block != NULL; block = block->next) {
        printf("Block at %p, size %d\n", (void*)block, block->size);
    }
}

void printManagedList() {
    printf("Managed List\n");
    for (int i = 0; i < managedListCounter; i++) {
        printf("ManagedList[%d] = %p\n", i, managedList[i]);
    }
}

void duMemoryDump() {
    printf("MEMORY DUMP\n");
    printf("Current heap (0/1 young): %d\n", currentHeap);
    printf("Young Heap (only the current one)\n");
    printMemoryBlocks(currentHeap);
    printFreeList(currentHeap);
    printf("Old Heap\n");
    printMemoryBlocks(2);  // Always print the old generation
    printFreeList(2);
    printManagedList();
}

void* duMalloc(int size, int heapIndex) {
    int actualSize = (size + 7) & ~7;

    memoryBlockHeader* bestBlock = NULL;
    memoryBlockHeader* bestPrev = NULL;
    memoryBlockHeader* prev = NULL;

    for (memoryBlockHeader* block = freeListHeads[heapIndex]; block != NULL; block = block->next) {
        if (block->size >= actualSize + sizeof(memoryBlockHeader)) {
            if (allocationStrategy == FIRST_FIT) {
                bestBlock = block;
                bestPrev = prev;
                break;
            } else if (allocationStrategy == BEST_FIT) {
                if (bestBlock == NULL || block->size < bestBlock->size) {
                    bestBlock = block;
                    bestPrev = prev;
                }
            }
        }
        prev = block;
    }

    if (bestBlock == NULL) {
        printf("duMalloc: No suitable block found in heap %d\n", heapIndex);
        return NULL; 
    }

    // Split the block if there's enough space for a new block header
    int remainingSize = bestBlock->size - actualSize - sizeof(memoryBlockHeader);
    if (remainingSize > 0) {
        memoryBlockHeader* newBlock = (memoryBlockHeader*)((unsigned char*)bestBlock + sizeof(memoryBlockHeader) + actualSize);
        newBlock->free = 1;
        newBlock->size = remainingSize;
        newBlock->next = bestBlock->next;
        newBlock->managedIndex = -1;
        newBlock->survivalAmt = 0;

        bestBlock->size = actualSize;
        bestBlock->next = newBlock;
    }

    // Update the free list
    if (bestPrev) {
        bestPrev->next = bestBlock->next;
    } else {
        freeListHeads[heapIndex] = bestBlock->next;
    }

    bestBlock->free = 0;
    printf("duMalloc: Allocated block of size %d in heap %d at address %p\n", size, heapIndex, (void*)((unsigned char*)bestBlock + sizeof(memoryBlockHeader)));
    return (void*)((unsigned char*)bestBlock + sizeof(memoryBlockHeader));
}

void** duManagedMalloc(int size) {
    void* ptr = duMalloc(size, currentHeap);
    if (!ptr) return NULL;

    managedList[managedListCounter] = ptr;

    // Find the corresponding block to set the managed index
    memoryBlockHeader* block = (memoryBlockHeader*)((unsigned char*)ptr - sizeof(memoryBlockHeader));
    block->managedIndex = managedListCounter;

    return &managedList[managedListCounter++];
}

void duFree(void* ptr) {
    if (!ptr) return;

    // Determine which heap the pointer belongs to
    int heapIndex;
    if (ptr >= (void*)heaps[2] && ptr < (void*)(heaps[2] + HEAP_SIZE)) {
        heapIndex = 2;  // Old heap
    } else if (ptr >= (void*)heaps[currentHeap] && ptr < (void*)(heaps[currentHeap] + HEAP_SIZE)) {
        heapIndex = currentHeap;  // Current young heap
    } else {
        printf("duFree: Pointer %p does not belong to any heap\n", ptr);
        return;
    }

    // Calculate the start of the block header
    memoryBlockHeader* block = (memoryBlockHeader*)((unsigned char*)ptr - sizeof(memoryBlockHeader));
    block->free = 1;

    // Find the correct place to insert the free block in memory order
    memoryBlockHeader* prev = NULL;
    memoryBlockHeader* current = freeListHeads[heapIndex];
    while (current != NULL && current < block) {
        prev = current;
        current = current->next;
    }

    block->next = current;

    if (prev) {
        prev->next = block;
    } else {
        freeListHeads[heapIndex] = block;
    }
}

void duManagedFree(void** mptr) {
    if (!mptr || !*mptr) return;

    void* ptr = *mptr;
    duFree(ptr);

    // Null out the managed list entry
    int index = ((memoryBlockHeader*)((unsigned char*)ptr - sizeof(memoryBlockHeader)))->managedIndex;
    if (index >= 0 && index < MANAGED_LIST_SIZE) {
        managedList[index] = NULL;
    }

    *mptr = NULL;
}

void minorCollection() {
    int newHeap = 1 - currentHeap;
    unsigned char* newHeapStart = heaps[newHeap];
    unsigned char* newHeapCurrent = newHeapStart;

    printf("Starting minor collection. Moving live objects from heap %d to heap %d.\n", currentHeap, newHeap);

    for (int i = 0; i < managedListCounter; i++) {
        if (managedList[i] != NULL) {
            memoryBlockHeader* oldBlock = (memoryBlockHeader*)((unsigned char*)managedList[i] - sizeof(memoryBlockHeader));
            int blockSize = sizeof(memoryBlockHeader) + oldBlock->size;
            oldBlock->survivalAmt++;

            if (oldBlock->survivalAmt >= 3) {
                // Move to the old heap
                void* newPtr = duMalloc(oldBlock->size, 2);
                if (newPtr) {
                    memcpy(newPtr, oldBlock + 1, oldBlock->size);

                    memoryBlockHeader* newBlock = (memoryBlockHeader*)((unsigned char*)newPtr - sizeof(memoryBlockHeader));
                    newBlock->next = NULL;
                    managedList[i] = newPtr;
                    printf("Moved block %d to old heap. New address: %p\n", i, newPtr);
                }
            } else {
                // Move to the other young heap
                memcpy(newHeapCurrent, oldBlock, blockSize);

                memoryBlockHeader* newBlock = (memoryBlockHeader*)newHeapCurrent;
                newBlock->next = NULL;
                managedList[i] = (void*)(newHeapCurrent + sizeof(memoryBlockHeader));
                printf("Moved block %d to new young heap. New address: %p\n", i, managedList[i]);

                newHeapCurrent += blockSize;
            }
        }
    }

    // Create a new free block for the remaining space in the new young heap
    if (newHeapCurrent < newHeapStart + HEAP_SIZE) {
        memoryBlockHeader* newFreeBlock = (memoryBlockHeader*)newHeapCurrent;
        newFreeBlock->free = 1;
        newFreeBlock->size = (newHeapStart + HEAP_SIZE) - newHeapCurrent - sizeof(memoryBlockHeader);
        newFreeBlock->next = NULL;
        freeListHeads[newHeap] = newFreeBlock;
    } else {
        freeListHeads[newHeap] = NULL;
    }

    currentHeap = newHeap;
}

void majorCollection() {
    unsigned char* oldHeapStart = heaps[2];
    unsigned char* oldHeapCurrent = oldHeapStart;

    printf("Starting major collection on old heap.\n");

    memoryBlockHeader* firstFreeBlock = freeListHeads[2];
    if (!firstFreeBlock) return;

    unsigned char* firstFree = (unsigned char*)firstFreeBlock;

    while (firstFree < oldHeapStart + HEAP_SIZE) {
        memoryBlockHeader* block = (memoryBlockHeader*)firstFree;

        if (block->free) {
            memoryBlockHeader* nextBlock = (memoryBlockHeader*)(firstFree + sizeof(memoryBlockHeader) + block->size);
            if ((unsigned char*)nextBlock >= oldHeapStart + HEAP_SIZE) break;

            if (nextBlock->free) {
                block->size += sizeof(memoryBlockHeader) + nextBlock->size;
                block->next = nextBlock->next;
            } else {
                memcpy(oldHeapCurrent, nextBlock, sizeof(memoryBlockHeader) + nextBlock->size);
                memoryBlockHeader* movedBlock = (memoryBlockHeader*)oldHeapCurrent;
                managedList[movedBlock->managedIndex] = (void*)(oldHeapCurrent + sizeof(memoryBlockHeader));
                movedBlock->next = NULL;
                oldHeapCurrent += sizeof(memoryBlockHeader) + movedBlock->size;

                block->next = nextBlock->next;
            }
        } else {
            firstFree += sizeof(memoryBlockHeader) + block->size;
        }
    }

    if (oldHeapCurrent < oldHeapStart + HEAP_SIZE) {
        memoryBlockHeader* newFreeBlock = (memoryBlockHeader*)oldHeapCurrent;
        newFreeBlock->free = 1;
        newFreeBlock->size = (oldHeapStart + HEAP_SIZE) - oldHeapCurrent - sizeof(memoryBlockHeader);
        newFreeBlock->next = NULL;
        freeListHeads[2] = newFreeBlock;
    }
}

