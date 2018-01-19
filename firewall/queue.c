#include "queue.h"

int enqueue(queue *q, volatile Packet_t *packet) {
  //  printf("head: %d. tail: %d\n", q->head, q->tail);
  if ((q->tail - q->head) == q->depth) {
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
    // printf("old head: %d New: %d\n", h, q->head);
    
    return 1;
  }
}
