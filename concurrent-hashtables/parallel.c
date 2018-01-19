#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <math.h>

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
  int bankSize;
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
		      float hitRate, long mean, int initSize, int numWorkers,
		      float timeToRun, char *tableType);

//  TODO finish do_work, which
//  is the worker thread routine for 
//  computing fingerprints and hashing
void do_work(void *ptr) {
  workerData *data;
  data = (workerData *)ptr;

  SerialHashTable_t *ht = data->ht;
  
  int packetsChecked = 0;
  while (packetsChecked != data->numPackets) {
    HashPacket_t *currHashPacket = malloc(sizeof(HashPacket_t));
    HashPacket_t **outHashPacket = malloc(sizeof(HashPacket_t *)); //  Out param
    volatile Packet_t * currPacket = malloc(sizeof(Packet_t));
    int check = -1;
    while (check != 1) {
            check = dequeue(data->q, outHashPacket);
    }
    currHashPacket = *outHashPacket;
    currPacket = currHashPacket->body;
    //    fingerprint += getFingerprint(currPacket->iterations, currPacket->seed);
    if (currHashPacket->type == Add) {
      //   printf("adding\n");
      pthread_mutex_lock(&ht->lock);
      add_ht(ht, currHashPacket->key, currPacket);
      pthread_mutex_unlock(&ht->lock);
    }
    if  (currHashPacket->type == Remove) {
      //   printf("Removing\n");
      pthread_mutex_lock(&ht->lock);
      remove_ht(ht, currHashPacket->key);
      pthread_mutex_unlock(&ht->lock);
    }
    if (currHashPacket->type == Contains) {
      //  printf("Contains...ing\n");
      pthread_mutex_lock(&ht->lock);
      contains_ht(ht, currHashPacket->key);
      pthread_mutex_unlock(&ht->lock);
    }

    *(data->counter) += 1;
    packetsChecked++;
  }
  pthread_exit(NULL);
}

//  is the worker thread routine for 
//  computing fingerprints and hashing
void do_awesome_work(void *ptr) {
  workerData *data;
  data = (workerData *)ptr;
  SerialHashTable_t *ht = data->ht;
  int bankSize = data->bankSize;
 
  int packetsChecked = 0;
  while (packetsChecked != data->numPackets) {
    HashPacket_t *currHashPacket = malloc(sizeof(HashPacket_t));
    HashPacket_t **outHashPacket = malloc(sizeof(HashPacket_t *)); //  Out param
    volatile Packet_t * currPacket = malloc(sizeof(Packet_t));
    int check = -1;
    while (check != 1) {
            check = dequeue(data->q, outHashPacket);
    }
    currHashPacket = *outHashPacket;
    currPacket = currHashPacket->body;
    //    fingerprint += getFingerprint(currPacket->iterations, currPacket->seed);
    int hashIndex = currHashPacket->key & ht->mask;
    int index = hashIndex & (ht->logSize - 1);
    index %= bankSize;
    // printf("index = %d\n", index);
    if (currHashPacket->type == Add) {
      //  printf("adding\n");
      pthread_rwlock_wrlock(ht->rwlock[index]);
      add_ht(ht, currHashPacket->key, currPacket);
      pthread_rwlock_unlock(ht->rwlock[index]);
    }
    if  (currHashPacket->type == Remove) {
      //  printf("Removing\n");
      pthread_rwlock_wrlock(ht->rwlock[index]);
      remove_ht(ht, currHashPacket->key);
      pthread_rwlock_unlock(ht->rwlock[index]);
    }
    if (currHashPacket->type == Contains) {
      //  printf("Contains...ing\n");
      pthread_rwlock_rdlock(ht->rwlock[index]);
      contains_ht(ht, currHashPacket->key);
      pthread_rwlock_unlock(ht->rwlock[index]);
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
		      float hitRate, long mean, int initSize, int numWorkers,
		      float timeToRun, char *tableType) {
    
  SerialHashTable_t * ht = createSerialHashTable(8, 1024);
  int bankSize = 0;
  if (strcmp(tableType, "CoarseLocking") == 0) {
    pthread_mutex_init(&ht->lock, NULL);
  }
  // Need to create a lock bank and realloc if we are using awesome
  else {
    bankSize = pow(2, ceil(log(numWorkers)/ log(2)));
    realloc(ht, sizeof(SerialHashTable_t));
    realloc(ht->table, sizeof(SerialList_t*) * ht->size);
    ht->rwlock = (pthread_rwlock_t **) malloc(bankSize * sizeof(pthread_rwlock_t *));
    for (int i = 0; i < bankSize; i++) {
      pthread_rwlock_t *tmp = malloc(sizeof(pthread_rwlock_t));
      pthread_rwlock_init(tmp, NULL);
      ht->rwlock[i] = tmp;
    }
  }
  HashPacketGenerator_t *hashGen = createHashPacketGenerator(fractionAdd, fractionRemove, hitRate, mean);
  //Preload the hash table with <initSize> values
  for (int i = 0; i < initSize; i++) {
    HashPacket_t *packetToAdd;
    packetToAdd = getAddPacket(hashGen);
    
      add_ht(ht, packetToAdd->key, packetToAdd->body);
  }

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
    workerDatas[i]->bankSize = bankSize;
    workerDatas[i]->counter = &sums[i];
    workerDatas[i]->ht = ht;
  }
  pthread_create(&dispatcher, NULL, (void *) &dispatch_packets, dispatchData);
  if (strcmp(tableType, "CoarseLocking") == 0) {
    for (int i = 0; i < numWorkers; i++) {
      pthread_create(&workers[i], NULL, (void *) &do_work, workerDatas[i]);
    }
  } else { // need to use the "awesome" worker function
    for (int i = 0; i < numWorkers; i++) {
      pthread_create(&workers[i], NULL, (void *) &do_awesome_work, workerDatas[i]);
    }
  }

  sleep(timeToRun / 1000);
  pthread_cancel(dispatcher);
  for (int i = 0; i < numWorkers; i++) {
    pthread_cancel(workers[i]);
  }

  int packetsProcessed = 0;
  for (int i = 0; i < numWorkers; i++) {
    packetsProcessed += sums[i];
  }
  printf("%d\n",  packetsProcessed);
  exit(1);
}

int main(int argc, char * argv[]) {
  
  if(argc >= DEFAULT_NUMBER_OF_ARGS) {
    const int numPackets = 10000000;
    const float timeToRun =  atof(argv[1]);
    const float fractionAdd = atof(argv[2]);
    const float fractionRemove = atof(argv[3]);
    const float hitRate = atof(argv[4]);
    const long mean = atol(argv[5]); // Expected work per packet.
    const int initSize = atoi(argv[6]); // Number of items to preload into table.
    const int numWorkers = atoi(argv[7]);
    char *tableType = argv[8];

    parallelFirewall(numPackets, fractionAdd, fractionRemove, hitRate, mean, initSize, numWorkers, timeToRun, tableType);
  }
  return 0;
}
