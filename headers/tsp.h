#include <pthread.h>
#include "../headers/stack.h"
#include "../headers/tour.h"
#include "../headers/graph.h"
#include "../headers/queue.h"

typedef struct term_t term;

term* CreateTerm();

void EvaluateTours(stack* stack_t, graph* graph_t, tour* best_tour, float* best_tour_cost, pthread_mutex_t evaluate_mutex, term* term_t, int n_cities, int hometown, int num_threads, int num_processes, int my_process_rank);

void InitializeProcessStacks(int root_node, int n_stacks, int stack_size, stack* stacks[n_stacks], graph* graph_t);

void FillProcessStacks(int root_node, int num_threads, int stack_size, stack* stacks[num_threads], graph* graph_t);

void InitializeThreadStacks(int n_stacks, int stack_size, stack* stacks[n_stacks], graph* graph_t);

void FillThreadStacks(stack* initial_stack, int num_threads, int stack_size, stack* stacks[num_threads], graph* graph_t);

