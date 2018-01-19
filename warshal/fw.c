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

// Phase 1: read file into memory
// read_input 2 takes in vertices as an out parameter.
// It returns the 2d array weights after reading through input.txt
int ** read_input(char* input, int *vertices){
  int n;
  FILE *fp;
  int i, j;
  int check;
  //  printf("opening file\n");
  fp = fopen(input, "r");
  check =   fscanf(fp, "%d\n", &n); //grab our n value
  if (check != 1)
    {
      printf("read_input could not properly scan input file.\n");
      exit(1);
    }
  *vertices = n; //assign it to out parameter
  // printf("n is %d\n", n);

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
        printf("Input.txt is invalid due to improper edge weight.\nEdges need t\
o be <= 1000 or 10000000 for infinite weight.\n");
        exit(1);
      }
      weights[i][j] = temp;
      // printf("Weights %d,%d is %d and i = %d\n", i, j, weights[i][j], i);
    }
  }
  fclose(fp);
  return weights;
}


// Phase 2: the algorithm itself (serial first; parallel next);
void fw(int *vertices, int **weights){
  int n = *vertices;
  int k, i, j;
  for(k = 0; k < n; k++){
    //I will need a barrier here. ALL threads must complete their work on a particular k before advancing.
    for(i = 0; i < n; i++){
      for(j = 0; j < n; j++){
	//do more work
	if (weights[i][j] > (weights[i][k] + weights[k][j])) {
	  weights[i][j] = weights[i][k] + weights[k][j];
	}
      }
    }
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

  char *input = calloc(500, sizeof(char));
  input = argv[1];

  int *n = malloc(sizeof(int));;
  int **weights =  read_input(input, n);
  StopWatch_t *watch = (StopWatch_t *)malloc(sizeof(StopWatch_t));
  startTimer(watch);
  fw(n, weights);
  stopTimer(watch);
  double elapsedTime = getElapsedTime(watch);
  printf("Elapsed Time = %f\n", elapsedTime);

  output(n, weights);
  int valid = validate(input, n);
  if(valid)
    printf("%s was computed correctly.\n", input);
  else
    printf("ERROR: %s was NOT computed correctly or was not found.\n",  input);

  return 1;
}
