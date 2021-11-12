#include "tour.h"

typedef struct stack_t stack;

stack* CreateStack(int max_cities);

void FreeStack(stack* stack_t);

void PushCopy(stack* stack_t, tour* tour_t);

tour* Pop(stack* stack_t);

stack* SplitStack(stack* src_stack);

void CopyStack(stack* orig, stack* dest);

int Empty(stack* stack_t);

tour* GetLastTour(stack* stack_t);

int GetSize(stack* stack_t);

// Just for debugging
void PrintStackInfo(stack* stack_t);
