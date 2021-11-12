#include <stdlib.h>
#include <stdio.h>
#include "../headers/stack.h"


struct stack_t {
  tour** tours;
  int max_size;
  int size;
};

stack* CreateStack(int max_cities) {
  stack* stack_t = (stack*) calloc (1, sizeof(stack));

  if(!stack_t) {
    printf("Failed to create stack.\n");
    exit(-1);
  }

  stack_t->tours = (tour**) calloc (max_cities, sizeof(tour*));
  stack_t->max_size = max_cities;
  stack_t->size = 0;

  return stack_t;
}

void FreeStack(stack* stack_t) {
  for(int i=0; i< stack_t->size; i++) {
    free(stack_t->tours[i]);
  }

  free(stack_t->tours);
}

void PushCopy(stack* stack_t, tour* tour_t) {
  if (stack_t->size == stack_t->max_size) {
    printf("Stack full!\n");
    return;
  }

  int loc = stack_t->size;
  int tour_size = GetTourMaxCities(tour_t);
  tour* tour_tmp = CreateTour(tour_size);

  CopyTour(tour_tmp, tour_t);

  stack_t->tours[loc] = tour_tmp;
  stack_t->size++;
}

tour* Pop(stack* stack_t) {
  if (Empty(stack_t)) {
    printf("Empty stack!\n");
    return NULL;
  }

  stack_t->size--;

  return stack_t->tours[stack_t->size];
}

stack* SplitStack(stack* src_stack) {
  stack* new_stack = CreateStack(src_stack->max_size/2);
  stack* aux_stack = CreateStack(src_stack->max_size/2);

  for(int i=0; i < src_stack->size; i++) {
    if(i % 2 == 0) {
      PushCopy(new_stack, src_stack->tours[i]);
    } else {
      PushCopy(aux_stack, src_stack->tours[i]);
    }
  }

  src_stack = aux_stack;

  return new_stack;
}

void CopyStack(stack* orig, stack* dest) {
  tour* tour_t;
  int stack_max_size = orig->max_size;
  stack* aux = CreateStack(stack_max_size);

  while(!Empty(orig)) {
    tour_t = Pop(orig);
    PushCopy(aux, tour_t);
  }

  while(!Empty(aux)) {
    tour_t = Pop(aux);
    PushCopy(dest, tour_t);
  }

  FreeStack(aux);
}

int Empty(stack* stack_t) {
  if (stack_t->size == 0) { return 1; }

  return 0;
}

tour* GetLastTour(stack* stack_t) {
  return stack_t->tours[stack_t->size - 1];
}

int GetSize(stack* stack_t) {
  return stack_t->size;
}

// Just for debugging
void PrintStackInfo(stack* stack_t) {
  printf("Stack size: %d\n", GetSize(stack_t));
  for(int i=GetSize(stack_t) - 1; i >= 0; i--) {
    PrintTourInfo(stack_t->tours[i]);
  }
}
