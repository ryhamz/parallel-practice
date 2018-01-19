#include "locks.h"
#include <time.h>
#include <string.h>
#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

void lockTAS(void *lock, int *slotIndex, int id) {
  TASLock *l = (TASLock *) lock;
  while(  __sync_lock_test_and_set (&(l->state), 1)) {
  }
}

void unlockTAS(void *lock, int *slotIndex, int id) {
  TASLock *l = (TASLock *) lock;
  __sync_lock_release(&(l->state));
}

int tryLockTAS(void *lock, int *slotIndex, int id) {
  TASLock *l = (TASLock *) lock;
  if (l->state == 1) {
    return 1;
  }
  else {
    lockTAS(l, slotIndex, id);
    return 0;
  }
}

void backoff(int limit) {
  int delay = rand() % limit;
  usleep(delay );
}

void lockBackoff(void *lock, int *slotIndex, int id) {
  backoffLock *l = (backoffLock *)lock;
  int limit= 1;
  int maxDelay = 16;
  while(1) {
    //printf("trying to lock backoff lock\n");
    if( __sync_lock_test_and_set (&(l->state), 1)) {
      return;
    }
    else {
      backoff(limit);
      limit = MIN(maxDelay, 2 * limit);
    }
  }
}

void unlockBackoff(void *lock, int *slotIndex, int id) {
  backoffLock *l = (backoffLock *)lock;
  __sync_lock_release(&(l->state));
}

int tryLockBackoff(void *lock, int *slotIndex, int id) {
  backoffLock *l = (backoffLock *)lock;
  if (l->state == 1) {
    return 1;
  }
  else {
    lockBackoff(l, slotIndex, id);
    return 0;
  }
}

void lockMutex(void *lock, int *slotIndex, int id) {
  pthreadMutexLock *l = (pthreadMutexLock *)lock;
  pthread_mutex_lock(&(l->mut));
}

void unlockMutex(void *lock, int *slotIndex, int id) {
  pthreadMutexLock *l = (pthreadMutexLock *)lock;
  pthread_mutex_unlock(&(l->mut));
}

int tryLockMutex(void *lock, int *slotIndex, int id) {
  pthreadMutexLock *l = (pthreadMutexLock *)lock;
  return  pthread_mutex_trylock(&(l->mut));
}

void lockAnderson(void *lock, int *slotIndex, int id) {
  aLock *l = (aLock *)lock;
  //  printf("incrementing tail from %d to\n", (l->tail));
  int slot = (__atomic_fetch_add(&(l->tail), 1, __ATOMIC_SEQ_CST));
  //printf("%d\n", slot);
  int padCount = 64 / sizeof(int);
  slot = slot % ((l->n) * padCount);
  *slotIndex = slot;
  while (! ((l->flag)[slot])) {}
}

void unlockAnderson(void *lock, int *slotIndex, int id) {
  aLock *l = (aLock *)lock;
  int slot = *slotIndex;
  (l->flag)[slot] = 0;
  int padCount = 64 / sizeof(int);
  (l->flag)[(slot+1) % ((l->n) * padCount)] = 1;
}

//returns 0 if lock is acquired successfully.
int tryLockAnderson(void *lock, int *slotIndex, int id) {
  lockAnderson(lock, slotIndex, id);
  return 0;
}

void lockClh(void *lock, int *slotIndex, int id) {
  clhLock *l = (clhLock *)lock;
volatile qNode *myNode = l->myNodes[id];
  myNode->locked = 1;
  
  volatile qNode *pred;
  /*
  pred = l->tail;
  l->tail = myNode;
  */
  pred =  __sync_lock_test_and_set(&(l->tail), myNode);
  myNode->pred = pred;
  while(pred->locked) {
  }
}

void unlockClh(void *lock, int *slotIndex, int id) {
  clhLock *l = (clhLock *)lock;
  
  volatile qNode *myNode = l->myNodes[id];
  myNode->locked = 0;
  l->myNodes[id] = myNode->pred;
}

//Yes, this is a fake implementation, for now.
int tryLockClh(void *lock, int *slotIndex, int id) {
  lockClh(lock, slotIndex, id);
  return 0;
}
//lockType is obvious, n is number of possible sources
lock_t *new_lock(char *lockType, int n) {
  lock_t *lock = malloc(sizeof(lock_t));
  if (strcmp(lockType, "TASLock") == 0)
    {
      TASLock *new_tas = malloc(sizeof(TASLock));
      new_tas->state = 0;
      lock->self = (void *)new_tas;
      lock->lock = lockTAS;
      lock->unlock = unlockTAS;
      lock->tryLock = tryLockTAS;
    }

  if (strcmp(lockType, "BackoffLock") == 0)
    {
      backoffLock *new_backoff = malloc(sizeof(backoffLock));
      new_backoff->state = 0;
      lock->self = (void *)new_backoff;
      lock->lock = lockBackoff;
      lock->unlock = unlockBackoff;
      lock->tryLock = tryLockBackoff;
    }

  if (strcmp(lockType, "mutex") == 0)
    {
      pthreadMutexLock *new_mutex = malloc(sizeof(pthreadMutexLock));
      pthread_mutex_init(&(new_mutex->mut), NULL);
      lock->self = (void *)new_mutex;
      lock->lock = lockMutex;
      lock->unlock = unlockMutex;
      lock->tryLock = tryLockMutex;
    }
  if (strcmp(lockType, "ALock") == 0)
    {
      //      aLock *andersonLock = malloc(sizeof(aLock) + sizeof(int[n]));
      aLock *andersonLock = malloc(sizeof(aLock) + sizeof(int) * n + ((64-sizeof(int)) * n)); //padded version
      andersonLock->tail = 0;
      int padCount = ( 64 - sizeof(int) ) / sizeof(int); //for padded version
      andersonLock->flag = calloc(n + padCount*n, sizeof(int)); //for padded version
      //      andersonLock->flag = calloc(n, sizeof(int));
      (andersonLock->flag)[0] = 1;
      andersonLock->n = n;
      lock->self = (void *)andersonLock;
      lock->lock = lockAnderson;
      lock->unlock = unlockAnderson;
      lock->tryLock = tryLockAnderson;
    }

  if (strcmp(lockType, "CLHLock") == 0) {
    clhLock *newCLH = malloc(sizeof(clhLock) +  n*sizeof(qNode *));
    newCLH->numThreads = n;
    newCLH->myNodes = malloc(n*sizeof(qNode *));
    qNode *temp = NULL;
    for(int i = 0; i < n; i++) {
      
      qNode *currNode = malloc(sizeof(qNode));
      currNode->locked = 0;
      currNode->pred = temp;
      (newCLH->myNodes)[i] = currNode;      
      
      temp = currNode; 
    }
    //the last node should be the tail
    newCLH->tail = temp;
    
    lock->self = (void *)newCLH;
    lock->lock = lockClh;
    lock->unlock = unlockClh;
    lock->tryLock = tryLockClh;
  }
  return lock;  
}
