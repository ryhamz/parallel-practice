#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int validate(char *input, int *vertices) {

  int n = *vertices;
  int check;
  int i, j;
  FILE *fp;

  fp = fopen("output.txt", "r");
  
  //allocate 2d array
  int **paths = (int **)malloc(n * sizeof(int *));
  for(i = 0; i < n; i++){
    paths[i] = (int *)malloc(n * sizeof(int));
  }
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
      paths[i][j] = temp;
    }
  }


  //branch off into various test cases

  //Two disconnected vertices  
  if(strcmp(input, "testInput1.txt") == 0) {
    //    printf("checking paths\n");
    if(paths[0][0] == 0 && paths[0][1] == 10000000 && paths[1][0] == 10000000 && paths[1][1] == 0)
      {
	return 1;
      }
  }

  if (strcmp(input, "testInput2.txt") == 0) {
    if(paths[0][0] == 0 && paths[0][1] == -1 && paths[0][2] == -2 && paths[0][3] == 0 &&
       paths[1][0] == 4 &&  paths[1][1] == 0 &&  paths[1][2] == 2 &&  paths[1][3] == 4 &&
       paths[2][0] == 5 &&  paths[2][1] == 1 &&  paths[2][2] == 0 &&  paths[2][3] == 2 &&
       paths[3][0] == 3 &&  paths[3][1] == -1 &&  paths[3][2] == 1 &&  paths[3][3] == 0)
      {
	return 1;
      }
}

  return 0;
}
