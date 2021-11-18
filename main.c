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

  // Read parameters
  threads_num = atoi(argv[1]);
  n_cities = atoi(argv[2]);
  char* path_to_matrix_file = argv[3];

  // Initialize Graph
  InitializeInstance(path_to_matrix_file);
  graph_t = CreateGraph(n_cities, nodes, adj_m);

  // Initializa MPI
  MPI_Init(&argc, &argv);
  MPI_Comm_size(MPI_COMM_WORLD, &num_process);
  MPI_Comm_rank(MPI_COMM_WORLD, &process_rank);

  if (process_rank == 0) {

      ShowMatrix(n_cities, adj_m);

      // best_tour = CreateTour(n_cities + 1);
      // stack_size = (n_cities*n_cities)/2;
      // stack* threads_stacks[threads_num];
      // term_t = CreateTerm();

      // Create stacks for each process
      stack* process_stacks[num_process];
      ProcessSplit(HOMETOWN, num_process, process_stacks, graph_t);

      // Send stack for each process
      for (int i=1; i<num_process; i++) {
        printf("Sending stack to process %d\n", i);

        // Send size of stack
        int current_stack_size = GetSize(process_stacks[i]);
        MPI_Send(&current_stack_size, 1, MPI_INT, i, 0, MPI_COMM_WORLD);

        // Pop tours into another stack to reverse the order
        tour** stack_tours = (tour**) calloc (current_stack_size, sizeof(tour*));
        for (int j=current_stack_size-1; j>=0; j--) {
          stack_tours[j] = Pop(process_stacks[i]);
        }

        // For each tour in stack
        for (int j=0; j<current_stack_size; j++) {
            // Send array of cities in tour
            int * cities_in_tour = GetCitiesInTour(stack_tours[j]);

            MPI_Send(cities_in_tour, n_cities+1, MPI_INT, i, j, MPI_COMM_WORLD);
        }
      }

      for(int i=0; i < num_process; i++) {
        FreeStack(process_stacks[i]);
      }

      FreeGraph(graph_t);
  }
  else {
      printf("[Process %d] Waiting for stack..\n", process_rank);

      MPI_Status receive_status;
      int size_of_my_stack;
      MPI_Recv(&size_of_my_stack, 1, MPI_INT, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &receive_status);

      printf("[Process %d] Size of my stack: %d, Size of each tour: %d\n", process_rank, size_of_my_stack, n_cities+1);

  	  stack * my_stack = CreateStack(n_cities+1);
      for (int j=0; j<size_of_my_stack; j++) {

          // Create tour
          tour * current_tour = CreateTour(n_cities+1);
          int * cities_in_tour = calloc(n_cities+1, sizeof(int));

          // Receive cities
          MPI_Recv(cities_in_tour, n_cities+1, MPI_INT, 0, j, MPI_COMM_WORLD, &receive_status);

          // Set cities to tour
          AddCitiesToTour(current_tour, graph_t, cities_in_tour, n_cities+1);

          // Push tour to stack
          PushCopy(my_stack, current_tour);
      }

      printf("[Process %d] Received my stack:\n", process_rank);
      PrintStackInfo(my_stack);
  }

  MPI_Finalize();
  return 0;
  

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
