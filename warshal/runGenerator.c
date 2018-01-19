#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

int main(int argc, char *argv[]) {

  int n = atoi(argv[1]);
  FILE *fp;

  char fileName[50];
  sprintf(fileName,"timedInput%d.txt", n);
  fp = fopen(fileName, "w");
  
  fprintf(fp, "%d\n", n);

  int i;
  int j;
  for(i = 0; i < n; i++){
    for(j = 0; j < n; j++){
      int temp = rand() % 1000;
      if (i == j){
	temp = 0; //distance to self should be 0
      }
      if (j != (n-1)){
	fprintf(fp, "%d ", temp);
      }
      else{
	fprintf(fp, "%d\n", temp);
      }
    }
  }
  fclose(fp);
  return 1;
}
