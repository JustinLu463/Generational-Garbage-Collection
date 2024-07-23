// This is the test file to use for version4
// It moves blocks between young generations until some get moved to old
// Then it tests the major collection compaction on the old

#include <stdio.h>  // printf
#include <stdlib.h>  // exit

// Load in the dumalloc interface
// Will need to be compiled with the dumalloc code as well
//   but for this lab we use a makefile to create a library
//   and link with that.  See the makefile.
#include "dumalloc.h"

void test() {
	// Without the managed macros
	printf("\nduMalloc a0\n");
	char** a0 = (char**)duManagedMalloc(64);
	if (a0 == NULL) {
		printf("Call to DuMalloc failed\n");
		exit(1);
	}
	duMemoryDump();

	printf("\nduMalloc a1\n");
	Managed_t(char *) a1 = (Managed_t(char*))duManagedMalloc(48);
	if (a1 == NULL) {
		printf("Call to DuMalloc failed\n");
		exit(1);
	}
	duMemoryDump();

	// Test to store and retrieve data from a1
	Managed(a1)[0] = 'D';
	Managed(a1)[1] = 'e';
	Managed(a1)[2] = 'n';
	Managed(a1)[3] = 'v';
	Managed(a1)[4] = 'e';
	Managed(a1)[5] = 'r';
	Managed(a1)[6] = '\0';
	printf("\nMemory access is: %s\n", Managed(a1));

	printf("\nduMalloc a2\n");
	Managed_t(char*) a2 = (Managed_t(char*))duManagedMalloc(64);
	if (a2 == NULL) {
		printf("Call to DuMalloc failed\n");
		exit(1);
	}
	duMemoryDump();

	printf("\nduMalloc a3\n");
	Managed_t(char*) a3 = (Managed_t(char*))duManagedMalloc(24);
	if (a3 == NULL) {
		printf("Call to DuMalloc failed\n");
		exit(1);
	}
	duMemoryDump();

	printf("\nduMalloc a4\n");
	Managed_t(char*) a4 = (Managed_t(char*))duManagedMalloc(88);
	if (a4 == NULL) {
		printf("Call to DuMalloc failed\n");
		exit(1);
	}
	duMemoryDump();

	printf("\nduMalloc a5\n");
	Managed_t(char*) a5 = (Managed_t(char*))duManagedMalloc(80);
	if (a5 == NULL) {
		printf("Call to DuMalloc failed\n");
		exit(1);
	}
	duMemoryDump();

	// a1 doesn't use the managed macros
	printf("\nduFree a0\n");
	duManagedFree((void**)a0);
	duMemoryDump();

	printf("\nduFree a3\n");
	duManagedFree((Managed_t(void*))a3);
	duMemoryDump();

	printf("\nduMalloc a6\n");
	Managed_t(char*) a6 = (Managed_t(char*))duManagedMalloc(160);
	if (a6 == NULL) {
		printf("Call to DuMalloc failed\n");
		exit(1);
	}
	duMemoryDump();

	// Still alive: a1(48), a2(64), a4(88), a5(80), a6(160)
	
	// Do a minor collection
	printf("\n********* MINOR COLLECTION ***********\n");
	minorCollection();
	duMemoryDump();

	// See if the memory is still correct in a1 even though it moved
	printf("\nMemory access is: %s\n", Managed(a1));

	printf("\nduFree a2\n");
	duManagedFree((Managed_t(void*))a2);
	duMemoryDump();

	printf("\nduMalloc a7\n");
	Managed_t(char*) a7 = (Managed_t(char*))duManagedMalloc(16);
	if (a7 == NULL) {
		printf("Call to DuMalloc failed\n");
		exit(1);
	}
	duMemoryDump();

	// Blocks still alive are a1(48), a7(16), a4(88), a5(80), a6(160)

	// Do a minor collection
	printf("\n********* MINOR COLLECTION ***********\n");
	minorCollection();
	duMemoryDump();

	// This will flip the order to a1(48), a4(88), a5(80), a6(160), a7(16)
	// a7 is surv 1, the others are surv 2

	printf("\nduMalloc a8\n");
	Managed_t(char*) a8 = (Managed_t(char*))duManagedMalloc(56);
	if (a8 == NULL) {
		printf("Call to DuMalloc failed\n");
		exit(1);
	}
	duMemoryDump();

	// Still alive a1(48), a4(88), a5(80), a6(160), a7(16), a8(56)

	// Do a minor collection
	printf("\n********* MINOR COLLECTION ***********\n");
	minorCollection();
	duMemoryDump();

	// Moved to old: a1(48), a4(88), a5(80), a6(160)
	// young: a7(16) surv 2, a8(56) is surv 1

	printf("\nduFree a4\n");
	duManagedFree((Managed_t(void*))a4);
	duMemoryDump();

	printf("\nduFree a5\n");
	duManagedFree((Managed_t(void*))a5);
	duMemoryDump();

	// Do a major collection
	printf("\n********* MAJOR COLLECTION ***********\n");
	majorCollection();
	duMemoryDump();

	// Should be a1(48) and a6(160) still alive in old, but compacted.
	// a7(16), a8(56) in young

	// Do a memory check one last time
	printf("\nMemory access is: %s\n", Managed(a1));
}

int main(int argc, char* argv[]) {

	// Must be first call in the program to get DuMalloc going
	printf("\nduInitMalloc\n");
	duManagedInitMalloc(FIRST_FIT);
	duMemoryDump();

	test();
}
