#include <stdlib.h>
#include <stdio.h>
#include "../headers/tsp.h"

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

void EvaluateTours(stack* stack_t, graph* graph_t, tour* best_tour, pthread_mutex_t evaluate_mutex, term* term_t, int n_cities, int hometown, int num_threads) {
  tour* current_tour;

  while(!Termination(stack_t, term_t, num_threads)) {
    current_tour = Pop(stack_t);

    if(GetTourNumberCities(current_tour) == n_cities) {
      // add hometown to current tour to compute the final cost
      AddCity(current_tour, graph_t, hometown);

      if(BestTour(current_tour, best_tour)) {
        pthread_mutex_lock(&evaluate_mutex);

        printf("Update best tour!\n");
        PrintTourInfo(current_tour);
        CopyTour(best_tour, current_tour);

        pthread_mutex_unlock(&evaluate_mutex);
      }
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
    }
    FreeTour(current_tour);
  }
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
