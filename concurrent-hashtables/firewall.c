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

void serialFirewall(void *ptr);

int main(int argc, char * argv[]) {

  if(argc >= DEFAULT_NUMBER_OF_ARGS) {
    const int numPackets = 100000000;
    const float timeToRun =  atof(argv[1]);
    const int numSources = 1;
    const float fractionAdd = atof(argv[2]);
    const float fractionRemove = atof(argv[3]);
    const float hitRate = atof(argv[4]);
    const long mean = atol(argv[5]); // Expected work per packet.
    const int initSize = atoi(argv[6]); // Number of items to preload into table.
    const int uniformFlag = 1;
    const short experimentNumber = 2;

    SerialHashTable_t * ht = createSerialHashTable(8, 1024);

    HashPacketGenerator_t *hashGen = createHashPacketGenerator(fractionAdd, fractionRemove, hitRate, mean);
    //  printf("init size: %d\n", initSize);
    for (int i = 0; i < initSize; i++) {
      HashPacket_t *packetToAdd;
      packetToAdd = getAddPacket(hashGen);
	    
      add_ht(ht, packetToAdd->key, packetToAdd->body);
    }
    pthread_t worker;
    args *firewallArgs = malloc(sizeof(args));
    firewallArgs->numPackets = numPackets;
    firewallArgs->numSources = numSources;
    firewallArgs->mean = mean;
    firewallArgs->uniformFlag = uniformFlag;
    firewallArgs->experimentNumber = experimentNumber;
    firewallArgs->ht = ht;
    firewallArgs->hashGen = hashGen;
    int *packetsProcessed = malloc(sizeof(int));
    *packetsProcessed = 0;
    firewallArgs->packetsProcessed = packetsProcessed;

    pthread_create(&worker, NULL, (void *) &serialFirewall, firewallArgs);
    sleep(timeToRun / 1000);
    pthread_cancel(worker);
      printf("%d\n", *packetsProcessed);
    //  print_ht(ht);

  }
  return 0;
}


void serialFirewall (void *ptr) {
  args *data;
  data = (args *)ptr;

  SerialHashTable_t * ht = data->ht;
  HashPacketGenerator_t *hashGen = data->hashGen;
  int numSources = data->numSources;
  int numPackets = data->numPackets;
  int *packetsProcessed = data->packetsProcessed;
  long fingerprint = 0;

  for( int i = 0; i < numSources; i++ ) {
    for( int j = 0; j < numPackets; j++ ) {
      HashPacket_t *currPacket = getRandomPacket(hashGen);
      volatile Packet_t * tmp = currPacket->body;
      fingerprint += getFingerprint(tmp->iterations, tmp->seed);
      if (currPacket->type == Add) {
	//  printf("adding\n");
	add_ht(ht, currPacket->key, tmp);
      }
      if (currPacket->type == Remove) {
	//  printf("Removing\n");
	remove_ht(ht, currPacket->key);
      }
      if (currPacket->type == Contains) {
	//  printf("Contains...ing\n");
	contains_ht(ht, currPacket->key);
      }
      (*packetsProcessed)++;
    }
  }
  //  printf("%ld\n", fingerprint);
}
