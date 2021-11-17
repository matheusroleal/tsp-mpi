#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <mpi.h>
#include "headers/tour.h"
#include "headers/stack.h"
#include "headers/graph.h"
#include "headers/tsp.h"
#include "headers/utils.h"

#define HOMETOWN 0

// Global for graph
int n_cities;
int* nodes;
float** adj_m;

// Global for all threads
tour* best_tour;
graph* graph_t;
int stack_size;
pthread_mutex_t execute_mutex;
term* term_t;
int threads_num;

void* execute(void* arg) {
  stack* my_stack = (stack*)arg;

  EvaluateTours(my_stack, graph_t, best_tour, execute_mutex, term_t, NumNodes(graph_t), HOMETOWN, threads_num);

  pthread_exit(NULL);
}

void ThreadsSplit(int root_node, int num_threads, stack** stacks, graph* graph_t) {
  int error;
  int n_nodes = NumNodes(graph_t);
  int stack_size = n_nodes * n_nodes;

  InitializeStacks2(root_node, num_threads, stack_size, stacks, graph_t);
  FillStacks2(root_node, num_threads, stack_size, stacks, graph_t);

  for(int i=0; i < num_threads; i++) {
    PrintStackInfo(stacks[i]);
  }

  pthread_t* workers = (pthread_t*) calloc (threads_num, sizeof(pthread_t));
  if (!workers) { 
    exit(-1); 
  }

  for(int i=0; i < num_threads; i++) {
    error = pthread_create(&workers[i], NULL, &execute, (void*)stacks[i]);
    if(error) { 
      printf("Failed to create thread: %lu\n", (long)workers[i]); 
      exit(-1); 
    }
  }

  for(int i=0; i < num_threads; i++) {
    error = pthread_join(workers[i], NULL);
    if(error) { 
      printf("Failed to join thread: %lu\n", (long)workers[i]); 
      exit(-1); 
    }
  }
}

void ProcessSplit(int root_node, int num_process, stack** stacks, graph* graph_t) {
  int n_nodes = NumNodes(graph_t);
  int stack_size = n_nodes * n_nodes;

  InitializeStacks2(root_node, num_process, stack_size, stacks, graph_t);
  FillStacks2(root_node, num_process, stack_size, stacks, graph_t);
}


void InitializeInstance(char * path_matrix_file) {

  nodes = calloc(n_cities, sizeof(int));
  if(!nodes) { 
    printf("Failed to allocate nodes array.\n"); exit(-1); 
  }

  for(int i=0; i < n_cities; i++) {
    nodes[i] = i;
  } 

  adj_m = calloc(n_cities, sizeof(float*));
  if(!adj_m) { 
    printf("Failed to allocate matrix.\n"); 
    exit(-1); 
  }
  for(int i = 0; i < n_cities; i++) {
    adj_m[i] = calloc(n_cities, sizeof(float));
    if(!adj_m[i]) { 
      printf("Failed to allocate matrix.\n"); 
      exit(-1); 
    }
  }

  ReadMatrix(n_cities, n_cities, adj_m, path_matrix_file);
}

int main(int argc, char** argv) {
  int num_process, process_rank;

  if (argc < 4) {
    printf("Missing parameters..\nUsage: ./main <num_threads> <num_cities> <path_to_matrix_file>\n"); 
    exit(-1); 
  }

  MPI_Init(&argc, &argv);

  MPI_Comm_size(MPI_COMM_WORLD, &num_process);
  MPI_Comm_rank(MPI_COMM_WORLD, &process_rank);

  if (process_rank == 0) {
      threads_num = atoi(argv[1]);
      n_cities = atoi(argv[2]);
      char* path_to_matrix_file = argv[3];

      InitializeInstance(path_to_matrix_file);

      ShowMatrix(n_cities, adj_m);

      graph_t = CreateGraph(n_cities, nodes, adj_m);

      best_tour = CreateTour(n_cities + 1);
      stack_size = (n_cities*n_cities)/2;
      stack* threads_stacks[threads_num];

      term_t = CreateTerm();

      stack* process_stacks[num_process];
      ProcessSplit(HOMETOWN, num_process, process_stacks, graph_t);

      for (int i=0; i<num_process; i++) {
        PrintStackInfo(process_stacks[i]);
      }

      for(int i=0; i < num_process; i++) {
        FreeStack(process_stacks[i]);
      }

      FreeGraph(graph_t);
  }
  else {
      printf("[Process %d]\n", process_rank);
  }

  MPI_Finalize();
  return 0;

  // for (int i=0; i<num_process; i++) {
  //   PrintStackInfo(process_stacks[i]);
  // }
  

  // MPI_Bcast(&n_cities, 1, MPI_INT, 0, MPI_COMM_WORLD);
  // MPI_Bcast(&nodes, n_cities, MPI_INT, 0, MPI_COMM_WORLD);

  // PrintStackInfo(process_stacks[process_rank]);
  

  // stack* process_stack = process_stacks[process_rank];
  // PrintStackInfo(process_stack);

  // pthread_mutex_init(&execute_mutex, NULL);

  // ThreadsSplit(HOMETOWN, threads_num, threads_stacks, graph_t);

  // printf("\nBEST TOUR: \n");
  // PrintTourInfo(best_tour);

  // for(int i=0; i < threads_num; i++) {
  //   FreeStack(threads_stacks[i]);
  // }

  // MPI_Finalize();
}
