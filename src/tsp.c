#include <stdlib.h>
#include <stdio.h>
#include <mpi.h>
#include "../headers/tsp.h"
#include <sys/types.h>
#include <unistd.h>
#include <sys/syscall.h>

#define NEW_COST_TAG 2
struct term_t {
  stack* new_stack;
  int threads_in_cond_wait;
  pthread_cond_t term_cond_var;
  pthread_mutex_t term_mutex;
};

term* CreateTerm() {
  term* term_t = (term*) calloc (1, sizeof(term));

  if(!term_t) { 
    printf("Failed to create term.\n"); 
    exit(-1); 
  }

  term_t->new_stack = NULL;
  term_t->threads_in_cond_wait = 0;
  pthread_cond_init(&term_t->term_cond_var, NULL);
  pthread_mutex_init(&term_t->term_mutex, NULL);

  return term_t;
}

int Termination(stack* my_stack, term* term_t, int num_threads) {
  int my_stack_size = GetSize(my_stack);

  if(my_stack_size >= 2 && term_t->threads_in_cond_wait > 0 && term_t->new_stack == NULL) {
    pthread_mutex_lock(&term_t->term_mutex);

    if(term_t->threads_in_cond_wait > 0 && term_t->new_stack == NULL) {
      printf("splitting stack...\n");
      term_t->new_stack = SplitStack(my_stack);

      pthread_cond_signal(&term_t->term_cond_var);
    }

    pthread_mutex_unlock(&term_t->term_mutex);

    return 0;
  } else if (!Empty(my_stack)) {
    return 0;
  } else {
    pthread_mutex_lock(&term_t->term_mutex);

    if(term_t->threads_in_cond_wait == num_threads - 1) {
      // last thread running
      term_t->threads_in_cond_wait++;
      printf("threads in wait: %d\n", term_t->threads_in_cond_wait);
      pthread_cond_broadcast(&term_t->term_cond_var);

      pthread_mutex_unlock(&term_t->term_mutex);

      return 1;
    } else {
      // other threads stil working... wait for work
      term_t->threads_in_cond_wait++;
      while(pthread_cond_wait(&term_t->term_cond_var, &term_t->term_mutex) != 0);

      if(term_t->threads_in_cond_wait < num_threads) {
        CopyStack(term_t->new_stack, my_stack);

        term_t->new_stack = NULL;
        term_t->threads_in_cond_wait--;

        pthread_mutex_unlock(&term_t->term_mutex);

        return 0;
      } else {
        printf("all threads done!\n");
        pthread_mutex_unlock(&term_t->term_mutex);

        return 1;
      }
    }
  }
}

void UpdateBestTour(tour* old_best_tour, tour* new_best_tour, float* global_best_cost, int num_processes, int my_process_rank, int thread_id) {

    // Checking again inside mutex
    if (BestTour(new_best_tour, old_best_tour)) {

      printf("[Process %d][Thread %d] Update local best tour!\n", my_process_rank, thread_id);
      PrintTourInfo(new_best_tour);

      // Update value in this process
      CopyTour(old_best_tour, new_best_tour);

      float local_best_cost = GetTourCost(new_best_tour);

      // Check if is also best global tour
      if (local_best_cost < *global_best_cost) {
        // Update global best cost
        *global_best_cost = local_best_cost;

        // Send update to other processes
        // Prepare buffer for sending the best cost
        int data_size;
        int message_size;
        int send_buffer_size;
        int total_buffer_size;
        MPI_Pack_size(1, MPI_FLOAT, MPI_COMM_WORLD, &data_size);
        message_size = data_size + MPI_BSEND_OVERHEAD;
        send_buffer_size = (num_processes-1) * message_size;
        total_buffer_size = send_buffer_size;

        char *send_buffer;
        send_buffer = (char *) calloc (total_buffer_size, sizeof(char));
        MPI_Buffer_attach(send_buffer, total_buffer_size);
        printf("\n[Process %d] [Thread %d] Buffer attached! Size: %d , buffer: %p\n", my_process_rank, thread_id, total_buffer_size, send_buffer);
        
        for (int i=1; i<num_processes; i++) {
          if (i != my_process_rank) {
            printf("[Process %d][Thread %d] Sending best cost %f to process %d!\n", my_process_rank, thread_id, local_best_cost, i);
            MPI_Bsend(&local_best_cost, 1, MPI_FLOAT, i, NEW_COST_TAG, MPI_COMM_WORLD);
          }
        }

        printf("\n[Process %d][Thread %d] Waiting to detach buffer... \n", my_process_rank, thread_id);
        char * buf;
        int buf_size;
        MPI_Buffer_detach(&buf, &buf_size);
        printf("\n[Process %d][Thread %d] Buffer detached! Size: %d , buffer: %p\n", my_process_rank, thread_id, buf_size, buf);
      }


    }

}

