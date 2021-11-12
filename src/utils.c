#include <stdlib.h>
#include <stdio.h>

void ReadNCities(int* n_cities) {
  printf("Numero de cidades:\n");
  scanf("%d", n_cities);
}

void ReadNThreads(int* n_threads) {
  printf("Numero de threads:\n");
  scanf("%d", n_threads);
}

void ReadMatrixPath(char* word) {
  printf("Path para arquivo com a matriz:\n");
  scanf("%s" , word);
}

void ReadMatrix(size_t rows, size_t cols, float **a, char* filename) {

  FILE *pf;
  pf = fopen (filename, "r");
  if (pf == NULL) {
    exit(-1);
  }

  for(size_t i = 0; i < rows; ++i) {
    for(size_t j = 0; j < cols; ++j) {
      fscanf(pf, "%f", a[i] + j);
    }
  }

  fclose (pf); 
}

void ShowMatrix(int n_cities, float **adj_m) {
  printf("\nMatriz definida\n");
  for(size_t i = 0; i < n_cities; ++i) {
    for(size_t j = 0; j < n_cities; ++j) {
      printf("%-4f ", adj_m[i][j]);
    }
    puts("");
  }
  printf("\n");
}