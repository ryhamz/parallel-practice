#include "Utils/hashgenerator.h"

typedef struct {
  volatile  int head;
  volatile  int tail;
  int depth;
  HashPacket_t *arr[];
} queue;


int enqueue(queue *q, HashPacket_t *packet);
int dequeue(queue *q, HashPacket_t **outPacket);
