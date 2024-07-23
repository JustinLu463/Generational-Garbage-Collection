mallocTest: mallocTest.o duMalloc.o
	gcc mallocTest.o duMalloc.o -o mallocTest

mallocTest.o: mallocTestVersion4.c dumalloc.h
	gcc mallocTestVersion4.c -c -o mallocTest.o

duMalloc.o: duMalloc.c dumalloc.h
	gcc duMalloc.c -c

clean:
	rm -f *.o
	rm -f mallocTest

