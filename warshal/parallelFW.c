#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "stopwatch.c"
#include "validation.c"
#define _XOPEN_SOURCE 600

/* The input is an adjacency matrix within a text file
 * The first line of the input is going to be the number
 * of vertices in the graph (n).
 * We will then have n lines with n integers which are the edge weights
 * Max edge length = 1000. Infinity is 10,000,000.
 */


pthread_barrier_t barrier;

// Phase 1: read file into memory
// read_input 2 takes in vertices as an out parameter.
// It returns the 2d array weights after reading through input.txt
int ** read_input(char* input, int *vertices){
  int n;
  FILE *fp;
  int i, j;
  int check;
  //    printf("opening file\n");
  fp = fopen(input, "r");
  check =   fscanf(fp, "%d\n", &n); //grab our n value
  if (check != 1)
    {
      printf("read_input could not properly scan input file.\n");
      exit(1);
    }
  *vertices = n; //assign it to out parameter
  //  printf("n is %d\n", n);

  //allocate 2d array
  int **weights = (int **)malloc(n * sizeof(int *));
  for(i = 0; i < n; i++){
    weights[i] = (int *)malloc(n * sizeof(int));
  }

  //  printf("malloc complete\n");

  for(i = 0; i < n; i++) {
    for(j = 0; j < n; j++) {
      int temp;
      if(j != n - 1) { //scan like this if we aren't at the last int
        check = fscanf(fp, "%d ", &temp);
	if (check != 1)
	  {
	    printf("read_input could not properly scan input file.\n");
	    exit(1);
	  }
      }
      else { //otherwise, we scant the last one and the newline
        check = fscanf(fp, "%d\n", &temp);
	if (check != 1)
	  {
	    printf("read_input could not properly scan input file.\n");
	    exit(1);
	  }
      }
      //check if we have an invalid edge weight
      if(temp > 1000 && temp != 10000000) {
	//tell the user to give us some input we can actually use
	printf("Input.txt is invalid due to improper edge weight.\nEdges need to be <= 1000 or 10000000 for infinite weight.\n");
	exit(1);
      }
      weights[i][j] = temp;
      // printf("Weights %d,%d is %d and i = %d\n", i, j, weights[i][j], i);
    }
  }
  fclose(fp);
  return weights;
}

typedef struct info{
  int *vertices;
  int **weights;
  int threadCount;
  int threadID;
  //  pthread_barrier_t barrier;
} info;

// Phase 2: the algorithm itself 

void *fw(void *data){
  info *threadInfo = (info *)data;
  //printf("in thread %d of %d\n", threadInfo->threadID, threadInfo->threadCount);
  int n = *threadInfo->vertices;
  int k, i, j;
  
  int **weights = threadInfo->weights;
  
  int start = threadInfo->threadID * n / threadInfo->threadCount;
  int end = (threadInfo->threadID + 1) * n / threadInfo->threadCount;
  // printf("start:%d end %d\n", start,end);

  for(k = 0; k < n; k++){
    pthread_barrier_wait(&barrier);
    for(i = start; i < end; i++){
      for(j = 0; j < n; j++){
        if (weights[i][j] > (weights[i][k] + weights[k][j])) {
	  //update with shorter path
          weights[i][j] = weights[i][k] + weights[k][j];
        }
      }
    }
  }
  free(threadInfo);  
  return NULL;
}

void startfw(int *vertices, int **weights, int threadCount) {
  pthread_t threads[threadCount];
  
  pthread_barrier_init(&barrier, NULL, threadCount);

  int i;
  for (i = 0; i < threadCount; ++i)
    {
      info *threadInfo;
      threadInfo = (info *)malloc(sizeof(info));
      threadInfo->vertices = vertices;
      threadInfo->weights = weights;
      threadInfo->threadCount = threadCount;
      threadInfo->threadID = i;
      printf("creating thread%d\n", i);
      pthread_create(&threads[i], NULL, fw, (void *)threadInfo);
    }
}


// Phase 3: Output the shortest paths table.

void output(int *vertices, int **paths) {
  FILE *fp;
  fp = fopen("output.txt", "w");

  int n = *vertices;

  int i, j;
  for(i = 0; i < n; i++){
    for(j = 0; j < n; j++){
      if (j != n - 1){
        fprintf(fp, "%d ", paths[i][j]);
      }
      else {
        fprintf(fp, "%d\n", paths[i][j]);
      }
    }
  }
  fclose(fp);
}

int main(int argc, char *argv[]) {
  int threadCount = atoi(argv[1]);

  char *input = calloc(500, sizeof(char));
  input = argv[2];

  int *n = malloc(sizeof(int));;
  int **weights =  read_input(input, n);
  if(threadCount > *n) { //we can't have more threads than lines
    threadCount = *n;
  }

  StopWatch_t *watch = (StopWatch_t *)malloc(sizeof(StopWatch_t));
  startTimer(watch);
  startfw(n, weights, threadCount);
  stopTimer(watch);
  double elapsedTime = getElapsedTime(watch);
  printf("Elapsed time: %f\n", elapsedTime);
  output(n, weights);
  int valid = validate(input, n);
  if(valid)
    printf("%s was computed correctly.\n", input);
  else
    printf("ERROR: %s was NOT computed correctly or was not found.\n",input);

  return 1;
}
  



