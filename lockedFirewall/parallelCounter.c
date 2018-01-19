#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "locks.c"
#include <string.h>
#include "stopwatch.c"
/*
 * This application launches n threads, which all try to grab the lock
 * and increment the counter. This code is configurable by:
 * – B (B) the number to which we are counting.
 * – numThreads (n) the total number of worker threads
 * – lockType (L) the lock algorithm
 */

typedef struct counter {
  int count;
  int max;
  lock_t *lock;
} counter_t;

typedef struct {
  int threadID;
  counter_t *counter;
} workerData;

void do_work(void *ptr) {
  workerData *data;
  data = (workerData *)ptr;
  counter_t *counter;
  counter = data->counter;

  int id = data->threadID;
  int slot = data->threadID;
  int internalCount = 0;
  slot = 0;
  //id = 0;
  void (*lockFunc)(void *lock, int *slotIndex, int id) = counter->lock->lock;
  void (*unlockFunc)(void *lock, int *slotIndex, int id) = counter->lock->unlock;
  while(1)
    {
      //      counter->lock->lock(counter->lock->self, &slot, id);
      lockFunc(counter->lock->self, &slot, id);
      if (counter->count == counter->max) {
	unlockFunc(counter->lock->self, &slot, id);
	//	printf("I am thread %d and counted %d\n", id, internalCount);
	pthread_exit(NULL);
      }
      (counter->count)++;
      internalCount++;
      unlockFunc(counter->lock->self, &slot, id);
      sched_yield();
    }
  
  pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
  const int b = atoi(argv[1]);
  const int n = atoi(argv[2]);
  char *l = argv[3];
  StopWatch_t watch;
  startTimer(&watch);
  lock_t *lock = new_lock(l, n);
 
  counter_t *counter = malloc(sizeof(counter_t));
  counter->count = 0;
  counter->max = b;
  counter->lock = lock;

  // make worker threads
  pthread_t workers[n];
  for (int i = 0; i < n; i++) {
    workerData *new_data = malloc(sizeof(workerData));
    new_data->threadID = i;
    new_data->counter = counter;
    pthread_create(&workers[i], NULL, (void *) &do_work, new_data);
      }

  for(int i = 0; i < n; i++) {
    pthread_join(workers[i], NULL);
  }
  stopTimer(&watch);
  printf("%f\n", getElapsedTime(&watch));
  //  printf("Counter result: %d\n", counter->count);
  return 1;
}
