#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "stopwatch.c"

int main(int argc, char *argv[]) {
  const int b = atoi(argv[1]);
  StopWatch_t watch;
  startTimer(&watch);
  volatile int count = 0;
  for(int i = 0; i < b; i++) {
    count++;
  }
  stopTimer(&watch);
  printf("%f\n", getElapsedTime(&watch));
  //  printf("%d\n", count);
  return 1;
}
