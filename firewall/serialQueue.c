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

#define _XOPEN_SOURCE 600
#define DEFAULT_NUMBER_OF_ARGS 6


void parallelFirewall(const int,
		      const int,
		      const long,
		      const int,
		      const short,
		      const int);

int main(int argc, char * argv[]) {

  if(argc >= DEFAULT_NUMBER_OF_ARGS) {
    const int numPackets = atoi(argv[1]); // This is T
    const int numSources = atoi(argv[2]); // This is n. equal to number of queues and workers
    const long mean = atol(argv[3]); // this is W
    const int uniformFlag = atoi(argv[4]);
    const short experimentNumber = (short)atoi(argv[5]);
    const int queueDepth = atoi(argv[6]); // D

    parallelFirewall(numPackets,numSources,mean,uniformFlag,experimentNumber,queueDepth);
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
  PacketSource_t *packetSource;
  int uniformFlag;
  int numPackets;
  int numSources;
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
      // if (data->threadID == 0)
      // printf("check was %d Checked = %d head: %d tail: %d\n", check, packetsChecked, h, data->q->tail);
    }
    //    printf("My counter is currently %ld\n", *(data->counter));
    *(data->counter) += getFingerprint((*tmp)->iterations, (*tmp)->seed);
    packetsChecked++;
  }
  //printf("I (thread %d) checked %d packets\n", data->threadID, packetsChecked);
  pthread_exit(NULL);
}



void dispatch_packets(void *ptr){
  dispatcherData *data;
  data = (dispatcherData *) ptr;
  
  int uniformFlag = data->uniformFlag;
  //printf("uni flag is: %d %d %d\n", uniformFlag, data->numPackets, data->numSources);
  int i, j;
  int total = 0;
  if (uniformFlag) {
    for (i = 0; i < data->numPackets; i++ ) {
      for (j = 0; j < data->numSources; j++ ) {
	volatile Packet_t * tmp = getUniformPacket(data->packetSource, j);
	volatile int check = -1;
	while (check != 1) {
	check =	enqueue(data->queueArray[j], tmp);
	//printf("enqueing packet %d into queue %d. Result: %d\n", i, j, check);
	}
	total++;
      }
    }
  }
  //else we'll do exponential
  else {
    for (i = 0; i < data->numPackets; i++ ) {
      for (j = 0; j < data->numSources; j++ ) {
	volatile Packet_t * tmp = getExponentialPacket(data->packetSource, j);
	volatile int check = -1;
	while (check != 1) {
	  check =	enqueue(data->queueArray[j], tmp);
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
		       int depth)
{
  PacketSource_t * packetSource = createPacketSource(mean, numSources, experimentNumber);
  StopWatch_t watch;
  long fingerprint = 0;

  startTimer(&watch);  
  pthread_t workers[numSources];
  workerData *workerDatas[numSources];
  
  pthread_t dispatcher;
  dispatcherData *dispatchData = (dispatcherData *)malloc(sizeof(dispatcherData) + numSources * sizeof(queue *));
  dispatchData->packetSource = packetSource;
  dispatchData->uniformFlag = uniformFlag;
  dispatchData->numPackets = numPackets;
  dispatchData->numSources = numSources;

  for (int i = 0; i < numSources; i++) {

    workerDatas[i] = (workerData *)malloc(sizeof(workerData) + depth * sizeof(Packet_t *));
    queue *tempQueue = (queue *)malloc(sizeof(queue) + depth * sizeof(Packet_t *));
    tempQueue->head = 0;
    tempQueue->tail = 0;
    tempQueue->depth = depth;
    workerDatas[i]->q = tempQueue;
    workerDatas[i]->threadID = i;
    workerDatas[i]->numPackets = numPackets;
    dispatchData->queueArray[i] = tempQueue;
  }

  //printf("going to loops\n");
  int i, j;
  int total = 0;
  if (uniformFlag) {
    for (i = 0; i < numPackets; i++ ) {
      for (j = 0; j < numSources; j++ ) {
	volatile Packet_t * tmp = getUniformPacket(packetSource, j);
	volatile Packet_t **outPacket = malloc(sizeof(Packet_t *));
	//	printf("enqueing\n");
	int check = enqueue(dispatchData->queueArray[j], tmp);
	//printf("dequeuing check was %d\n", check);
	dequeue(dispatchData->queueArray[j], outPacket);
	fingerprint += getFingerprint((*outPacket)->iterations, (*outPacket)->seed);
	total++;
      }
    }
  }
  //else we'll do exponential
  else {
    for (i = 0; i < numPackets; i++ ) {
      for (j = 0; j < numSources; j++ ) {
	volatile Packet_t * tmp = getExponentialPacket(packetSource, j);
	volatile Packet_t **outPacket = malloc(sizeof(Packet_t *));
  	enqueue(dispatchData->queueArray[j], tmp);
	dequeue(dispatchData->queueArray[j], outPacket);
	fingerprint += getFingerprint((*outPacket)->iterations, (*outPacket)->seed);
	total++;
      }
    }
  }
  
  //  printf("total: %d\n", total); 
  stopTimer(&watch);
  
  printf("end of program fingerprint = %ld time: %f\n", fingerprint, getElapsedTime(&watch));
}
