#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>    /* POSIX Threads */

#include "generators.c"
#include "stopwatch.c"
#include "fingerprint.c"
#include "packetsource.c"
#include "queue.c"
#include "locks.c"

#define _XOPEN_SOURCE 600
#define DEFAULT_NUMBER_OF_ARGS 6


void parallelFirewall(const int,
		      const int,
		      const long,
		      const int,
		      const short,
		      const int,
		      const int);

void lockedFirewall(const int,
		    const int,
		    const long,
		    const int,
		    const short,
		    const int,
		    const int,
		    int,
		    char *);

int main(int argc, char * argv[]) {

  if(argc >= DEFAULT_NUMBER_OF_ARGS) {
    const int numPackets = 90000000; // This is T
    const int timeToRun = atoi(argv[1]);
    const int numSources = atoi(argv[2]); // This is n. equal to number of queues and workers
    const long mean = atol(argv[3]); // this is W
    const int uniformFlag = atoi(argv[4]);
    const short experimentNumber = (short)atoi(argv[5]);
    const int queueDepth = atoi(argv[6]); // D
    char *lockType = argv[7];
    char *strategy = argv[8];
    if (strcmp("LockFree", strategy) == 0) {
      parallelFirewall(numPackets, numSources, mean, uniformFlag, experimentNumber, queueDepth, timeToRun);
    }
    int homeQueueFlag;
    if(strcmp("HomeQueue", strategy) == 0) {
      homeQueueFlag = 1;
      lockedFirewall(numPackets,numSources,mean,uniformFlag,experimentNumber,queueDepth, timeToRun, homeQueueFlag, lockType);
    }
    if(strcmp("RandomQueue", strategy) == 0) {
      homeQueueFlag = 0;
      lockedFirewall(numPackets,numSources,mean,uniformFlag,experimentNumber,queueDepth, timeToRun, homeQueueFlag, lockType);
    }
    else if(strcmp("awesome", strategy) == 0) {
      homeQueueFlag = 2;
      lockedFirewall(numPackets,numSources,mean,uniformFlag,experimentNumber,queueDepth, timeToRun, homeQueueFlag, lockType);
    }
  }
  return 0;
}

typedef struct {
  int numPackets;
  int threadID;
  long *counter;
  queue *q;
} workerData;

typedef struct {
  int numSources;
  int numPackets;
  int homeQueueFlag;
  int threadID;
  long *counter;
  queue *queueArray[];
} lockedWorkerData;

typedef struct {
  PacketSource_t *packetSource;
  int uniformFlag;
  int numPackets;
  int numSources;
  long mean;
  queue *queueArray[];
} dispatcherData;

void do_work(void *ptr) {
  workerData *data;
  data = (workerData *) ptr;

  int packetsChecked = 0;
  //printf("numpackets is %d\n", data->numPackets);
  while (packetsChecked != data->numPackets) {
    volatile int check = -1;
    volatile Packet_t **tmp = malloc(sizeof(Packet_t *));
    while (check != 1) {
      check = dequeue(data->q, tmp);
    }
    //    *(data->counter) += getFingerprint((*tmp)->iterations, (*tmp)->seed);
    getFingerprint((*tmp)->iterations, (*tmp)->seed);
    packetsChecked++;
    *(data->counter) += 1;
  }
  //  printf("I (thread %d) checked %d packets\n", data->threadID, packetsChecked);
  pthread_exit(NULL);
}

