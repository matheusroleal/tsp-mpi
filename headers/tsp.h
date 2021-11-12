#include <pthread.h>
#include "../headers/stack.h"
#include "../headers/tour.h"
#include "../headers/graph.h"

typedef struct term_t term;

term* CreateTerm();

void EvaluateTours(stack* stack_t, graph* graph_t, tour* best_tour, pthread_mutex_t evaluate_mutex, term* term_t, int n_cities, int hometown, int num_threads);

void InitializeStacks(int root_node, int n_stacks, int stack_size, stack* stacks[n_stacks], graph* graph_t);

void FillStacks(int root_node, int num_threads, int stack_size, stack* stacks[num_threads], graph* graph_t);

