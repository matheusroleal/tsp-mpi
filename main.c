#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <mpi.h>
#include "headers/tour.h"
#include "headers/stack.h"
#include "headers/graph.h"
#include "headers/tsp.h"
#include "headers/utils.h"
#include <unistd.h>
#include <float.h>
#include <time.h>



#define HOMETOWN 0
#define GLOBAL_BEST_COST_TAG 10
// Global for graph
int n_cities;
int* nodes;
float** adj_m;

// Global for all threads
tour* best_tour;
float best_tour_cost;
graph* graph_t;
int stack_size;
pthread_mutex_t execute_mutex;
term* term_t;
int threads_num;
int num_process, process_rank;

void* execute(void* arg) {
  stack* my_stack = (stack*)arg;

  EvaluateTours(my_stack, graph_t, best_tour, &best_tour_cost, execute_mutex, term_t, NumNodes(graph_t), HOMETOWN, threads_num, num_process, process_rank);

  pthread_exit(NULL);
}

void ThreadsSplit(stack* initial_stack, int num_threads, stack** threads_stacks, graph* graph_t) {
  int error;
  int n_nodes = NumNodes(graph_t);
  int stack_size = n_nodes * n_nodes;

  InitializeThreadStacks(num_threads, stack_size, threads_stacks, graph_t);
  FillThreadStacks(initial_stack, num_threads, stack_size, threads_stacks, graph_t);

  for(int i=0; i < num_threads; i++) {
    PrintStackInfo(threads_stacks[i]);
  }

  pthread_t* workers = (pthread_t*) calloc (threads_num, sizeof(pthread_t));
  if (!workers) { 
    exit(-1); 
  }

  for(int i=0; i < num_threads; i++) {
    error = pthread_create(&workers[i], NULL, &execute, (void*)threads_stacks[i]);
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

  InitializeProcessStacks(root_node, num_process, stack_size, stacks, graph_t);
  FillProcessStacks(root_node, num_process, stack_size, stacks, graph_t);
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

void SendStacks(stack** process_stacks, int num_process) {
  for (int i=0; i<num_process; i++) {
    int current_process_rank = i+1;

    // Send size of stack
    int current_stack_size = GetSize(process_stacks[i]);
    MPI_Send(&current_stack_size, 1, MPI_INT, current_process_rank, 0, MPI_COMM_WORLD);

    // Pop tours into another stack to reverse the order
    tour** stack_tours = (tour**) calloc (current_stack_size, sizeof(tour*));
    for (int j=current_stack_size-1; j>=0; j--) {
      stack_tours[j] = Pop(process_stacks[i]);
    }

    // For each tour in stack
    for (int j=0; j<current_stack_size; j++) {
        // Send array of cities in tour
        int * cities_in_tour = GetCitiesInTour(stack_tours[j]);

        MPI_Send(cities_in_tour, n_cities+1, MPI_INT, current_process_rank, j, MPI_COMM_WORLD);
    }
  }
}

stack* ReceiveStack(int process_rank) {

  MPI_Status receive_status;
  int size_of_my_stack;
  MPI_Recv(&size_of_my_stack, 1, MPI_INT, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &receive_status);

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
  return my_stack;
}

int main(int argc, char** argv) {

  if (argc < 4) {
    printf("Missing parameters..\nUsage: ./main <num_threads> <num_cities> <path_to_matrix_file>\n"); 
    exit(-1); 
  }

  clock_t start = clock();

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

  // Check if there are multiple processes to distribute work
  if (num_process > 1) {

    if (process_rank == 0) {

      ShowMatrix(n_cities, adj_m);

      // Create stacks for each process
      stack* process_stacks[num_process-1];
      ProcessSplit(HOMETOWN, num_process-1, process_stacks, graph_t);

      // Send stack for each process
      SendStacks(process_stacks, num_process-1);

      for(int i=0; i < num_process-1; i++) {
        FreeStack(process_stacks[i]);
      }

      sleep(5);
    }
    else {
      stack * my_stack = ReceiveStack(process_rank);

      // Split my stack into different threads and start
      best_tour = CreateTour(n_cities + 1);
      stack_size = (n_cities*n_cities)/2;
      stack* threads_stacks[threads_num];
      term_t = CreateTerm();
      best_tour_cost = FLT_MAX;

      pthread_mutex_init(&execute_mutex, NULL);
      ThreadsSplit(my_stack, threads_num, threads_stacks, graph_t);

    }
  }
  else {
    // only 1 process, do all work by itself
    MPI_Finalize();
    return 0;
  }
  
  struct {
    int cost;
    int rank;
  } local_data;

  struct {
    int cost;
    int rank;
  } global_data;

  if (process_rank != 0)
    local_data.cost = (int) GetTourCost(best_tour);
  else
    local_data.cost = (int) FLT_MAX;
  local_data.rank = process_rank;

  MPI_Allreduce(&local_data, &global_data, 1, MPI_2INT, MPI_MINLOC, MPI_COMM_WORLD);
  
  if (global_data.rank == 0) {
    MPI_Finalize();
    return 0;
  }
  else if (process_rank == 0) {
    // receive best cost from global_data.rank
    int * cities_in_tour = calloc(n_cities+1, sizeof(int));
    MPI_Status receive_status;
    MPI_Recv(cities_in_tour, n_cities + 1, MPI_INT, global_data.rank, GLOBAL_BEST_COST_TAG, MPI_COMM_WORLD, &receive_status);

    // Create tour
    tour * best_global_tour  = CreateTour(n_cities+1);

    // Set cities to tour
    AddCitiesToTour(best_global_tour, graph_t, cities_in_tour, n_cities+1);

    printf("\n[Process %d] BEST GLOBAL TOUR RECEIVED:\n", process_rank);
    PrintTourInfo(best_global_tour);

    clock_t end = clock();
    printf("\n[Process %d] Total execution time (ms): %f\n", process_rank, (double)(end - start) / CLOCKS_PER_SEC);

    FreeGraph(graph_t);

  } else if (process_rank == global_data.rank) {

    // Send array of cities in tour to process 0
    int * cities_in_tour = GetCitiesInTour(best_tour);
    MPI_Send(cities_in_tour, n_cities+1, MPI_INT, 0, GLOBAL_BEST_COST_TAG, MPI_COMM_WORLD);
  } 

  MPI_Finalize();
  return 0;
}
