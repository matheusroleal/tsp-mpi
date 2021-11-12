typedef struct graph_t graph;

graph* CreateGraph(int size, int *nodes, float **adjency_matrix);

void FreeGraph(graph* graph_t);

float GetEdgeWeight(graph* graph_t, int i, int j);

int NumNodes(graph* graph_t);

// Just for debugging
void PrintGraph(graph *graph_t);
