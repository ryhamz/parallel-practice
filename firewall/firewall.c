#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "generators.c"
#include "stopwatch.c"
#include "fingerprint.c"
#include "packetsource.c"
#include "queue.c"

#define DEFAULT_NUMBER_OF_ARGS 6

void serialFirewall(const int,
		    const int,
		    const long,
		    const int,
		    const short);

int main(int argc, char * argv[]) {

  if(argc >= DEFAULT_NUMBER_OF_ARGS) {
    const int numPackets = atoi(argv[1]);
    const int numSources = atoi(argv[2]);
    const long mean = atol(argv[3]);
    const int uniformFlag = atoi(argv[4]);
    const short experimentNumber = (short)atoi(argv[5]);

    serialFirewall(numPackets,numSources,mean,uniformFlag,experimentNumber);
  }
  return 0;
}
void serialFirewall (int numPackets,
		     int numSources,
		     long mean,
		     int uniformFlag,
		     short experimentNumber)
{
  PacketSource_t * packetSource = createPacketSource(mean, numSources, experimentNumber);
  StopWatch_t watch;
  long fingerprint = 0;

  if( uniformFlag == 2) {
    startTimer(&watch);
    for( int i = 0; i < numSources; i++ ) {
      for( int j = 0; j < numPackets; j++ ) {
	volatile Packet_t * tmp = getConstantPacket(packetSource,i, mean);
	fingerprint += getFingerprint(tmp->iterations, tmp->seed);
      }
    }
    stopTimer(&watch);
  }

  if( uniformFlag == 1) {
    startTimer(&watch);
    for( int i = 0; i < numSources; i++ ) {
      for( int j = 0; j < numPackets; j++ ) {
	volatile Packet_t * tmp = getUniformPacket(packetSource,i);
	fingerprint += getFingerprint(tmp->iterations, tmp->seed);
      }
    }
    stopTimer(&watch);
  }
  if(uniformFlag == 0) {
    startTimer(&watch);
    for( int i = 0; i < numSources; i++ ) {
      for( int j = 0; j < numPackets; j++ ) {
	volatile Packet_t * tmp = getExponentialPacket(packetSource,i);
	fingerprint += getFingerprint(tmp->iterations, tmp->seed);
      }
    }
    stopTimer(&watch);
  }
  printf("%f\n",getElapsedTime(&watch));
  //  printf("fingerprint = %ld\n", fingerprint);
}
