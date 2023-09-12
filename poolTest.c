#include "include/pool.h"
#include "include/rc.h"
#include <stdio.h>
#include <stdlib.h>

#define NUM_THREADS 10

rc_t myfun(void* arg, void** result) {
   *result = NULL;
   printf("Hello World\n");
   return Success;
}

int main(int argc, char* argv[]) {
   rc_t rc;
   thread_pool_t pool;

   rc = pool_create(&pool, NUM_THREADS);
   if (rc != Success) {
      fprintf(stderr, "The pool was not created successfully.\n");
      return rc;
   }

   void* args[NUM_THREADS*2];
   void* results[NUM_THREADS*2];

   int isNull = 0;
   rc = pool_map(&pool, myfun, 100, NULL, NULL);
   if (rc != Success) {
      fprintf(stderr, "The pool map was not successful. Error code=%d\n", rc);
      return rc;
   }

   rc = pool_destroy(&pool);
   if (rc != Success) {
      fprintf(stderr, "The pool was not destroyed successfully.\n");
      return rc;
   }

   fprintf(stderr, "Test 1 passed successfully");
}