void CheckForReceivedBestTour(float * global_best_cost, int my_process_rank, int thread_id) {
  int message_available = 0;
  MPI_Status status;
  printf("[Process %d][Thread %d] Checking if there is any message to receive...\n", my_process_rank, thread_id);
  MPI_Iprobe(MPI_ANY_SOURCE, NEW_COST_TAG, MPI_COMM_WORLD, &message_available, &status);

  while (message_available) {
    float received_new_best_cost;
    printf("[Process %d][Thread %d] Receiving best cost from process %d!\n", my_process_rank, thread_id, status.MPI_SOURCE);
    MPI_Recv(&received_new_best_cost, 1, MPI_FLOAT, status.MPI_SOURCE, NEW_COST_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    if (received_new_best_cost < *global_best_cost) {
      // SetTourCost(local_best_tour, received_new_best_cost);
      // ATUALIZAR O BEST TOUR GLOBAL AQUI
      *global_best_cost = received_new_best_cost;
    }
    MPI_Iprobe(MPI_ANY_SOURCE, NEW_COST_TAG, MPI_COMM_WORLD, &message_available, &status);
  }
}

void EvaluateTours(stack* stack_t, graph* graph_t, tour* best_tour, float* best_tour_cost, pthread_mutex_t evaluate_mutex, term* term_t, int n_cities, int hometown, int num_threads, int num_processes, int my_process_rank) {
  tour* current_tour;
  pid_t thread_id = syscall(__NR_gettid);
  printf("[Process %d][Thread %d] ======= STARTING TO EVALUATE TOURS ======\n", my_process_rank, thread_id);
  while(!Termination(stack_t, term_t, num_threads)) {
    current_tour = Pop(stack_t);

    CheckForReceivedBestTour(best_tour_cost, my_process_rank, thread_id);

    if(GetTourNumberCities(current_tour) == n_cities) {
      printf("[Process %d][Thread %d] (global_best_tour=%f) Complete tour popped: \n", my_process_rank, thread_id, *best_tour_cost);
      PrintTourInfo(current_tour);
      printf("[Process %d][Thread %d] ------------- \n", my_process_rank, thread_id);
      // add hometown to current tour to compute the final cost
      AddCity(current_tour, graph_t, hometown);


      if(BestTour(current_tour, best_tour)) {
        pthread_mutex_lock(&evaluate_mutex);
        // sleep(2);
        UpdateBestTour(best_tour, current_tour, best_tour_cost, num_processes, my_process_rank, thread_id);
        // sleep(2);

        pthread_mutex_unlock(&evaluate_mutex);
      }
      // printf("aqui1\n");
    } else {
      for (int nbr=n_cities-1; nbr >= 0; nbr--) {
        // dont go back to hometown, with this we can set hometown to any city
        if (nbr == hometown) { continue; }

        if(!TourContainCity(current_tour, nbr)) {
          AddCity(current_tour, graph_t, nbr);
          PushCopy(stack_t, current_tour);
          RemoveLastCity(current_tour, graph_t);
        }
      }
      // printf("aqui2\n");
    }
    FreeTour(current_tour);
  }
  printf("[Process %d][Thread %d] ======= FINISHED EVALUATING TOURS ======\n", my_process_rank, thread_id);
}

int AnyStackNotFilled(int num_threads, stack* stacks[num_threads]) {
  for(int i=0; i < num_threads; i++) {
    if(GetTourNumberCities(GetLastTour(stacks[i])) == 1 ) { return 1; }
  }

  return 0;
}

void InitializeStacks(int root_node, int n_stacks, int stack_size, stack* stacks[n_stacks], graph* graph_t) {
  tour* initial_tour = CreateTour(NumNodes(graph_t) + 1);
  AddCity(initial_tour, graph_t, root_node);

  // each stack initialize with minimal tour, that is only root node
  for(int i=0; i < n_stacks; i++) {
    stacks[i] = CreateStack(stack_size);
    PushCopy(stacks[i], initial_tour);
  }
}

void InitializeProcessStacks(int root_node, int n_stacks, int stack_size, stack* stacks[n_stacks], graph* graph_t) {
  for(int i=0; i < n_stacks; i++) {
    stacks[i] = CreateStack(stack_size);
  }
}

void FillStacks(int root_node, int num_threads, int stack_size, stack* stacks[num_threads], graph* graph_t) {
  float nbr_cost;
  int num_nodes = NumNodes(graph_t);
  int visited_nodes[num_nodes], current_node;
  tour* current_tour;
  stack* current_stack;

  // initialize visited nodes array
  for(int i=0; i < num_nodes; i++) { visited_nodes[i] = 0; }

  current_node = root_node;

  while(AnyStackNotFilled(num_threads, stacks)) {
    for(int nbr=num_nodes-1; nbr >= 0; nbr--) {
      nbr_cost = GetEdgeWeight(graph_t, current_node, nbr);

      // current node not neighbour from nbr... this kind of loop is not so efficient, we always itera through all nodes - 1
      if (nbr_cost == 0.0 || visited_nodes[current_node] == 1) { continue; }

      current_stack = stacks[nbr % num_threads];
      current_tour = GetLastTour(current_stack);
      AddCity(current_tour, graph_t, nbr);
      PushCopy(current_stack, current_tour);

      visited_nodes[nbr] = 1;
    }

    current_node = GetTourLastCity(GetLastTour(stacks[0]));
  }
}


void FillProcessStacks(int root_node, int num_threads, int stack_size, stack* stacks[num_threads], graph* graph_t) {
  float nbr_cost;
  int num_nodes = NumNodes(graph_t);
  int current_node;
  tour* current_tour;
  tour* initial_tour;
  stack* current_stack;
  Queue * tours_queue = CreateQueue();

  printf("\n\nFilling stacks with initial tours... \n\n");
  initial_tour = CreateTour(num_nodes + 1);
  AddCity(initial_tour, graph_t, root_node);

  // Insert root node tour into queue
  enQueue(tours_queue, (void*) initial_tour);

  // Populate queue with at least num_thread tours
  while(GetQueueSize(tours_queue) < num_threads) {
      
    // Pop tour from queue
    current_tour = (tour*) deQueue(tours_queue);
    current_node = GetTourLastCity(current_tour);

    for(int nbr=0; nbr < num_nodes; nbr++) {
      nbr_cost = GetEdgeWeight(graph_t, current_node, nbr);
      if (nbr == current_node || nbr_cost == 0.0 || TourContainCityOrHometown(current_tour, nbr)) {
        continue;
      }

      // Create new tour from current tour
      tour * new_tour = CreateTour(num_nodes+1);
      CopyTour(new_tour, current_tour);
      AddCity(new_tour, graph_t, nbr);

      enQueue(tours_queue, (void*) new_tour);
    }
  }

  // Insert tour from queue into stacks
  int stack_index = 0;
  while (GetQueueSize(tours_queue) > 0) {
    current_tour = (tour*) deQueue(tours_queue);
    current_stack = stacks[stack_index % num_threads];

    PushCopy(current_stack, current_tour);

    stack_index++;
  }

}

void InitializeThreadStacks(int n_stacks, int stack_size, stack* stacks[n_stacks], graph* graph_t) {
  for(int i=0; i < n_stacks; i++) {
    stacks[i] = CreateStack(stack_size);
  }
}

void FillThreadStacks(stack* initial_stack, int num_threads, int stack_size, stack* stacks[num_threads], graph* graph_t) {
  float nbr_cost;
  int num_nodes = NumNodes(graph_t);
  int current_node;
  tour* current_tour;
  stack* current_stack;
  Queue * tours_queue = CreateQueue();

  // Creating queue of tours from initial stack
  while(!Empty(initial_stack)) {
    tour* current_tour = Pop(initial_stack);
    enQueue(tours_queue, (void*) current_tour);
  }

  while(GetQueueSize(tours_queue) < num_threads) {
      
    // Pop tour from queue
    current_tour = (tour*) deQueue(tours_queue);
    current_node = GetTourLastCity(current_tour);

    for(int nbr=0; nbr < num_nodes; nbr++) {
      nbr_cost = GetEdgeWeight(graph_t, current_node, nbr);
      if (nbr == current_node || nbr_cost == 0.0 || TourContainCityOrHometown(current_tour, nbr)) {
        continue;
      }

      // Create new tour from current tour
      tour * new_tour = CreateTour(num_nodes+1);
      CopyTour(new_tour, current_tour);
      AddCity(new_tour, graph_t, nbr);

      enQueue(tours_queue, (void*) new_tour);
    }
  }

  // Insert tour from queue into stacks
  int stack_index = 0;
  while (GetQueueSize(tours_queue) > 0) {
    current_tour = (tour*) deQueue(tours_queue);
    current_stack = stacks[stack_index % num_threads];

    PushCopy(current_stack, current_tour);

    stack_index++;
  }
}
