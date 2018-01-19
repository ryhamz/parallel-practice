#include "packetsource.h"


typedef struct {
volatile  int head;
volatile  int tail;
  int depth;
  volatile  Packet_t *arr[];
} queue;


int enqueue(queue *q, volatile Packet_t *packet);
int dequeue(queue *q, volatile Packet_t **outPacket);
