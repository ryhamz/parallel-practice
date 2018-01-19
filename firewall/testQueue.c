#include "queue.h"

int enqueue(queue *q, volatile Packet_t *packet) {
  //  printf("head: %d. tail: %d\n", q->head, q->tail);
  if (((q->tail) - (q->head)) == q->depth) {
    return -1;
  }

  else {
    q->arr[q->tail % q->depth] = packet;
    int t = q->tail;
    q->tail = t + 1;
    
    return 1;
  }
}


int dequeue(queue *q, volatile Packet_t **outPacket) {
  //   printf("about to compare head and tail\n");
  if (q->tail == q->head) {
    //queue is empty. error.
    return -1;
  }
  else {
    *outPacket = q->arr[q->head % q->depth];
    int h = q->head;
    q->head = h + 1;
    
    return 1;
  }
}


int main() {
  queue *q = (queue *)malloc(sizeof(queue) + 5 * sizeof(Packet_t *));
  q->depth = 4;
  q->tail = 0;
  q->head = 0;
  
  for (int i = 0; i < 5; i++) {
    volatile Packet_t *p = (Packet_t *)malloc(sizeof(Packet_t ));
    p->iterations = i;
    p->seed = i *5;
    int c =    enqueue(q, p);
    printf("%d\n", c);
  }
  for (int i = 0; i < 4; i++) {
    volatile  Packet_t **tmp = (volatile Packet_t **)malloc(sizeof(Packet_t *));
    int   c =  dequeue(q, tmp);
    printf("iterations: %ld Seed: %ld\n", (*tmp)->iterations, (*tmp)->seed);
  }
  return 1;
}
