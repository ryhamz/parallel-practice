#ifndef LOCKS_H
#define LOCKS_H

typedef struct lock {
  void *self;
  void (*lock)(void *lock, int *slotIndex, int id);
  void (*unlock)(void *lock, int *slotIndex, int id);
  int (*tryLock)(void *lock, int *slotIndex, int id);
} lock_t;

// Takes in a TAS lock and releases it
typedef struct TASLock {
  volatile int state;
} TASLock;

typedef struct backoffLock {
  volatile int state;
} backoffLock;

typedef struct pthreadMutexLock {
  pthread_mutex_t mut;
} pthreadMutexLock;

typedef struct aLock {
  int n; //number of threads accessing
  volatile int tail;
  volatile int *flag;
} aLock;


typedef struct qNode qNode;
struct qNode{
  volatile  int locked;
  volatile qNode *pred;
};


typedef struct clhLock {
  int numThreads;
  volatile qNode *tail;
  volatile qNode **myNodes;
} clhLock;

// Takes in a TAS lock and locks it.
void lockTAS(void *lock, int *slotIndex, int id);

// Takes in a TAS lock and releases it
void unlockTAS(void *lock, int *slotIndex, int id);

int tryLockTAS(void *lock, int *slotIndex, int id);

void lockBackoff(void *lock, int *slotIndex, int id);
void unlockBackoff(void *lock, int *slotIndex, int id);
int tryLockBackoff(void *lock, int *slotIndex, int id);

void lockMutex(void *lock, int *slotIndex, int id);
void unlockMutex(void *lock, int *slotIndex, int id);
int tryLockMutex(void *lock, int *slotIndex, int id);

void lockAnderson(void *lock, int *slotIndex, int id);
void unlockAnderson(void *lock, int *slotIndex, int id);
int tryLockAnderson(void *lockint, int *slotIndex, int id);

void lockClh(void *lock, int *slotIndex, int id);
void unlockClh(void *lock, int *slotIndex, int id);
int tryLockClh(void *lockint, int *slotIndex, int id);

lock_t *new_lock(char *lockType, int n);
#endif
