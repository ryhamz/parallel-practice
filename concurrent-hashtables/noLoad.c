#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#include "Utils/generators.c"
#include "Utils/stopwatch.c"
#include "Utils/fingerprint.c"
#include "Utils/packetsource.c"
#include "Utils/crc32.c"
#include "Utils/hashgenerator.c"
#include "Utils/hashtable.c"
#include "Utils/seriallist.c"
#include "queue.c"

#define DEFAULT_NUMBER_OF_ARGS 6

typedef struct args {
  int numPackets;
  int numSources;
  long mean;
  int uniformFlag;
  short experimentNumber;
  HashPacketGenerator_t *hashGen;
  SerialHashTable_t *ht;
  int *packetsProcessed;
} args;

typedef struct workerData {
  int threadID;
  int numPackets;
  long *counter;
  SerialHashTable_t *ht;
  queue *q;
} workerData;

typedef struct dispatcherData {
  int numPackets;
  int numWorkers;
  HashPacketGenerator_t *hashGen;
  queue *queueArray[];
} dispatcherData;

void parallelFirewall(int numPackets, float fractionAdd, float fractionRemove,
		      float hitRate, long mean, int initSize, int numWorkers, float timeToRun);

//  TODO finish do_work, which
//  is the worker thread routine for 
//  computing fingerprints and hashing
void do_work(void *ptr) {
  workerData *data;
  data = (workerData *)ptr;

  
  
  int packetsChecked = 0;
  while (packetsChecked != data->numPackets) {
    HashPacket_t **outHashPacket = malloc(sizeof(HashPacket_t *)); //  Out param
    int check = -1;
    while (check != 1) {
            check = dequeue(data->q, outHashPacket);
    }


    *(data->counter) += 1;
    packetsChecked++;
  }
  pthread_exit(NULL);
}

void dispatch_packets(void *ptr) {
  dispatcherData *data;
  data = (dispatcherData *)ptr;
  
  int total = 0; // Used to test # packets dispatched;
  int numPackets = data->numPackets;
  int numWorkers = data->numWorkers;
  HashPacketGenerator_t *hashGen = data->hashGen;
  queue **queueArr = data->queueArray;

  int i, j;
  for (i = 0; i < numPackets; i++ ) {
    for (j = 0; j < numWorkers; j++ ) {
      HashPacket_t *currHashPacket = getRandomPacket(hashGen);
      int check = -1;
      while (check != 1) {
	check = enqueue(queueArr[j], currHashPacket);
      }
      total++;
    }
  }
  //  printf("total: %d\n", total);
}
  

void parallelFirewall(int numPackets, float fractionAdd, float fractionRemove,
		      float hitRate, long mean, int initSize, int numWorkers, float timeToRun) {
    

    HashPacketGenerator_t *hashGen = createHashPacketGenerator(fractionAdd, fractionRemove, hitRate, mean);

  int depth = 12; // We are keeping constant queue depth.
  long sums[numWorkers]; // each thread can increment its part of this array. Sum to determine throughput.
  pthread_t workers[numWorkers];
  workerData *workerDatas[numWorkers];
  pthread_t dispatcher;
  dispatcherData *dispatchData = (dispatcherData *)malloc(sizeof(dispatcherData) + numWorkers * sizeof(queue *));
  dispatchData->numPackets = numPackets;
  dispatchData->numWorkers = numWorkers;
  dispatchData->hashGen = hashGen;

  for (int i = 0; i < numWorkers; i++) {
    sums[i] = 0;
    queue *tempQueue = (queue *)malloc(sizeof(queue) + depth * sizeof(HashPacket_t *));
    tempQueue->head = 0;
    tempQueue->tail = 0;
    tempQueue->depth = depth;
    dispatchData->queueArray[i] = tempQueue;

    //  initialize data for worker threads
    workerDatas[i] = (workerData *)malloc(sizeof(workerData) + depth * sizeof(HashPacket_t *));
    workerDatas[i]->q = tempQueue;
    workerDatas[i]->threadID = i;
    workerDatas[i]->numPackets = numPackets;
    workerDatas[i]->counter = &sums[i];

  }

  pthread_create(&dispatcher, NULL, (void *) &dispatch_packets, dispatchData);
  for (int i = 0; i < numWorkers; i++) {
    pthread_create(&workers[i], NULL, (void *) &do_work, workerDatas[i]);
  }

  sleep(timeToRun / 1000);
  pthread_cancel(dispatcher);
  for (int i = 0; i < numWorkers; i++) {
    pthread_cancel(workers[i]);
  }

  long fingerprint = 0;
  for (int i = 0; i < numWorkers; i++) {
    fingerprint += sums[i];
  }
  printf("%ld\n", fingerprint);
}


int main(int argc, char * argv[]) {

  if(argc >= DEFAULT_NUMBER_OF_ARGS) {
    const int numPackets = 100000000;
    const float timeToRun =  atof(argv[1]);
    const float fractionAdd = atof(argv[2]);
    const float fractionRemove = atof(argv[3]);
    const float hitRate = atof(argv[4]);
    const long mean = atol(argv[5]); // Expected work per packet.
    const int initSize = atoi(argv[6]); // Number of items to preload into table.
    const int numWorkers = atoi(argv[7]);

    parallelFirewall(numPackets, fractionAdd, fractionRemove, hitRate, mean, initSize, numWorkers, timeToRun);
  }
  return 0;
}