void do_locked_work(void *ptr) {
  lockedWorkerData *data;
  data = (lockedWorkerData *) ptr;
  int packetsChecked = 0;
  queue **queueArr = data->queueArray; 
  void (*lockFunc)(void *lock, int *slotIndex, int id) = queueArr[0]->lock->lock;
  void (*unlockFunc)(void *lock, int *slotIndex, int id) = queueArr[0]->lock->unlock;
  int missedDequeues = 0; // used for awesome strategy. If we fail enough on our home queue, switch to random.
  //we don't want to stop grabbing packets, so infinite loop!
  while (1) {
    volatile int check = -1;
    volatile Packet_t **tmp = malloc(sizeof(Packet_t *));
    int id = data->threadID;
    int slot = 0;
    int queueIndex;
    if(data->homeQueueFlag == 1) {
      queueIndex = data->threadID;
    }
    if(data->homeQueueFlag == 0 || (data->homeQueueFlag == 2 && missedDequeues > 20)) {
     queueIndex = rand() % data->numSources;
    }
    while (check != 1) {
      lockFunc(queueArr[queueIndex]->lock->self, &slot, id);
      check = dequeue(queueArr[id], tmp);
      unlockFunc(queueArr[queueIndex]->lock->self, &slot, id);
      if(check == -1) {
	missedDequeues++;
      }
      else {
	missedDequeues = 0;
      }
      sched_yield();
    }
    // *(data->counter) += getFingerprint((*tmp)->iterations, (*tmp)->seed);
    getFingerprint((*tmp)->iterations, (*tmp)->seed);
    packetsChecked++;
    *(data->counter) += 1;
  }
  pthread_exit(NULL);
}



void dispatch_packets(void *ptr){
  dispatcherData *data;
  data = (dispatcherData *) ptr;
  
  int uniformFlag = data->uniformFlag;
  int i, j;
  int total = 0;
 
  int numPackets = data->numPackets;
  int numSources = data->numSources;  
  PacketSource_t *packetSource = data->packetSource;
  if (uniformFlag == 1) {
    for (i = 0; i < numPackets; i++ ) {
      for (j = 0; j < numSources; j++ ) {
	volatile Packet_t * tmp = getUniformPacket(packetSource, j);
	volatile int check = -1;
	while (check != 1) {
	  check = enqueue(data->queueArray[j], tmp);
	  //printf("enqueing packet %d into queue %d. Result: %d\n", i, j, check);
	  if (check == 1) {
	    //printf("failed enqueue\n");
	  }
	}
	total++;
      }
    }
  }
  //else we'll do exponential
  if (uniformFlag == 0) {
    for (i = 0; i < numPackets; i++ ) {
      for (j = 0; j < numSources; j++ ) {
	volatile Packet_t * tmp = getExponentialPacket(packetSource, j);
	volatile int check = -1;
	while (check != 1) {
	  check = enqueue(data->queueArray[j], tmp);
	  //printf("enqueing packet %d into queue %d. Result: %d\n", i, j, check);
	}
	total++;
      }
    }
  }
  //printf("total: %d\n", total); 
}

void parallelFirewall (int numPackets,
		       int numSources,
		       long mean,
		       int uniformFlag,
		       short experimentNumber,
		       int depth,
		       int timeToRun)
{
  PacketSource_t * packetSource = createPacketSource(mean, numSources, experimentNumber);
  long fingerprint = 0;
  long sums[numSources]; // each thread can increment it's part of this array. Sum at the end.
  
  pthread_t workers[numSources];
  workerData *workerDatas[numSources];
  
  pthread_t dispatcher;
  dispatcherData *dispatchData = (dispatcherData *)malloc(sizeof(dispatcherData) + numSources * sizeof(queue *));
  dispatchData->packetSource = packetSource;
  dispatchData->uniformFlag = uniformFlag;
  dispatchData->numPackets = numPackets;
  dispatchData->numSources = numSources;
    dispatchData->mean = mean;
  for (int i = 0; i < numSources; i++) {
    sums[i] = 0;
    workerDatas[i] = (workerData *)malloc(sizeof(workerData) + depth * sizeof(Packet_t *));
    queue *tempQueue = (queue *)malloc(sizeof(queue) + depth * sizeof(Packet_t *));
    tempQueue->head = 0;
    tempQueue->tail = 0;
    tempQueue->depth = depth;
    workerDatas[i]->q = tempQueue;
    workerDatas[i]->threadID = i;
    workerDatas[i]->numPackets = numPackets;
    workerDatas[i]->counter = &sums[i];
    dispatchData->queueArray[i] = tempQueue;
  }
  //make dispatcher thread
  pthread_create(&dispatcher, NULL, (void *) &dispatch_packets, dispatchData);
  //make worker threads
  for (int i = 0; i < numSources; i++) {
    pthread_create(&workers[i], NULL, (void *) &do_work, workerDatas[i]);
      }

  sleep(timeToRun / 1000);
  pthread_cancel(dispatcher);
  for (int i = 0; i < numSources; i++) {
    pthread_cancel(workers[i]);
  }
  
  for (int i = 0; i < numSources; i++) {
    fingerprint += sums[i];
  }

  //    printf("Total packets checked = %ld\n", fingerprint);
    printf("%ld\n", fingerprint);
    exit(1);
}

