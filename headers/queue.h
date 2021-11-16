
typedef struct {
    struct Node *out, *in;
    int size;
} Queue;

Queue* CreateQueue();

void enQueue(Queue* q, void * item);

void * deQueue(Queue* q);

int GetQueueSize(Queue* q);