CFLAGS = -g -Iinclude
EXECUTABLE = poolTest 

poolTest: poolTest.o cqueue.o spinlock.o pool.o
	gcc -o ${EXECUTABLE} ${CFLAGS} -pthread poolTest.o cqueue.o spinlock.o pool.o

poolTest.o: poolTest.c pool.h
	gcc -c ${CFLAGS} poolTest.c

pool.o: pool.c pool.h cqueue.h 
	gcc -c ${CFLAGS} pool.c

cqueue.o: cqueue.c cqueue.h spinlock.h
	gcc -c ${CFLAGS} cqueue.c

spinlock.o: spinlock.c spinlock.h
	gcc -c ${CFLAGS} spinlock.c 

clean:
	rm -f *.o poolTest
	rm -f core*