//TODO lockedFirewall
void lockedFirewall (int numPackets,
		     int numSources,
		     long mean,
		     int uniformFlag,
		     short experimentNumber,
		     int depth,
		     int timeToRun,
		     int homeQueueFlag,
		     char *lockType)
{
  PacketSource_t * packetSource = createPacketSource(mean, numSources, experimentNumber);
  long fingerprint = 0;
  long sums[numSources]; // each thread can increment it's part of this array. Sum at the end.
  
  pthread_t workers[numSources];
  lockedWorkerData *workerDatas[numSources];
  
  pthread_t dispatcher;
  dispatcherData *dispatchData = (dispatcherData *)malloc(sizeof(dispatcherData) + numSources * sizeof(queue *));
  dispatchData->packetSource = packetSource;
  dispatchData->uniformFlag = uniformFlag;
  dispatchData->numPackets = numPackets;
  dispatchData->numSources = numSources;
  dispatchData->mean = mean;

  for (int i = 0; i < numSources; i++) {
    sums[i] = 0;
    //allocate all worker datas on first pass, because I need them on that pass
    if (i == 0) {
      for(int j = 0; j < numSources; j++) {
	workerDatas[j] = (lockedWorkerData *)malloc(sizeof(lockedWorkerData) + numSources  * sizeof(queue *));
      }
    }
    queue *tempQueue = (queue *)malloc(sizeof(queue) + depth * sizeof(Packet_t *));
    tempQueue->head = 0;
    tempQueue->tail = 0;
    tempQueue->depth = depth;
    tempQueue->lock = new_lock(lockType, numSources);
    for(int j = 0; j < numSources; j++) {
      workerDatas[j]->queueArray[i] = tempQueue;
    }
    //    workerDatas[i]->queueArray[i] = tempQueue;
    workerDatas[i]->threadID = i;
    workerDatas[i]->numSources = numSources;
    workerDatas[i]->numPackets = numPackets;
    workerDatas[i]->homeQueueFlag = homeQueueFlag;
    workerDatas[i]->counter = &sums[i];
    dispatchData->queueArray[i] = tempQueue;
  }
  //printf("creating threads\n");
  //make dispatcher thread
  pthread_create(&dispatcher, NULL, (void *) &dispatch_packets, dispatchData);
  //make worker threads
    for (int i = 0; i < numSources; i++) {
      pthread_create(&workers[i], NULL, (void *) &do_locked_work, workerDatas[i]);
    }
    // printf("going to sleep\n");
  sleep(timeToRun / 1000);
  pthread_cancel(dispatcher);
  for (int i = 0; i < numSources; i++) {
    pthread_cancel(workers[i]);
  }
  
  for (int i = 0; i < numSources; i++) {
    fingerprint += sums[i];
  }
  
  //  printf("Total packets checked = %ld\n", fingerprint);
  printf("%ld\n", fingerprint);
  exit(1);
}
