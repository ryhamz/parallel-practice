#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#include "generators.c"
#include "stopwatch.c"
#include "fingerprint.c"
#include "packetsource.c"
#include "queue.c"

#define DEFAULT_NUMBER_OF_ARGS 6

typedef struct {
  int numPackets;
  int numSources;
  long mean;
  int uniformFlag;
  short experimentNumber;
  int *packetsProcessed;
} workerData;

void serialFirewall(void *ptr);

int main(int argc, char * argv[]) {

  if(argc >= DEFAULT_NUMBER_OF_ARGS) {
    const int numPackets = 90000000;
    const int timeToRun = atoi(argv[1]);
    const int numSources = atoi(argv[2]);
    const long mean = atol(argv[3]);
    const int uniformFlag = atoi(argv[4]);
    const short experimentNumber = (short)atoi(argv[5]);

    pthread_t worker;
    workerData *workerData = malloc(sizeof(workerData));
    workerData->numPackets = numPackets;
    workerData->numSources = numSources;
    workerData->mean = mean;
    workerData->uniformFlag = uniformFlag;
    workerData->experimentNumber = experimentNumber;


    int *packetsProcessed = malloc(sizeof(int));
    *packetsProcessed = 0;
    workerData->packetsProcessed = packetsProcessed;

    pthread_create(&worker, NULL, (void *) &serialFirewall, workerData);
    sleep(timeToRun / 1000);
    pthread_cancel(worker);
    printf("%d\n", *packetsProcessed);
    //    serialFirewall(numPackets,numSources,mean,uniformFlag,experimentNumber, timeToRun);
  }
  return 0;
}
void serialFirewall(void *ptr)
{
  workerData *data;
  data = (workerData *)ptr;
  const int  numPackets =  data->numPackets;
  const int numSources = data->numSources;
  const long mean = data->mean;
  const int uniformFlag = data->uniformFlag;
  const short experimentNumber = data->experimentNumber;

  int *packetsProcessed = data->packetsProcessed;  

  PacketSource_t * packetSource = createPacketSource(mean, numSources, experimentNumber);

  long fingerprint = 0;
 
  if( uniformFlag == 1) {
    for( int i = 0; i < numSources; i++ ) {
      for( int j = 0; j < numPackets; j++ ) {
	volatile Packet_t * tmp = getUniformPacket(packetSource,i);
	fingerprint += getFingerprint(tmp->iterations, tmp->seed);
	*(packetsProcessed) += 1;
      }
    }
  }
  if(uniformFlag == 0) {
    for( int i = 0; i < numSources; i++ ) {
      for( int j = 0; j < numPackets; j++ ) {
	volatile Packet_t * tmp = getExponentialPacket(packetSource,i);
	fingerprint += getFingerprint(tmp->iterations, tmp->seed);
	(*packetsProcessed)++;
      }
    }
  }

}
