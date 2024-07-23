#ifndef DUMALLOC_H
#define DUMALLOC_H

#define HEAP_SIZE (128*8) 
#define FIRST_FIT 0
#define BEST_FIT 1

void duManagedInitMalloc(int strategy);
void** duManagedMalloc(int size);
void duManagedFree(void** mptr);
void duMemoryDump();
void minorCollection();
void majorCollection();

#define Managed(p) (*p)
#define Managed_t(t) t*

#endif